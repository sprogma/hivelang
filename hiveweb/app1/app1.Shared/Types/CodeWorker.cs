using System;
using System.Collections.Generic;
using System.Runtime.Serialization;
using System.Text;
using System.Text.Json.Serialization;
using System.Text.Json.Serialization.Metadata;

namespace app1.Shared.Types
{
    public class Variable(long type)
    {
        public long Type { get; set; } = type;
    }


    public enum TypeType
    {
        SCALAR,
        CLASS,
        RECORD,
        ARRAY,
        PIPE,
        PROMISE,
    }

    public class VarType(TypeType type, string name, long[] bases)
    {
        [JsonIgnore]
        public long CachedSize = 0;
        public TypeType Type { get; set; } = type;
        public string Name { get; set; } = name;
        public long[] Bases { get; set; } = bases;
    }

    public enum OpcodeType
    {
        LOAD_INPUT,

        LOAD_INT,
        LOAD_STRING,
        LOAD_ARRAY,
        ARRAY_INDEX,
        VARIABLE,
        QUERY,

        PUSH_ARRAY_INDEX,
        PUSH_VARIABLE,

        // operators
        ADD, SUB, MUL, DIV, MOD,

        AND, NOT, OR, XOR,

        BAND, BNOT, BOR, BXOR,

        LT, LE, GT, GE, EQ, NE,

        JMP, JZ, JNZ,

        CALL,
    }

    public class Opcode(OpcodeType type, long[] data)
    {
        public OpcodeType Type { get; set; } = type;
        public long[] Data { get; set; } = data;
    }

    public class CodeWorker(long id, long[] inputs, (long, VarType)[] outputs)
    {
        [JsonIgnore]
        public SortedSet<long> freeTemps = [];
        [JsonIgnore]
        public long allocatedTemps = 0;
        public long Id { get; set; } = id;
        public Dictionary<long, string> Strings { get; set; } = [];
        public long[] Inputs { get; set; } = inputs;
        public (long, VarType)[] Outputs { get; set; } = outputs;
        public List<Opcode> Code { get; set; } = [];

        public long GetTemp()
        {
            if (freeTemps.Count == 0)
            {
                return allocatedTemps++;
            }
            long res = freeTemps.First();
            freeTemps.Remove(res);
            return res;
        }

        public void FreeTemp(long tmp)
        {
            freeTemps.Add(tmp);
        }
    }

    public class CodeProgram()
    {
        public Dictionary<long, VarType> Types { get; set; } = [];
        public Dictionary<long, CodeWorker> Workers { get; set; } = [];
    }


    public class ObjectBase(VarType type)
    {
        [JsonIgnore]
        public VarType Type = type;
    }

    public class ObjectScalar(VarType type, byte[] value) : ObjectBase(type)
    {
        public byte[] Value { get; set; } = value;

        public override string ToString()
        {
            return $"ObjectScalar({string.Join("-", Value)})";
        }
    }

    public class ObjectPipe(VarType type) : ObjectBase(type)
    {
        public Queue<ObjectBase> Pipe { get; set; } = [];
        public override string ToString()
        {
            return $"ObjectArray(<{string.Join(", ", Pipe)}>)";
        }
    }

    public class ObjectPromise(VarType type) : ObjectBase(type)
    {
        public ObjectBase? Value { get; set; }

        public override string ToString()
        {
            return $"ObjectPromise({Value?.ToString() ?? "null"})";
        }
    }

    public class ObjectArray(VarType type, ObjectBase[] items) : ObjectBase(type)
    {
        public ObjectBase[] Items { get; set; } = items;

        public override string ToString()
        {
            return $"ObjectArray([{string.Join(", ", Items)}])";
        }
    }

    public enum ResultCode
    {
        ERROR,
        HANG,
        OK,
    }

    public class RunningProgram(CodeWorker worker, ObjectBase[] inputs)
    {
        [JsonIgnore]
        public CodeWorker Worker = worker;
        public long Ip {get; set;} = 0;
        public ObjectBase[] Inputs { get; set; } = inputs;
        public Dictionary<long, ObjectBase> Objects { get; set; } = [];
        public Dictionary<long, ObjectBase> Variables { get; set; } = [];
        public Dictionary<long, ObjectBase> Temp { get; set; } = [];
    }
}
