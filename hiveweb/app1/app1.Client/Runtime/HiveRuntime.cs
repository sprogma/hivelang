using app1.Shared.Types;
using Microsoft.AspNetCore.SignalR.Client;
using Microsoft.JSInterop;
using System.Runtime.CompilerServices;

namespace app1.Client.Runtime
{
    public class HiveRuntime(IJSRuntime js, HubConnection _hubConnection, CodeProgram _code)
    {
        public Dictionary<long, LinkObjectBase> objects = [];
        public Queue<RunningProgram> runningPrograms = [];
        public Queue<long> requests = [];
        public Dictionary<(long, long), Queue<RunningProgram>> queues = [];
        public CodeProgram code = _code;
        private VarType i64Type = _code.Types.Values.First(x => x.Type == TypeType.SCALAR && x.Name == "i64" && x.Bases[0] == 8);
        private VarType byteType = _code.Types.Values.First(x => x.Type == TypeType.SCALAR && x.Name == "byte" && x.Bases[0] == 1);
        private IJSRuntime JS = js;
        private HubConnection hubConnection = _hubConnection;


        private long GetNewObjectId()
        {
            return objects.Count + 1;
        }


        private async Task SetObject(long id, long param, ObjectBase value)
        {
            if (objects.TryGetValue(id, out var obj))
            {
                switch (obj)
                {
                    case LinkObjectArray loa:
                        loa.Items[param] = value;
                        return;
                    case LinkObjectPipe lope:
                        {
                            if (queues.TryGetValue((id, param), out var waitList))
                            {
                                if (waitList.Count > 0)
                                {
                                    runningPrograms.Enqueue(waitList.Dequeue());
                                }
                            }
                            lope.Pipe.Enqueue(value);
                        }
                        return;
                    case LinkObjectPromise lopr:
                        {
                            if (queues.TryGetValue((id, param), out var waitList))
                            {
                                foreach (var prog in waitList)
                                {
                                    runningPrograms.Enqueue(prog);
                                }
                                waitList.Clear();
                            }
                            lopr.Value = value;
                        }
                        return;
                }
            }
            /* send request, and not wait to result? */
            // TODO: wait for result?
            await hubConnection.InvokeAsync("SetObject", id, param, value.Encode());
        }

        private async Task<ObjectBase?> GetObject(RunningProgram program, long id, long param)
        {
            if (objects.TryGetValue(id, out var value))
            {
                switch (value)
                {
                    case LinkObjectArray loa:
                        if (param == -1)
                        {
                            return new ObjectScalar(i64Type!, BitConverter.GetBytes(loa.Items.LongLength));
                        }
                        return loa.Items[param];
                    case LinkObjectPipe lope:
                        return lope.Pipe.Dequeue();
                    case LinkObjectPromise lopr:
                        if (lopr.Value is null)
                        {
                            if (!queues.TryGetValue((id, param), out var queue))
                            {
                                queues[(id, param)] = queue = new();
                            }
                            queue.Enqueue(program);
                            return null;
                        }
                        return lopr.Value;
                }
            }
            /* add request */
            {
                if (!queues.TryGetValue((id, param), out var queue))
                {
                    queues[(id, param)] = queue = new();
                }
                queue.Enqueue(program);
            }
            _ = hubConnection.InvokeAsync("GetObject", id, param);
            return null;
        }

        public async Task AnswerGetObject(long id, long param, ObjectBase value)
        {
            /* remove all elements from queue */
            foreach (var prog in queues[(id, param)])
            {
                prog.AsyncResultValue = value;
                runningPrograms.Enqueue(prog);
            }
            queues[(id, param)].Clear();
        }

        public async Task<string> Run(long workerId, ObjectBase[] input)
        {

            (RunningProgram? prg, ObjectBase[]? output) = await PrepareWorker(workerId, input);
            if (prg is not null && output is not null)
            {
                var waitStart = DateTime.UtcNow;
                while (runningPrograms.Count > 0 || queues.Any(x => x.Value.Count > 0))
                {
                    if (runningPrograms.Count > 0)
                    {
                        RunningProgram cur = runningPrograms.Dequeue();
                        switch (await RunWorker(cur))
                        {
                            case ResultCode.HANG:
                                runningPrograms.Enqueue(cur);
                                break;
                            case ResultCode.QUEUED:
                                break;
                            case ResultCode.OK:
                                break;
                            case ResultCode.ERROR:
                                throw new Exception("Error: worker returned ERROR result code");
                        }
                        waitStart = DateTime.UtcNow;
                    }
                    else
                    {
                        var waitDuration = DateTime.UtcNow - waitStart;
                        if (waitDuration > TimeSpan.FromSeconds(2))
                        {
                            bool res = await JS.InvokeAsync<bool>("confirm", $"Process was idle more than 2 seconds, do you want to cansel calculations?");
                            if (res)
                            {
                                waitStart = DateTime.UtcNow;
                                break;
                            }
                        }
                    }
                }

                string results = string.Join(";", output.Select(x => ObjectVisualizer.GetObj(this, x)));
                return results;
            }
            return "Error: wrong code";
        }

        private async Task<(RunningProgram?, ObjectBase[]? outputs)> PrepareWorker(long id, ObjectBase[] inputs)
        {
            if (code is null)
            {
                await JS.InvokeVoidAsync("alert", "Load code before running");
                return (null, null);
            }
            var program = new RunningProgram(code.Workers[id], inputs);
            var output = new List<ObjectBase>();
            foreach ((long varId, long type) in Enumerable.Zip(code.Workers[id].Outputs, code.Workers[id].OutputsTypes))
            {
                switch (code.Types[type].Type)
                {
                    case TypeType.PROMISE:
                        {
                            long objId = GetNewObjectId();
                            objects[objId] = new LinkObjectPromise(code.Types[code.Types[type].Bases[0]]);
                            output.Add(program.Variables[varId] = new ObjectPromise(code.Types[code.Types[type].Bases[0]], objId));
                        }
                        break;
                    case TypeType.PIPE:
                        {
                            long objId = GetNewObjectId();
                            objects[objId] = new LinkObjectPipe(code.Types[code.Types[type].Bases[0]]);
                            output.Add(program.Variables[varId] = new ObjectPipe(code.Types[code.Types[type].Bases[0]], objId));
                        }
                        break;
                    default:
                        throw new Exception("Error: output type isn't promise or pipe");
                }
            }
            if (program != null)
            {
                    runningPrograms.Enqueue(program);
            }
            return (program, output.ToArray());
        }

        private async Task<ResultCode> RunWorker(RunningProgram program)
        {
            if (i64Type is null || byteType is null || code is null)
            {
                await JS.InvokeVoidAsync("alert", "i64 or byte type is not found, or code is null");
                return ResultCode.ERROR;
            }

            while (program.Ip < program.Worker.Code.Count)
            {
                Opcode opcode = program.Worker.Code[(int)program.Ip];

                // await JS.InvokeVoidAsync("alert", $"Operator {opcode.Type} [args {string.Join(" ", opcode.Data)}]");

                switch (opcode.Type)
                {
                    case OpcodeType.LOAD_INPUT:
                        {
                            long varId = opcode.Data[0];
                            long inputId = opcode.Data[1];
                            program.Variables[varId] = program.Inputs[(int)inputId];
                            program.Ip++;
                        }
                        break;
                    case OpcodeType.VARIABLE:
                        {
                            long tmpId = opcode.Data[0];
                            long varId = opcode.Data[1];
                            program.Temp[tmpId] = program.Variables[varId];
                            program.Ip++;
                        }
                        break;
                    case OpcodeType.LOAD_INT:
                        {
                            long tmpId = opcode.Data[0];
                            long value = opcode.Data[1];
                            program.Temp[tmpId] = new ObjectScalar(i64Type, BitConverter.GetBytes(value));
                            program.Ip++;
                        }
                        break;
                    case OpcodeType.LOAD_STRING:
                        {
                            long destId = opcode.Data[0];
                            long stringId = opcode.Data[1];
                            string str = program.Worker.Strings[stringId];
                            ObjectScalar[] data = System.Text.Encoding.UTF8.GetBytes(str).Select(x => new ObjectScalar(byteType, [x])).ToArray();

                            long objId = GetNewObjectId();
                            objects[objId] = new LinkObjectArray(byteType, data);
                            program.Temp[destId] = new ObjectArray(byteType, objId);
                        }
                        break;
                    case OpcodeType.LOAD_ARRAY:
                        {
                            long destId = opcode.Data[0];
                            long typeId = opcode.Data[1];
                            long sizeId = opcode.Data[2];
                            if (program.Temp[sizeId] is ObjectScalar os)
                            {
                                byte[] temp = new byte[8];
                                Buffer.BlockCopy(os.Value, 0, temp, 0, os.Value.Length);
                                long size = BitConverter.ToInt64(temp);
                                if (code.Types[typeId].Type == TypeType.SCALAR)
                                {
                                    long itemSize = code.Types[typeId].Bases[0];
                                    ObjectScalar[] data = Enumerable.Range(1, (int)size).Select(x => new ObjectScalar(code.Types[typeId], new byte[itemSize])).ToArray();

                                    long objId = GetNewObjectId();
                                    objects[objId] = new LinkObjectArray(code.Types[typeId], data);
                                    program.Temp[destId] = new ObjectArray(code.Types[typeId], objId);
                                }
                                else
                                {
                                    throw new NotImplementedException();
                                    // program.Temp[destId] = new ObjectArray(code.Types[typeId], []);
                                }
                            }
                            program.Ip++;
                        }
                        break;
                    case OpcodeType.PUSH_ARRAY_INDEX:
                        {
                            long destId = opcode.Data[0];
                            long indexId = opcode.Data[1];
                            long valueId = opcode.Data[2];
                            if (program.Temp[destId] is ObjectArray oa)
                            {
                                if (program.Temp[indexId] is ObjectScalar os)
                                {
                                    byte[] temp = new byte[8];
                                    Buffer.BlockCopy(os.Value, 0, temp, 0, os.Value.Length);
                                    long index = BitConverter.ToInt64(temp);

                                    /* call method to set by index */
                                    await SetObject(oa.Id, index, program.Temp[valueId]);
                                }
                            }
                            program.Ip++;
                        }
                        break;
                    case OpcodeType.ARRAY_INDEX:
                        {
                            long destId = opcode.Data[0];
                            long arrayId = opcode.Data[1];
                            long indexId = opcode.Data[2];
                            if (program.Temp[arrayId] is ObjectArray oa)
                            {
                                if (program.Temp[indexId] is ObjectScalar os)
                                {
                                    byte[] temp = new byte[8];
                                    Buffer.BlockCopy(os.Value, 0, temp, 0, os.Value.Length);
                                    long index = BitConverter.ToInt64(temp);

                                    /* call method to get by index */
                                    var res = program.AsyncResultValue ?? await GetObject(program, oa.Id, index);
                                    if (res is null)
                                    {
                                        return ResultCode.QUEUED;
                                    }
                                    program.AsyncResultValue = null;
                                    program.Temp[destId] = res;
                                }
                            }
                            program.Ip++;
                        }
                        break;
                    case OpcodeType.PUSH_VARIABLE:
                        {
                            long varId = opcode.Data[0];
                            long tempId = opcode.Data[1];
                            if (program.Variables.TryGetValue(varId, out var res) == false)
                            {
                                program.Variables[varId] = program.Temp[tempId];
                            }
                            else
                            {
                                switch (program.Variables[varId])
                                {
                                    case ObjectScalar os:
                                        program.Variables[varId] = program.Temp[tempId];
                                        break;
                                    case ObjectArray oa:
                                        program.Variables[varId] = program.Temp[tempId];
                                        break;
                                    case ObjectPromise ops:
                                        /* call method to set by index */
                                        await SetObject(ops.Id, 0, program.Temp[tempId]);
                                        break;
                                    case ObjectPipe ope:
                                        /* call method to set by index */
                                        await SetObject(ope.Id, 0, program.Temp[tempId]);
                                        break;
                                }
                            }
                            program.Ip++;
                        }
                        break;
                    case OpcodeType.ADD: await ArithmeticOpcode(program, opcode, (x, y) => x + y); break;
                    case OpcodeType.SUB: await ArithmeticOpcode(program, opcode, (x, y) => x - y); break;
                    case OpcodeType.MUL: await ArithmeticOpcode(program, opcode, (x, y) => x * y); break;
                    case OpcodeType.DIV: await ArithmeticOpcode(program, opcode, (x, y) => x / y); break;
                    case OpcodeType.MOD: await ArithmeticOpcode(program, opcode, (x, y) => x % y); break;
                    case OpcodeType.BAND: await ArithmeticOpcode(program, opcode, (x, y) => x & y); break;
                    case OpcodeType.BOR: await ArithmeticOpcode(program, opcode, (x, y) => x | y); break;
                    case OpcodeType.BXOR: await ArithmeticOpcode(program, opcode, (x, y) => x ^ y); break;
                    case OpcodeType.AND: await ArithmeticOpcode(program, opcode, (x, y) => (x != 0) && (y != 0) ? -1 : 0); break;
                    case OpcodeType.OR: await ArithmeticOpcode(program, opcode, (x, y) => (x != 0) || (y != 0) ? -1 : 0); break;
                    case OpcodeType.XOR: await ArithmeticOpcode(program, opcode, (x, y) => (x != 0) ^ (y != 0) ? -1 : 0); break;
                    case OpcodeType.LT: await ArithmeticOpcode(program, opcode, (x, y) => x < y ? -1 : 0); break;
                    case OpcodeType.LE: await ArithmeticOpcode(program, opcode, (x, y) => x <= y ? -1 : 0); break;
                    case OpcodeType.GT: await ArithmeticOpcode(program, opcode, (x, y) => x > y ? -1 : 0); break;
                    case OpcodeType.GE: await ArithmeticOpcode(program, opcode, (x, y) => x >= y ? -1 : 0); break;
                    case OpcodeType.EQ: await ArithmeticOpcode(program, opcode, (x, y) => x == y ? -1 : 0); break;
                    case OpcodeType.NE: await ArithmeticOpcode(program, opcode, (x, y) => x != y ? -1 : 0); break;
                    case OpcodeType.NOT: await ArithmeticOpcode(program, opcode, (x) => x == 0 ? -1 : 0); break;
                    case OpcodeType.BNOT: await ArithmeticOpcode(program, opcode, (x) => ~x); break;
                    case OpcodeType.JMP:
                        {
                            long dest = opcode.Data[0];
                            program.Ip = dest;
                        }
                        break;
                    case OpcodeType.JNZ:
                        {
                            long dest = opcode.Data[0];
                            long guard = opcode.Data[1];
                            if (program.Temp[guard] is ObjectScalar scalar && scalar.Value.Any(x => x != 0))
                            {
                                program.Ip = dest;
                            }
                            else
                            {
                                program.Ip++;
                            }
                        }
                        break;
                    case OpcodeType.JZ:
                        {
                            long dest = opcode.Data[0];
                            long guard = opcode.Data[1];
                            if (program.Temp[guard] is ObjectScalar scalar && scalar.Value.All(x => x == 0))
                            {
                                program.Ip = dest;
                            }
                            else
                            {
                                program.Ip++;
                            }
                        }
                        break;
                    case OpcodeType.QUERY:
                        {
                            long destId = opcode.Data[0];
                            long objectId = opcode.Data[1];
                            switch (program.Temp[objectId])
                            {
                                case ObjectArray oa:
                                    {
                                        var res = program.AsyncResultValue ?? await GetObject(program, oa.Id, -1);
                                        if (res is null)
                                        {
                                            return ResultCode.QUEUED;
                                        }
                                        program.AsyncResultValue = null;
                                        program.Temp[destId] = res;
                                    }
                                    break;
                                case ObjectPromise ops:
                                    /* call method to get by index */
                                    {
                                        var res = program.AsyncResultValue ?? await GetObject(program, ops.Id, 0);
                                        if (res is null)
                                        {
                                            return ResultCode.QUEUED;
                                        }
                                        program.AsyncResultValue = null;
                                        program.Temp[destId] = res;
                                    }
                                    break;
                                case ObjectPipe ope:
                                    /* call method to get by index */
                                    {
                                        var res = program.AsyncResultValue ?? await GetObject(program, ope.Id, 0);
                                        if (res is null)
                                        {
                                            return ResultCode.QUEUED;
                                        }
                                        program.AsyncResultValue = null;
                                        program.Temp[destId] = res;
                                    }
                                    break;
                                default:
                                    throw new Exception("Error: QUERY of not array, promise or pipe");
                            }
                            program.Ip++;
                        }
                        break;
                    case OpcodeType.CALL:
                        {
                            /* read inputs */
                            long callId = opcode.Data[0];
                            long inputsCount = code.Workers[callId].Inputs.Length;
                            long outputsCount = code.Workers[callId].Outputs.Length;
                            var inputs = opcode.Data[1..(int)(1 + inputsCount)].Select(x => program.Temp[x]).ToArray();
                            var outputs = await PrepareWorker(callId, inputs);
                            if (outputs.Item1 is not null && outputs.outputs is not null)
                            {
                                /* call */
                                foreach ((var num, var value) in Enumerable.Zip(opcode.Data[(int)(1 + inputsCount)..], outputs.outputs))
                                {
                                    program.Variables[num] = value;
                                }
                            }
                            program.Ip++;
                        }
                        break;
                    default:
                        await JS.InvokeVoidAsync("alert", $"Unknown Opcode {opcode.Type}");
                        program.Ip++;
                        break;
                }

            }
            return ResultCode.OK;
        }

        private async Task ArithmeticOpcode(RunningProgram program, Opcode opcode, Func<long, long, long> operation)
        {
            long resId = opcode.Data[0];
            long aId = opcode.Data[1];
            long bId = opcode.Data[2];
            if (program.Temp[aId] is ObjectScalar a &&
                program.Temp[bId] is ObjectScalar b)
            {
                if (a.Value.Length == 8)
                {
                    long result = operation(BitConverter.ToInt64(a.Value), BitConverter.ToInt64(b.Value, 0));
                    program.Temp[resId] = new ObjectScalar(a.Type, BitConverter.GetBytes(result));
                }
                else if (a.Value.Length == 4)
                {
                    int result = (int)operation(BitConverter.ToInt32(a.Value), BitConverter.ToInt32(b.Value, 0));
                    program.Temp[resId] = new ObjectScalar(a.Type, BitConverter.GetBytes(result));
                }
                else if (a.Value.Length == 2)
                {
                    short result = (short)operation(BitConverter.ToInt16(a.Value), BitConverter.ToInt16(b.Value, 0));
                    program.Temp[resId] = new ObjectScalar(a.Type, BitConverter.GetBytes(result));
                }
                else if (a.Value.Length == 1)
                {
                    byte result = (byte)operation(a.Value[0], b.Value[0]);
                    program.Temp[resId] = new ObjectScalar(a.Type, [result]);
                }
            }
            else
            {
                await JS.InvokeVoidAsync("alert", $"Error! arithmeric operation with not number type [{program.Temp[aId]}, {program.Temp[bId]}]");
            }
            program.Ip++;
        }

        private async Task ArithmeticOpcode(RunningProgram program, Opcode opcode, Func<long, long> operation)
        {
            long resId = opcode.Data[0];
            long aId = opcode.Data[1];
            if (program.Temp[aId] is ObjectScalar a)
            {
                if (a.Value.Length == 8)
                {
                    long result = operation(BitConverter.ToInt64(a.Value));
                    program.Temp[resId] = new ObjectScalar(a.Type, BitConverter.GetBytes(result));
                }
                else if (a.Value.Length == 4)
                {
                    int result = (int)operation(BitConverter.ToInt32(a.Value));
                    program.Temp[resId] = new ObjectScalar(a.Type, BitConverter.GetBytes(result));
                }
                else if (a.Value.Length == 2)
                {
                    short result = (short)operation(BitConverter.ToInt16(a.Value));
                    program.Temp[resId] = new ObjectScalar(a.Type, BitConverter.GetBytes(result));
                }
                else if (a.Value.Length == 1)
                {
                    byte result = (byte)operation(a.Value[0]);
                    program.Temp[resId] = new ObjectScalar(a.Type, [result]);
                }
            }
            else
            {
                await JS.InvokeVoidAsync("alert", $"Error! arithmeric operation with not number type [{program.Temp[aId]}]");
            }
            program.Ip++;
        }
    }
}
