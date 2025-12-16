using app1.Shared.Types;
using System.ComponentModel;

namespace app1.Client.Runtime
{
    public class ObjectVisualizer
    {
        public static string GetObj(HiveRuntime hive, ObjectBase obj)
        {
            return obj switch
            {
                ObjectArray a => GetLinkedObj(hive, a.Id),
                ObjectPipe a => GetLinkedObj(hive, a.Id),
                ObjectPromise a => GetLinkedObj(hive, a.Id),
                ObjectScalar a => $"Scalar<{string.Join("-", a.Value)}>",
                _ => $"UnknownObject<{obj}>",
            };
        }

        public static string GetLinkedObj(HiveRuntime hive, long id)
        {
            if (!hive.objects.TryGetValue(id, out LinkObjectBase? value))
            {
                return $"RemoteObject<{id}>";
            }
            return value switch
            {
                LinkObjectArray a => $"[{string.Join(", ", a.Items.Select(x => GetObj(hive, x)))}]",
                LinkObjectPipe a => $"[{string.Join(", ", a.Pipe.Select(x => GetObj(hive, x)))}]",
                LinkObjectPromise a => $"[{string.Join(", ", a.Value is not null ? GetObj(hive, a.Value) : "null")}]",
                _ => $"UnknownLinkedObject<{value}>",
            };
        }
    }
}
