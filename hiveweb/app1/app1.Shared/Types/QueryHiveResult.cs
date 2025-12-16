using System;
using System.Collections.Generic;
using System.Text;

namespace app1.Shared.Types
{
    public record struct QueryHiveResult
    {
        public long ClientId { get; set; }
        public double Payload { get; set; }
        public double Potential { get; set; }

        public QueryHiveResult(long clientId, double payload, double potential)
        {
            ClientId = clientId;
            Payload = payload;
            Potential = potential;
        }
    }
}
