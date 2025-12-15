using System;
using System.Collections.Generic;
using System.Text;
using System.Text.Json.Serialization;

namespace app1.Shared.Types
{
    public class Variable(long type)
    {
        public long Type { get; set; } = type;
    }

    public class TypeBase
    {
        [JsonIgnore]
        public long CachedSize = 0;
    }

    public class TypeScalar(string name, long size) : TypeBase
    {
        public string Name { get; set; } = name;
        public long Size { get; set; } = size;
    }

    public class TypeArray(long basetype) : TypeBase
    {
        public long Base { get; set; } = basetype;
    }

    public class TypePipe(long basetype) : TypeBase
    {
        public long Base { get; set; } = basetype;
    }

    public class TypePromise(long basetype) : TypeBase
    {
        public long Base { get; set; } = basetype;
    }

    public class TypeClass(string name, long[] fields) : TypeBase
    {
        public string Name{ get; set; } = name;
        public long[] Fields { get; set; } = fields;
    }

    public class TypeRecord(string name, long[] fields) : TypeBase
    {
        public string Name{ get; set; } = name;
        public long[] Fields { get; set; } = fields;
    }

    public enum OpcodeType
    {
        LOAD_INPUT,

        LOAD_INT,
        LOAD_STRING,
        LOAD_ARRAY,
        ARRAY_INDEX,
        ARRAY_SET_INDEX,
        VARIABLE,
        QUERY,
        PUSH,

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

    public class CodeWorker(long id, long[] inputs, long[] outputs)
    {
        [JsonIgnore]
        public SortedSet<long> freeTemps = [];
        [JsonIgnore]
        public long allocatedTemps = 0;
        public long Id { get; set; } = id;
        public Dictionary<long, string> Strings { get; set; } = [];
        public long[] Inputs { get; set; } = inputs;
        public long[] Outputs { get; set; } = outputs;
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
        public Dictionary<long, TypeBase> Types { get; set; } = [];
        public Dictionary<long, CodeWorker> Workers { get; set; } = [];
    }
}
