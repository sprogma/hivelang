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
        VARIABLE,
        QUERY,
        PUSH,

        // operators
        ADD, SUB, MUL, DIV, MOD,

        AND, NOT, OR, XOR,

        BAND, BNOT, BOR, BXOR,

        LT, LE, GT, GE, EQ, NE,
    }

    public class Opcode(OpcodeType type, long[] data)
    {
        public OpcodeType Type { get; set; } = type;
        public long[] Data { get; set; } = data;
    }

    public class Expression(Opcode[] code, long? result)
    {
        public Opcode[] Code { get; set; } = code;
        public long? Result { get; set; } = result;
    }

    public class StatementBase
    { }

    public class StatementBlock(CodeBlock block) : StatementBase
    {
        public CodeBlock Block { get; set; } = block;
    }

    public class StatementLoop(CodeBlock body, Expression guard) : StatementBase
    {
        public CodeBlock Body { get; set; } = body;
        public Expression Guard { get; set; } = guard;
    }

    public class StatementExpression(Expression expression) : StatementBase
    {
        public Expression Expression { get; set; } = expression;
    }

    public class StatementCall(long worker, long inputs, long outputs) : StatementBase
    {
        public long Worker { get; set; } = worker;
        public long Inputs { get; set; } = inputs;
        public long Outputs { get; set; } = outputs;
    }

    public class StatementBreak : StatementBase { }

    public class StatementMatch(Expression expression, long?[] defaults, Expression[] literals, CodeBlock[] blocks) : StatementBase
    {
        public Expression Expression { get; set; } = expression;
        public long?[] Defaults { get; set; } = defaults;
        public Expression[] Literals { get; set; } = literals;
        public CodeBlock[] Blocks { get; set; } = blocks;
    }

    public class CodeBlock(Dictionary<long, Variable> vars, StatementBase[] code)
    {
        public Dictionary<long, Variable> Vars { get; set; } = vars;
        public StatementBase[] Code { get; set; } = code;
    }

    public class CodeWorker(long id, Dictionary<long, string> strings, long[] inputs, long[] outputs, CodeBlock body)
    {
        public long Id { get; set; } = id;
        public Dictionary<long, string> Strings { get; set; } = strings;
        public long[] Inputs { get; set; } = inputs;
        public long[] Outputs { get; set; } = outputs;
        public CodeBlock Body { get; set; } = body;
    }

    public class CodeProgram()
    {
        public Dictionary<long, TypeBase> Types { get; set; } = [];
        public Dictionary<long, CodeWorker> Workers { get; set; } = [];
    }
}
