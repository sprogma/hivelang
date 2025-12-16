using app1;
using Microsoft.AspNetCore.SignalR;
using app1.Shared.Types;
namespace BlazorSignalRApp.Hubs;


public class HiveHub : Hub
{
    static private Dictionary<string, long> ConnectedUsers = [];

    private readonly ServerHive Server;

    public HiveHub(ServerHive Server)
    {
        this.Server = Server;
    }

    public override async Task OnConnectedAsync()
    {
        long newId = await Server.AddClient();
        Console.WriteLine($"Get event: Connect user id = {Context.ConnectionId} get id = {newId}");
        ConnectedUsers[Context.ConnectionId] = newId;
    }

    public override async Task OnDisconnectedAsync(Exception? exception)
    {
        if (ConnectedUsers.TryGetValue(Context.ConnectionId, out var clientId))
        {
            Console.WriteLine($"Get event: Disconnected user id = {Context.ConnectionId} of id = {clientId}");
            ConnectedUsers.Remove(Context.ConnectionId);
            await Server.RemoveClient(clientId);
        }
    }

    public async Task<CodeProgram> GetCode()
    {
        CodeProgram codeProgram = Server.GetCode();
        return codeProgram;
    }

    public async Task GetObject(long objectId, long parameter)
    {
        if (ConnectedUsers.TryGetValue(Context.ConnectionId, out long clientId))
        {
            Console.WriteLine($"Call GetObject by {clientId} object {objectId}, parameter={parameter}");

            /* get node with this object */
        }
    }

    public async Task<long> GetMyId()
    {
        return ConnectedUsers[Context.ConnectionId];
    }

    public async Task<QueryHiveResult[]> QueryHive()
    {
        if (ConnectedUsers.TryGetValue(Context.ConnectionId, out long clientId))
        {
            Console.WriteLine($"Call QueryHive by {clientId}");
            return await Server.Query(clientId);
        }
        return [];
    }

    public async Task GetQueryResults(QueryHiveResult result)
    {
        if (ConnectedUsers.TryGetValue(Context.ConnectionId, out long clientId))
        {
            Console.WriteLine($"Recieve GetQueryResults by {clientId} [data: {result}]");
            await Server.GetQueryResults(clientId, result);
        }
    }
}
