Problem Sheet #7, Problem 7.1: dynamic linking (Project Euler 14)

Files
  collatz.h        declaration of int collatz(long long n)
  collatz.c        C implementation of collatz(), built as a shared library
  main.c           pure C solution, linked against libcollatz.so
  collatz.py       Python solution calling collatz() via ctypes.CDLL
  collatz_pure.py  original pure Python solution
  Makefile         build rules
  Assignment07_Daris_Naska.pdf   performance measurements (part d)

Build
  make          builds libcollatz.so and collatz-c
  make bench    times all three variants
  make clean    removes built files

Run
  ./collatz-c
  python3 collatz.py        (run make first)
  python3 collatz_pure.py
