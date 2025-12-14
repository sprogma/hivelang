using app1.Shared.Types;
using System.Buffers.Text;
using System.Text.Json.Nodes;

namespace app1
{
    public class CodeConverter
    {
        public StatementBase BuildStatement(Dictionary<long, string> strings, JsonNode stmt)
        {
            StatementBase result;

            switch (stmt["type"]?.GetValue<string>())
            {
                case "block":
                    ;
                    break;
                case "expr":

                    break;
                case "loop":
                    if (stmt?["body"] is not JsonObject loopBodyObj)
                    {
                        throw new Exception("Error: loop's body is corrupted");
                    }
                    var guard = BuildExpression(strings, loopBodyObj);
                    var body = BuildCode(strings, loopBodyObj);
                    break;
                case "match":

                    break;
                case "call":

                    break;
                default:
                    throw new Exception("Error: unknown statement type");
            }

            return result;
        }
        public CodeBlock BuildCode(Dictionary<long, string> strings, JsonNode code)
        {
            List<StatementBase> result = [];
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

            return new CodeBlock(vars, stmts.Where(x => x is not null).Cast<JsonNode>().Select(x => BuildStatement(strings, x)).ToArray());
        }

        public CodeWorker BuildWorker(long id, long[] inputs, long[] outputs, JsonNode code)
        {
            Dictionary<long, string> strings = [];
            return new CodeWorker(id, strings, inputs, outputs, BuildCode(strings, code));
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
                            string? name = value?["type"]?["scalar"]?.GetValue<string?>() ?? throw new Exception("Error: Corrupted scalar data");
                            long? size = value?["type"]?["size"]?.GetValue<long?>() ?? throw new Exception("Error: Corrupted scalar data");
                            result.Types[keyLong] = new TypeScalar(name, size.Value);
                        }
                        break;
                    case 1:
                        {
                            string? name = value?["type"]?["class"]?.GetValue<string?>() ?? throw new Exception("Error: Corrupted class data");
                            long[]? fields = value?["type"]?["fields"]?.GetValue<long[]?>() ?? throw new Exception("Error: Corrupted class data");
                            result.Types[keyLong] = new TypeClass(name, fields);
                        }
                        break;
                    case 2:
                        {
                            string? name = value?["type"]?["record"]?.GetValue<string?>() ?? throw new Exception("Error: Corrupted record data");
                            long[]? fields = value?["type"]?["fields"]?.GetValue<long[]?>() ?? throw new Exception("Error: Corrupted record data");
                            result.Types[keyLong] = new TypeRecord(name, fields);
                        }
                        break;
                    case 3:
                        {
                            long? @base = value?["type"]?["base"]?.GetValue<long?>() ?? throw new Exception("Error: Corrupted array data");
                            result.Types[keyLong] = new TypeArray(@base.Value);
                        }
                        break;
                    case 4:
                        {
                            long? @base = value?["type"]?["base"]?.GetValue<long?>() ?? throw new Exception("Error: Corrupted promise data");
                            result.Types[keyLong] = new TypePromise(@base.Value);
                        }
                        break;
                    case 5:
                        {
                            long? @base = value?["type"]?["base"]?.GetValue<long?>() ?? throw new Exception("Error: Corrupted pipe data");
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
                var inputs = value?["inputs"]?.GetValue<long[]>() ?? throw new Exception("Error: workers input isn't array of integers");
                var outputs = value?["outputs"]?.GetValue<long[]>() ?? throw new Exception("Error: workers output isn't array of integers");

                var code = value?["code"] ?? throw new Exception("Error: workers code doesn't exists");

                result.Workers[keyLong] = BuildWorker(keyLong, inputs, outputs, code);
            }

            return result;
        }
    }
}
