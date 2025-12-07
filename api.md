# This file describes basic hive functinoal

## Every hive must support this api: [api v1]

1. `RunWorker(int64 id, object[] input) -> none` \
    Run worker `id` on this hive using `input` as inputs
1. `TransferWorker(int64 id, object[] input, byte[] state?) -> none` \
    [maybe this is too hard? - one can not implement this]
1. `TransferObject(object object, byte[] data?) -> none` \
    Transfer object to this hive, using data.
    Only pointer types could be transfered, such as classes, arrays and pipes
    But if object is transfered, All it's subobjects aren't transfered automaticly.
    > **TODO**: Maybe this api entry will be promoted to transfer many of objects at same time
1. `GetObject(object object, int field / int index / bool query) -> object` \
    Used to get value on objects, belonging to this hive
1. `QueryHive() -> statistics of hive usage` \
    Used to distribute payload between all computers of network
1. `Connect(ipaddress/someelse) -> none` \
    Connect new hive as neibour
1. `...` \
    Some functions to disconnect hive?

## Also hive can

* Count accesses to objects, and transfer them, if remote accesses count from any domain is greater than local accesses
* Query other hives, and distribute payload

## Communication

* I think hives will communicate using many different ways, becouse I can't imagine way to use one api:
    Network hives must use json and can communicate only with their server, so we can't connect them to C hive.
    So, There will be many realization, and some of them will be compactable, some - not.
