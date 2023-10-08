# Benchmarks

To run the benchmarks of the two inclusive scan implementations, you need to have the `pyperf` package installed. After compiling and running the two programs, run the command:

`pyperf compare_to output_inclusive.json output_blelloch.json --table`

A table will appear with a comparison of the execution times.

The source code for the benchmarks can be found in the [`nanobench`](https://github.com/martinus/nanobench) repository.
