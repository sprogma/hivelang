using app1.Shared.Types;
using System.Buffers.Text;
using System.Linq.Expressions;
using System.Text.Json.Nodes;

namespace app1
{
    public class CodeConverter
    {
        public long[] ExtractLongArray(JsonNode? node)
        {
            if (node is null)
            {
                throw new Exception("Error: excepted long array");
            }
            return node?.AsArray().Select(node => node?.GetValue<long>()).Where(x => x is not null).Cast<long>().ToArray() ?? throw new Exception("Error: excepted long array");
        }

        public long BuildExpression(CodeWorker worker, JsonObject expr)
        {
            if (expr["int"]?.GetValue<long>() is long value)
            {
                long resultTemp = worker.GetTemp();
                worker.Code.Add(new Opcode(OpcodeType.LOAD_INT, [resultTemp, value]));
                return resultTemp;
            }
            else if (expr["string"]?.GetValue<string>() is string stringValue)
            {
                long resultTemp = worker.GetTemp();
                long stringKey = worker.Strings.Count;
                worker.Strings[stringKey] = stringValue;
                worker.Code.Add(new Opcode(OpcodeType.LOAD_STRING, [resultTemp, stringKey]));
                return resultTemp;
            }
            else if (expr.ContainsKey("array"))
            {
                long typeId = expr["type"]?.GetValue<long>() ?? throw new Exception("Error: wrong array expresion");
                if (expr["size"] is not JsonObject size)
                {
                    throw new Exception("Error: wrong index expresion");
                }
                long sizeTemp = BuildExpression(worker, size);
                worker.Code.Add(new Opcode(OpcodeType.LOAD_ARRAY, [sizeTemp, typeId, sizeTemp]));
                return sizeTemp;
            }
            else if (expr.ContainsKey("index"))
            {
                if (expr["from"] is not JsonObject from)
                {
                    throw new Exception("Error: wrong index expresion");
                }
                if (expr["at"] is not JsonObject at)
                {
                    throw new Exception("Error: wrong index expresion");
                }
                long fromId = BuildExpression(worker, from);
                long atId = BuildExpression(worker, at);
                worker.Code.Add(new Opcode(OpcodeType.ARRAY_INDEX, [fromId, fromId, atId]));
                worker.FreeTemp(atId);
                return fromId;
            }
            else if (expr["variable"]?.GetValue<long>() is long variableId)
            {
                long resultTemp = worker.GetTemp();
                worker.Code.Add(new Opcode(OpcodeType.VARIABLE, [resultTemp, variableId]));
                return resultTemp;
            }
            else if (expr["query"] is JsonObject queryObj)
            {
                long subexpr = BuildExpression(worker, queryObj);
                worker.Code.Add(new Opcode(OpcodeType.QUERY, [subexpr, subexpr]));
                return subexpr;
            }
            else if (expr.ContainsKey("push"))
            {
                if (expr["to"] is not JsonObject to)
                {
                    throw new Exception("Error: wrong push expresion");
                }
                if (expr["from"] is not JsonObject from)
                {
                    throw new Exception("Error: wrong push expresion");
                }
                long fromId = BuildExpression(worker, from);
                if (to.ContainsKey("variable"))
                {
                    long varId = to["variable"]?.GetValue<long>() ?? throw new Exception("Error: wrong push to variable");
                    worker.Code.Add(new Opcode(OpcodeType.PUSH, [varId, fromId]));
                    return fromId;
                }
                else if (to.ContainsKey("index"))
                {
                    if (to["from"] is not JsonObject arrayFrom)
                    {
                        throw new Exception("Error: wrong index expresion");
                    }
                    if (to["at"] is not JsonObject arrayAt)
                    {
                        throw new Exception("Error: wrong index expresion");
                    }
                    long arrayFromId = BuildExpression(worker, arrayFrom);
                    long arrayAtId = BuildExpression(worker, arrayAt);
                    worker.Code.Add(new Opcode(OpcodeType.ARRAY_SET_INDEX, [arrayFromId, arrayAtId, fromId]));
                    worker.FreeTemp(arrayAtId);
                    return fromId;
                }
                else
                {
                    throw new NotImplementedException();
                }
            }
            else if (expr.ContainsKey("op"))
            {
                string type = expr["op"]?.GetValue<string>() ?? throw new Exception("Error: Wrong operator");
                if (expr["childs"] is not JsonArray childs)
                {
                    throw new Exception("Error: no childs in operator");
                }
                JsonObject[] childsObj = childs.Where(x => x is JsonObject).Cast<JsonObject>().ToArray();
                long[] childsTemps = childsObj.Select(x => BuildExpression(worker, x)).ToArray();
                var binOps = new Dictionary<string, OpcodeType>
                {
                    ["add"] = OpcodeType.ADD,
                    ["sub"] = OpcodeType.SUB,
                    ["mul"] = OpcodeType.MUL,
                    ["div"] = OpcodeType.DIV,
                    ["mod"] = OpcodeType.MOD,
                    ["and"] = OpcodeType.AND,
                    ["or"] = OpcodeType.OR,
                    ["xor"] = OpcodeType.XOR,
                    ["band"] = OpcodeType.BAND,
                    ["bor"] = OpcodeType.BOR,
                    ["bxor"] = OpcodeType.BXOR,
                    ["lt"] = OpcodeType.LT,
                    ["le"] = OpcodeType.LE,
                    ["gt"] = OpcodeType.GT,
                    ["ge"] = OpcodeType.GE,
                    ["eq"] = OpcodeType.EQ,
                    ["ne"] = OpcodeType.NE,
                };
                if (binOps.TryGetValue(type, out OpcodeType bOpCode) && childsTemps.Length == 2)
                {
                    worker.Code.Add(new Opcode(bOpCode, [childsTemps[0], childsTemps[0], childsTemps[1]]));
                    worker.FreeTemp(childsTemps[1]);
                    return childsTemps[0];
                }
                var unaryOps = new Dictionary<string, OpcodeType>
                {
                    ["not"] = OpcodeType.NOT,
                    ["bnot"] = OpcodeType.BNOT,
                };
                if (unaryOps.TryGetValue(type, out OpcodeType uOpCode) && childsTemps.Length == 1)
                {
                    worker.Code.Add(new Opcode(bOpCode, [childsTemps[0], childsTemps[0]]));
                    return childsTemps[0];
                }
                throw new Exception($"Error: Unknown operation type <{type}>");
            }
            throw new Exception($"Error: Unknown expression type");
        }

        public void BuildStatement(CodeWorker worker, JsonObject stmt)
        {
            switch (stmt["type"]?.GetValue<string>())
            {
                case "block":
                    if (stmt?["block"] is not JsonObject blockObj)
                    {
                        throw new Exception("Error: block's block is corrupted");
                    }
                    BuildCode(worker, blockObj);
                    break;

                case "expr":
                    if (stmt?["expr"] is not JsonObject exprObj)
                    {
                        throw new Exception("Error: expr's expr is corrupted");
                    }
                    long exprRes = BuildExpression(worker, exprObj);
                    worker.FreeTemp(exprRes);
                    break;

                case "loop":
                    if (stmt?["body"] is not JsonObject loopBodyObj)
                    {
                        throw new Exception("Error: loop's body is corrupted");
                    }
                    if (stmt?["guard"] is not JsonObject loopGuardObj)
                    {
                        throw new Exception("Error: guard's body is corrupted");
                    }

                    int curPos = worker.Code.Count;
                    worker.Code.Add(new Opcode(OpcodeType.JMP, [0]));

                    BuildCode(worker, loopBodyObj);

                    worker.Code[curPos].Data[0] = worker.Code.Count + 1;

                    long loopGuard = BuildExpression(worker, loopGuardObj);

                    worker.Code.Add(new Opcode(OpcodeType.JNZ, [curPos, loopGuard]));

                    worker.FreeTemp(loopGuard);
                    break;

                case "match":
                    if (stmt?["match"] is not JsonObject matchExprObj)
                    {
                        throw new Exception("Error: match's expression is corrupted");
                    }

                    long matchSubexpr = BuildExpression(worker, matchExprObj);

                    if (stmt?["cases"] is not JsonArray matchesArray)
                    {
                        throw new Exception("Error: match's cases is corrupted");
                    }

                    foreach (var var in matchesArray)
                    {
                        if (var is not JsonObject obj)
                        {
                            throw new Exception("Error: one of cases is corrupted");
                        }
                        bool isDefault = obj["default"]?.GetValue<bool>() ?? throw new Exception("Error: Default key is corrupted");
                        long resultVar;
                        List<long> fixups = [];
                        if (isDefault)
                        {
                            resultVar = obj["var"]?.GetValue<long>() ?? throw new Exception("Error: Default variable key is corrupted");
                        }
                        else
                        {
                            if (var["case"] is not JsonObject caseObj)
                            {
                                throw new Exception("Error: case key not found");
                            }
                            resultVar = BuildExpression(worker, caseObj);

                            long temp = worker.GetTemp();
                            worker.Code.Add(new Opcode(OpcodeType.EQ, [temp, matchSubexpr, resultVar]));
                            fixups.Add(worker.Code.Count);
                            worker.Code.Add(new Opcode(OpcodeType.JNZ, [0, temp]));
                            worker.FreeTemp(resultVar);
                            worker.FreeTemp(temp);
                        }

                        /* add guard, if it is */
                        if (var["guard"] is JsonObject guardObj)
                        {
                            long guard = BuildExpression(worker, guardObj);
                            worker.Code.Add(new Opcode(OpcodeType.JNZ, [0, guard]));
                            worker.FreeTemp(guard);
                        }

                        if (var["body"] is not JsonObject caseBodyObj)
                        {
                            throw new Exception("Error: case body is corrupted");
                        }

                        /* build body */
                        BuildCode(worker, caseBodyObj);

                        /* skip, if it is false */
                        foreach (var pos in fixups)
                        {
                            worker.Code[(int)pos].Data[0] = worker.Code.Count;
                        }
                    }
                    break;

                case "call":
                    long workerId = stmt?["worker"]?.GetValue<long?>() ?? throw new Exception("Error: call worker wrong id");
                    if (stmt?["inputs"] is not JsonArray callInputArr)
                    {
                        throw new Exception("Error: call's input array is corrupted");
                    }
                    var inputsTmps = new long[callInputArr.Count];
                    for (long ind = 0; ind < callInputArr.Count; ++ind)
                    {
                        if (callInputArr[(int)ind] is not JsonObject inputObj)
                        {
                            throw new Exception("Error: call's input is corrupted");
                        }
                        inputsTmps[ind] = BuildExpression(worker, inputObj);
                    }
                    long[] outputs = ExtractLongArray(stmt?["outputs"]);

                    worker.Code.Add(new Opcode(OpcodeType.CALL, [.. inputsTmps, .. outputs]));

                    foreach (var id in inputsTmps)
                    {
                        worker.FreeTemp(id);
                    }

                    break;

                case "break":
                    /* do nothing */
                    break;

                default:
                    throw new Exception("Error: unknown statement type");
            }
        }
        public void BuildCode(CodeWorker worker, JsonObject code)
        {
            Dictionary<long, Variable> vars = [];

            if (code["locals"] is not JsonObject locals)
            {
                throw new Exception("Error: locals variables key is corrupted");
            }

            /* read variables */
            foreach ((var key, var value) in locals)
            {
                if (!long.TryParse(key, out var keyLong))
                {
                    throw new Exception("Error: variable key isn't integer");
                }

                vars[keyLong] = new Variable(value?.GetValue<long>() ?? throw new Exception("Variable type isn't integer"));
            }

            /* read code */
            if (code["code"] is not JsonArray stmts)
            {
                throw new Exception("Error: code statements key is corrupted");
            }

            foreach (var stmt in stmts.Where(x => x is JsonObject).Cast<JsonObject>())
            {
                BuildStatement(worker, stmt);
            }
        }

        public CodeWorker BuildWorker(long id, long[] inputs, long[] outputs, JsonObject code)
        {
            var result = new CodeWorker(id, inputs, outputs);
            /* get code to load inputs */
            for (int i = 0; i < inputs.Length; i++)
            {
                result.Code.Add(new Opcode(OpcodeType.LOAD_INPUT, [inputs[i], i]));
            }
            BuildCode(result, code);
            return result;
        }

        public CodeProgram BuildCode(string JsonData)
        {
            var data = JsonNode.Parse(JsonData)?.AsObject()
                       ?? throw new Exception("Json parsing failed.");

            var result = new CodeProgram();

            if (data["types"] is not JsonObject typesObj)
            {
                throw new Exception("Error: types key isn't object");
            }

            foreach ((var key, var value) in typesObj)
            {
                if (!long.TryParse(key, out var keyLong))
                {
                    throw new Exception("Error: types key isn't integer");
                }

                /* parse value */
                switch (value?["type"]?.GetValue<long>())
                {
                    case 0:
                        {
                            string? name = value?["scalar"]?.GetValue<string?>() ?? throw new Exception("Error: Corrupted scalar data");
                            long? size = value?["size"]?.GetValue<long?>() ?? throw new Exception("Error: Corrupted scalar data");
                            result.Types[keyLong] = new TypeScalar(name, size.Value);
                        }
                        break;
                    case 1:
                        {
                            string? name = value?["class"]?.GetValue<string?>() ?? throw new Exception("Error: Corrupted class data");
                            long[]? fields = ExtractLongArray(value?["fields"]);
                            result.Types[keyLong] = new TypeClass(name, fields);
                        }
                        break;
                    case 2:
                        {
                            string? name = value?["record"]?.GetValue<string?>() ?? throw new Exception("Error: Corrupted record data");
                            long[]? fields = ExtractLongArray(value?["fields"]);
                            result.Types[keyLong] = new TypeRecord(name, fields);
                        }
                        break;
                    case 3:
                        {
                            long? @base = value?["base"]?.GetValue<long?>() ?? throw new Exception("Error: Corrupted array data");
                            result.Types[keyLong] = new TypeArray(@base.Value);
                        }
                        break;
                    case 4:
                        {
                            long? @base = value?["base"]?.GetValue<long?>() ?? throw new Exception("Error: Corrupted promise data");
                            result.Types[keyLong] = new TypePromise(@base.Value);
                        }
                        break;
                    case 5:
                        {
                            long? @base = value?["base"]?.GetValue<long?>() ?? throw new Exception("Error: Corrupted pipe data");
                            result.Types[keyLong] = new TypePipe(@base.Value);
                        }
                        break;
                    default:
                        throw new Exception($"Error: types[{key}].type isn't enum value");
                }
            }


            if (data["workers"] is not JsonObject workersObj)
            {
                throw new Exception("Error: workers key isn't object");
            }

            foreach ((var key, var value) in workersObj)
            {
                if (!long.TryParse(key, out var keyLong))
                {
                    throw new Exception("Error: workers key isn't integer");
                }

                /* read worker */
                var inputs = ExtractLongArray(value?["inputs"]);
                var outputs = ExtractLongArray(value?["outputs"]);

                if (value?["code"] is not JsonObject code)
                {
                    throw new Exception("Error: workers code doesn't exists");
                }

                result.Workers[keyLong] = BuildWorker(keyLong, inputs, outputs, code);
            }

            return result;
        }
    }
}
