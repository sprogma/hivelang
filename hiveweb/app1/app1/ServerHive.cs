using BlazorSignalRApp.Hubs;
using Microsoft.AspNetCore.SignalR;
using app1.Shared.Types;

namespace app1
{
    public class ClientHiveInfo
    {
        public ulong id;

        public double Payload { get; private set; }
        public double Potential { get; private set; }

        public ClientHiveInfo(ulong id)
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

        private Dictionary<ulong, ClientHiveInfo> clients = [];
        private ulong nextClientId;

        public ServerHive(IHubContext<HiveHub> hubContext)
        {
            this.hubContext = hubContext;
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
        public async Task GetQueryResults(ulong clientId, QueryHiveResult result)
        {
            lock (clients)
            {
                if (clients.TryGetValue(clientId, out ClientHiveInfo? value))
                {
                    value.UpdateUsing(result);
                }
            }
        }

        public async Task RemoveClient(ulong id)
        {
            clients.Remove(id);
        }

        public async Task<ulong> AddClient()
        {
            ulong newId;

            lock (clients)
            {
                newId = nextClientId++;
                clients[newId] = new(newId);
            }

            return newId;
        }

        public async Task<QueryHiveResult[]> Query(ulong excludeClientId)
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
