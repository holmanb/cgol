Conway's Game of Life
=====================

Conway's Game of Life is a game dictated by a set of rules and an initial condition.

> The rules are:
> 
> 1. Survivals. Every counter with two or three neighboring counters survives for the next generation.
> 2. Deaths. Each counter with four or more neighbors dies (is removed) from overpopulation. Every counter with one neighbor or none dies from isolation.
> 3. Births. Each empty cell adjacent to exactly three neighbors--no more, no fewer--is a birth cell. A counter is placed on it at the next move. 

(Taken from Martin Gardner's article on Conway's Game of Life, Scientific American, 1970)



The aim of this project is to idiomatically implement this game in multiple programming languages to explore multiple different languages' ability to solve embarrassingly parallel problems.

Current implementations:

	- C (OpenMP)
	- Go (goroutines)
	- Rust (not parallel yet)
	- Python (multiprocessing, shared memory)

Notes:

	- data format is CSV
	- terminal is used to display the output of each generation (simple and portable across \*nix systems)
	- arbitrary finite sizes (defined at runtime) are supported
	- each implementation contains a readme describing how to build and use (`./cgol -h` should work)
	- tests for each implementation currently use Github Actions (TODO: much room for improvement here)
	- an iterative approach to performance is taken with the following steps:
		1. an initial "correct" serial version is created
		2. parallelisation added
		3. memory efficiency considered (bitsets, etc)
		4. further optimizations (examine dissassembly, benchmark/flamegraph, etc)
		5. accept pull requests for contributions that provide measurable improvements
	- of course, this small project is under active development, so the above may actually describe "desired state", rather than "current state", pull requests are welcome :)

