#!/usr/bin/env bash

CC=gcc
CFLAGS="-g -O0 -pedantic-errors -Wall -Wextra -Werror -Wunused"

tests=`ls -1 *_test.c`

for test in $tests;
do
  root=${test%_test.c}
  echo $root
  compile="${CC} ${CFLAGS} -o ${root}_test ${root}.c ${root}_test.c"
  (eval ${compile} && eval valgrind --leak-check=full ./${root}_test) || \
      ( echo FAIL: ./${root}_test ; exit 1 ) ||  exit 1
done