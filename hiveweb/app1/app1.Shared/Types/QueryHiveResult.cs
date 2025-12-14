using System;
using System.Collections.Generic;
using System.Text;

namespace app1.Shared.Types
{
    public record struct QueryHiveResult
    {
        public ulong ClientId { get; set; }
        public double Payload { get; set; }
        public double Potential { get; set; }

        public QueryHiveResult(ulong clientId, double payload, double potential)
        {
            ClientId = clientId;
            Payload = payload;
            Potential = potential;
        }
    }
}
