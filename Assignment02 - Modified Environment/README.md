# Problem 2.2 - Execute a Command in a Modified Environment
Overview
This assignment focuses on Unix environment variables and process execution in C. The goal is to implement a simplified version of the standard Unix env utility.
Implementation
Developed a C program (env) that can display and modify the process environment, as well as execute commands with a modified environment.
The program supports:
	•	Printing the current environment using environ.
	•	Adding environment variables using NAME=VALUE arguments.
	•	Removing environment variables using the -u option.
	•	Executing commands with optional arguments.
	•	Executing commands in a modified environment.
	•	Verbose mode (-v) for tracing program operations to standard error.
	•	Command-line argument parsing using getopt().
Command execution is handled using execvp() rather than system(), avoiding unnecessary shell interpretation.
Concepts and Technologies
	•	Language: C
	•	Operating Systems: Unix processes and environment variables.
	•	Environment Management: environ, getenv(), setenv(), unsetenv().
	•	Process Execution: execvp() and the exec family of system calls.
	•	Command-line Interfaces: Argument parsing with getopt().
	•	Error Handling: System call errors and appropriate error reporting.
Files
	•	env.c - Implementation of the env utility.
