# rynx
Rynx game engine tech stuff.

# ECS
The rynx ecs implementation seems to be decisively faster than EnTT, which I guess is somewhat of a standard as a level of quality.
Iterating over all entities that contain a single type `T`, EnTT is on par with rynx. But when the query gets more complex,
rynx ecs gets increasinly faster relative to EnTT. Below is a simple benchmark where data was synthesized such that there are four component types:
int, float, uint, double

Each entity has an independent 50% chance to have each component type. Since the data is very much synthesized and probably doesn't reflect any real world use case very well, you should not read too deeply into the results.

That said, the rynx implementation is much faster with this data already when querying for two component types at the same time. Also rynx ecs queries only get faster as the constraints for the query get more complex. This is in contrast to EnTT where more complex queries appear to take more time to execute even though there is less actual data that we want to process.

```
-------------------------------------------------------------------------------
EnTT :: 50% random components
-------------------------------------------------------------------------------
...............................................................................

benchmark name                                  samples       iterations    estimated
                                                mean          low mean      high mean
                                                std dev       low std dev   high std dev
-------------------------------------------------------------------------------
query int                                               100           20     4.676 ms
                                                   2.223 us     2.076 us     2.384 us
                                                     782 ns       725 ns       820 ns

query uint                                              100           22    4.5848 ms
                                                   5.432 us      5.33 us     5.526 us
                                                     500 ns       380 ns       646 ns

query float                                             100           15    4.5825 ms
                                                   2.524 us     2.498 us     2.574 us
                                                     172 ns        93 ns       268 ns

query int float                                         100            2    8.4494 ms
                                                   33.97 us    33.715 us    34.379 us
                                                   1.622 us     1.148 us     2.335 us

query float int                                         100            2     7.009 ms
                                                  26.727 us    26.488 us    27.385 us
                                                   1.847 us       800 ns     3.885 us

query float int uint32                                  100            2    6.8666 ms
                                                  28.501 us    27.659 us    29.731 us
                                                    5.14 us     3.809 us     6.622 us

query float int uint32 double                           100            1    4.9766 ms
                                                  64.758 us    57.311 us    73.472 us
                                                  40.877 us    36.039 us    46.748 us


-------------------------------------------------------------------------------
rynx :: 50% random components
-------------------------------------------------------------------------------
...............................................................................

benchmark name                                  samples       iterations    estimated
                                                mean          low mean      high mean
                                                std dev       low std dev   high std dev
-------------------------------------------------------------------------------
query int                                               100           18    4.4442 ms
                                                   2.196 us      2.19 us     2.221 us
                                                      56 ns        11 ns       131 ns

query uint                                              100           15    4.5195 ms
                                                    2.21 us     2.197 us     2.243 us
                                                      97 ns        23 ns       174 ns

query float                                             100           10     4.452 ms
                                                    3.01 us     2.996 us     3.044 us
                                                     103 ns        46 ns       203 ns

query int float                                         100           10     4.705 ms
                                                    3.59 us     3.578 us     3.621 us
                                                      87 ns        28 ns       183 ns

query float int                                         100           11    4.8653 ms
                                                   3.622 us     3.608 us     3.665 us
                                                     117 ns        47 ns       253 ns

query float int uint32                                  100           15    4.4715 ms
                                                   1.878 us     1.853 us      1.92 us
                                                     161 ns       106 ns       229 ns

query float int uint32 double                           100           37    4.4992 ms
                                                   1.054 us     1.047 us     1.072 us
                                                      47 ns         6 ns       102 ns


===============================================================================
```

# Scheduler

In case you are uncertain about multithreading with an ecs implementation (I've heard that his turns out to be difficult sometimes), you should know that rynx offers a super friendly scheduler that understands ecs views and data, and will automatically ensure that there are no multithreading issues for read/write accesses to the same resources when used properly.
Essentially when you create all your multithreaded accesses to the ecs through the rynx scheduler contexts, and do not capture any data in your task lambdas but request all your resources as parameters, the scheduler will do the work for you.
`TODO: Offer an example :)`
