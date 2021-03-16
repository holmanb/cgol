Instructions
============

Build Deps
----------
golang

Build
-----
go build

Run
---
./cgol    # will use the default data set (to be executed using the symlink for now)
./cgol -h # view more options

Benchmark
---------
go test -cpuprofile cpu.prof -memprofile mem.prof -bench .
go tool pprof cpu.prof
(prof) web # will open svg in browser of profiled call graph
