# rynx
Rynx game engine tech stuff.

# ECS
The rynx ecs implementation seems to be decisively faster than EnTT, which I guess is somewhat of a standard as a level of quality.
Iterating over all entities that contain a single type T, EnTT is faster by some margin. But when the query gets more complex,
rynx ecs gets increasinly faster relative to EnTT. Below is a simple benchmark where data was synthesized such that there are four component types:
int, float, uint, double

Each entity has an independent 50% chance to have each component type. Since the data is very much synthesized and probably doesn't reflect any real world use case very well, you should not read too deeply into the results.

That said, the rynx implementation is much faster with this data already when querying for two component types at the same time. Also rynx ecs queries only get faster as the constraints for the query get more complex. This is in contrast to EnTT where more complex queries appear to take more time to execute even though there is less actual data that we want to process.

```-------------------------------------------------------------------------------
EnTT :: 50% random components
-------------------------------------------------------------------------------
...............................................................................

benchmark name                                  samples       iterations    estimated
                                                mean          low mean      high mean
                                                std dev       low std dev   high std dev
-------------------------------------------------------------------------------
query int                                               100           11    4.5914 ms
                                                   2.557 us     2.539 us     2.612 us
                                                     143 ns        52 ns       311 ns

query uint                                              100           12    4.6488 ms
                                                   2.548 us     2.537 us     2.575 us
                                                      85 ns        36 ns       144 ns

query float                                             100           13    4.7671 ms
                                                   2.523 us     2.505 us     2.574 us
                                                     139 ns        55 ns       299 ns

query int float                                         100            2    7.6332 ms
                                                  27.221 us    27.116 us    27.541 us
                                                     850 ns       322 ns     1.861 us

query float int                                         100            2    7.2344 ms
                                                   33.26 us    31.332 us     35.61 us
                                                   10.86 us     9.356 us    12.495 us

query float int                                         100            1    4.6982 ms
                                                  39.265 us      37.1 us    44.452 us
                                                  16.041 us     7.143 us    30.858 us

query float int uint32                                  100            1    6.3381 ms
                                                  37.457 us     36.23 us    41.384 us
                                                  10.096 us      3.71 us    22.297 us

query float int uint32 double                           100            1    5.5735 ms
                                                  82.095 us    71.762 us    94.219 us
                                                  57.018 us    49.845 us    67.004 us


-------------------------------------------------------------------------------
rynx :: 50% random components
-------------------------------------------------------------------------------
...............................................................................

benchmark name                                  samples       iterations    estimated
                                                mean          low mean      high mean
                                                std dev       low std dev   high std dev
-------------------------------------------------------------------------------
query int                                               100            7    5.0302 ms
                                                   9.039 us     8.866 us     9.174 us
                                                     776 ns       617 ns       936 ns

query uint                                              100            5     4.844 ms
                                                   6.764 us     6.697 us      6.92 us
                                                     490 ns       246 ns       895 ns

query float                                             100            7    5.2003 ms
                                                   7.371 us     6.987 us     8.756 us
                                                   3.283 us       913 ns     7.477 us

query int float                                         100           11    4.9863 ms
                                                   3.705 us     3.567 us     4.153 us
                                                   1.135 us       386 ns     2.534 us

query float int                                         100            9    4.8429 ms
                                                   3.508 us     3.496 us      3.55 us
                                                     102 ns        27 ns       229 ns

query float int uint32                                  100           14    4.7642 ms
                                                   1.865 us     1.832 us     1.949 us
                                                     242 ns        78 ns       473 ns

query float int uint32 double                           100           33    4.6002 ms
                                                   1.259 us     1.209 us     1.439 us
                                                     432 ns       122 ns       984 ns


===============================================================================
```

# Scheduler

In case you are uncertain about multithreading with an ecs implementation (I've heard that his turns out to be difficult sometimes), you should know that rynx offers a super friendly scheduler that understands ecs views and data, and will automatically ensure that there are no multithreading issues for read/write accesses to the same resources when used properly.
Essentially when you create all your multithreaded accesses to the ecs through the rynx scheduler contexts, and do not capture any data in your task lambdas but request all your resources as parameters, the scheduler will do the work for you.
`TODO: Offer an example :)`
