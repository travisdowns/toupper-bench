Read all the gory details on the [associated blog post](https://travisdowns.github.io/blog/2019/11/19/toupper.html).

The effect seems to apply on most glibc versions, but I can't guarantee it. Build with `make` and run the benchmark with `./bench`. There are env vars that affect the behavior (see `main.cpp`) and some scripts in `scripts` that I use to generate the plots: collect the data with [scripts/collect-data.sh](scripts/collect-data.sh) and plot it with [scripts/all-plots.sh](scripts/all-plots.sh). Yeah, it forces you to set some env vars, sorry.
