# EzLogger
Easy, lightweight, header-only, thread-safe, C++20 file logger.

Ez is a **header-only**, thread-safe, modern C++20 logger designed to be easy to drop into any project.  
It uses `std::source_location` to automatically capture file, function, and line information without traditional macros.

## Features

- Header-only, no external dependencies
- Thread-safe (mutex + atomic)
- Configurable log levels: TRACE, DEBUG, INFO, WARNING, ERROR
- Automatic capture of function, file, and line via `std::source_location`

## Installation
Simply copy `logger.h` into your project include folder (e.g., `include/`) and include it:
```cpp
#include "logger.h"
```
## Example usage
```cpp
#include "logger.h"

int main() {
    if (auto err = setLogFile("app.log")) {
        return *err;
    }

    // Set log level (optional, default is INFO)
    setDebugLogLevel();

    // Log messages
    LOG_INFO("Application started");
    LOG_DEBUG("Value of x: 42");
    LOG_ERROR("Critical error occurred");

    // Close log file (optional, RAII handles it)
    closeLogFile();
    return 0;
}
```
