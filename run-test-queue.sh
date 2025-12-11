#!/bin/bash

function run_test() {
  for nthr in 1 2 4 8 16 32; do
    ./$1 $nthr >/dev/null
    for i in $(seq 1 3); do
      ./$1 $nthr
      echo
    done
    echo
  done
}

echo "test lock queue"
run_test "test-queue-lock"

echo "test ck lockfree queue"
LD_LIBRARY_PATH=$HOME/.local/lib run_test "test-queue-lockfree-ck"

echo "test lockfree queue"
run_test "test-queue-lockfree"
