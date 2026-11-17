# Problem 9 - Weather Forecast in the Terminal 

## Overview

This assignment involves developing a C program that retrieves weather information from an online weather service and displays an hourly weather forecast in the terminal.

The program uses the Open-Meteo API to obtain weather data and the JSON-C library to parse the returned JSON response.

## Implementation

The program is divided into several components:

- **Weather Data Fetching (`meteo_fetch.c`):** Retrieves weather data by creating a child process to execute `curl` and using a pipe to capture its output. The function returns the response as a JSON object.
- **JSON Decoding (`meteo_decode.c`):** Parses the JSON response and extracts hourly weather information, including temperature, relative humidity, precipitation, and wind speed.
- **Weather Management (`weather.c`):** Implements functions for creating, managing, appending, resetting, printing, and freeing weather records.
- **Main Program (`main.c`):** Coordinates the retrieval, processing, and display of the weather forecast.

## Concepts and Technologies

- **Language:** C
- **Operating Systems:** Process creation and inter-process communication.
- **Process Management:** Creating child processes using `fork()` and executing external programs.
- **Pipes:** Capturing output from a child process.
- **System Calls:** Process and file descriptor management.
- **External Programs:** Executing `curl` to retrieve data from a web service.
- **JSON Parsing:** JSON-C library.
- **Dynamic Memory Management:** Allocating and freeing weather records.
- **Debugging:** Memory checking and error detection using Valgrind.

## Files

- `main.c` - Main program.
- `meteo_fetch.c` - Fetches weather data using `curl`.
- `meteo_decode.c` - Parses the JSON weather response.
- `weather.c` - Implements weather data management and printing functions.
- `weather.h` - Header file containing weather structures and function declarations.