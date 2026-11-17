# Problem 4.1 - 100 Prisoners Problem - Operating Systems
Overview
This assignment explores concurrent programming and thread synchronization in C through a simulation of the 100 Prisoners Problem.
The goal is to simulate different strategies used by 100 prisoners searching for their assigned numbers among 100 drawers, with each prisoner allowed to open a maximum of 50 drawers.
Implementation
Developed a C program (prisoner) that simulates the game using concurrent threads, with each thread representing one prisoner.
Two search strategies are implemented:
	•	Random Strategy: Each prisoner randomly selects drawers while searching for their assigned number.
	•	Smart Strategy: Each prisoner starts by opening the drawer matching their own number and then follows the sequence of drawer numbers found.
Each strategy is implemented using two different locking mechanisms:
	1	Global Lock: A single lock protects all drawers, causing prisoners to execute their searches sequentially.
	2	Individual Drawer Locks: Each drawer has its own lock, allowing prisoners to search concurrently while coordinating access to individual drawers.
This results in four implementations:
	•	random_global - Random search using a global lock.
	•	random_drawer - Random search using individual drawer locks.
	•	smart_global - Smart search using a global lock.
	•	smart_drawer - Smart search using individual drawer locks.
Simulation and Performance Analysis
The program supports:
	•	Configurable number of game simulations using the -n option (default: 100).
	•	Random number generator seeding using the -s option.
	•	Measuring the success rate of each strategy.
	•	Measuring and comparing average execution times.
The results are used to analyse the differences between sequential and concurrent execution, as well as the effectiveness of the two search strategies.
Concepts and Technologies
	•	Language: C
	•	Operating Systems: Processes and concurrent programming.
	•	Multithreading: POSIX threads (pthreads).
	•	Synchronization: Mutex locks, global locking, and per-drawer locking.
	•	Concurrency: Sequential and concurrent execution.
	•	Randomization: Random number generation and seeding.
	•	Performance Analysis: Execution time measurement and success-rate comparison.
Files
	•	prisoner.c - Implementation of the 100 Prisoners Problem simulation.