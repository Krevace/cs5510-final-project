# Installation of Concurrency Kit

This project includes a **lock-free fifo mpmc implementation** from [Concurrency Kit (CK)](https://github.com/concurrencykit/ck) for benchmarking purposes. It can be installed like so:

```bash
git clone https://github.com/concurrencykit/ck.git
cd ck
./configure --prefix=$HOME/.local
make 
make install
```

**Note:** If you wish to exclude this from testing, simply comment out the `test-queue-lockfree-ck` compilation target in the `Makefile` and the corresponding execution in ```./run-test-queue.sh```.

# Compilation

```make clean; make```

# Running Tests

Simply run ```./run-test-queue.sh```. 

## Output Format

- **Rows:** Groups of three, representing thread counts: 1, 2, 4, 8, 16, 32
- **Columns:** Different tests, left to right:
    1. 1M concurrent enqueues
    2. 1M concurrent dequeues
    3. 500K concurrent enqueues
    4. 500K concurrent enqueues & dequeues
- **Each Cell:** Reports three executions of the same setup for averaging, including:
    1. Throughput (operations per second)
    2. Latency (microseconds)

Correctness checks are made in tandem. If any check fails, the program will print an error message and immediately exit.
