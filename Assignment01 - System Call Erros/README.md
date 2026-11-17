#Assignment 01 - Operating Systems
Overview
This assignment explores fundamental operating system concepts related to system calls, error handling, and buffered I/O in C.
Problems
Problem 1.1 - System Call Errors
Explored error handling in Unix system calls, focusing on open() and close(), the use of errno, and how system calls report failures.
Problem 1.2 - Repeating a Stringx
Developed a C program (repeat) that repeatedly writes a user-provided message to standard output.
The program supports:
	•	Command-line argument parsing using getopt().
	•	Configurable repetition count using -r.
	•	Optional newline suppression using -n.
	•	Output buffering configuration:
	◦	-u: Unbuffered output.
	◦	-l: Line-buffered output.
	◦	-b: Fully buffered output.
	•	System call tracing and library call analysis using strace and ltrace.
	•	Performance measurements comparing different buffering modes and buffer sizes.
Concepts and Technologies
	•	Language: C
	•	Operating Systems: Unix system calls, error handling, file descriptors.
	•	I/O: Standard I/O, output buffering, setvbuf().
	•	Command-line interfaces: Argument parsing with getopt().
	•	Debugging and Analysis: strace, ltrace, performance benchmarking.