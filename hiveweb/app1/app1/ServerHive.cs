using BlazorSignalRApp.Hubs;
using Microsoft.AspNetCore.SignalR;
using app1.Shared.Types;

namespace app1
{
    public class ClientHiveInfo
    {
        public long id;

        public double Payload { get; private set; }
        public double Potential { get; private set; }

        public ClientHiveInfo(long id)
        {
            this.id = id;
        }

        public void UpdateUsing(QueryHiveResult result)
        {
            this.Payload = result.Payload;
            this.Potential = result.Potential;
        }
    }

    public class ServerHive : IHostedService, IDisposable
    {
        private readonly IHubContext<HiveHub> hubContext;
        private CancellationTokenSource? runToken;
        private Task? backgroundLoopTask;
        private CodeProgram program;

        private Dictionary<long, ClientHiveInfo> clients = [];
        private long nextClientId;

        public ServerHive(IHubContext<HiveHub> hubContext, CodeProgram program)
        {
            this.hubContext = hubContext;
            this.program = program;
        }

        public Task StartAsync(CancellationToken cancellationToken)
        {
            Console.WriteLine("Staring ServerHive service...");

            runToken = CancellationTokenSource.CreateLinkedTokenSource(cancellationToken);
            backgroundLoopTask = Task.Run(() => HiveLoopAsync(runToken.Token), CancellationToken.None);

            return Task.CompletedTask;
        }

        public async Task StopAsync(CancellationToken cancellationToken)
        {
            runToken?.Cancel();
            if (backgroundLoopTask != null)
            {
                try
                {
                    await Task.WhenAny(backgroundLoopTask, Task.Delay(Timeout.Infinite, cancellationToken));
                }
                catch (OperationCanceledException) { }
            }
            runToken?.Dispose();
            Console.WriteLine("Stopped ServerHive service.");
        }

        public void Dispose()
        {
            runToken?.Dispose();
        }

        public async Task HiveLoopAsync(CancellationToken runToken)
        {
            while (!runToken.IsCancellationRequested)
            {
                Console.WriteLine("Polling...");
                await hubContext.Clients.All.SendAsync("QueryHive", runToken);
                await Task.Delay(3000, runToken);
            }
        }
        public async Task GetQueryResults(long clientId, QueryHiveResult result)
        {
            lock (clients)
            {
                if (clients.TryGetValue(clientId, out ClientHiveInfo? value))
                {
                    value.UpdateUsing(result);
                }
            }
        }

        public async Task RemoveClient(long id)
        {
            clients.Remove(id);
        }

        public async Task<long> AddClient()
        {
            long newId;

            lock (clients)
            {
                newId = nextClientId++;
                clients[newId] = new(newId);
            }

            return newId;
        }

        public CodeProgram GetCode()
        {
            return program;
        }

        public async Task<QueryHiveResult[]> Query(long excludeClientId)
        {
            lock (clients)
            {
                return clients.Values
                              .Where(x => x.id != excludeClientId)
                              .Select(x => new QueryHiveResult(x.id, x.Payload, x.Potential))
                              .ToArray();
            }
        }
    }
}
