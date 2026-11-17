# Problem 5- 100 Prisoners Problem - Operating Systems

## Overview

This assignment explores thread synchronization and concurrent programming in C by simulating an energy drink vending machine.

The system consists of students, a vending machine, and a supplier. Multiple students can request drinks, but the machine must coordinate their access to ensure that each transaction is completed correctly.

## Implementation

Implemented a vending machine simulation in C using POSIX threads, mutexes, and condition variables.

The program models three types of concurrent activities:

- **Students:** Request energy drinks, insert the required number of coins, and pick up their drinks.
- **Vending Machine:** Accepts coins, dispenses drinks, and waits for students to pick them up before starting another transaction.
- **Supplier:** Refills the machine to its maximum capacity and collects the accumulated coins when the machine runs out of drinks.

The synchronization mechanism ensures that:

- Only one student can use the machine at a time.
- The student who inserts the coins is the same student who receives the drink.
- Students and the machine wait when the machine runs out of drinks.
- The machine waits until the current student picks up their drink before serving another student.

## Command-Line Options

The program supports the following options:

- `-n` - Number of students (default: 2).
- `-c` - Maximum capacity of the vending machine (default: 4).
- `-p` - Price of each drink in coins (default: 5).

## Concepts and Technologies

- **Language:** C
- **Operating Systems:** Concurrent programming and thread synchronization.
- **Multithreading:** POSIX threads (pthreads).
- **Synchronization:** POSIX mutexes and condition variables.
- **Mutual Exclusion:** Ensuring exclusive access to the vending machine.
- **Producer-Consumer Problem:** Coordination between students, the machine, and the supplier.
- **Thread Coordination:** Managing shared resources and transaction order.

## Files

- `energy.c` - Implementation of the energy drink vending machine simulation.