# Tests for OS Exercise 2, 2019-2020
## Introduction

These are are some basic tests for OS EX2, they only cover basic usage
involving deterministic scenarios, they do not cover signal races and other
possible edge cases.

## Important notes

- The helpfulness of these tests may be very limited, especially
  due to the low level nature of this exercise - you will probably face weird segfaults
  and other exotic bugs, and sometimes you won't get very meaningful stackframes.
  Therefore, you must be able to read and understand these tests, and I recommend you
  try writing some of your own before using these tests - it is a great way to understand
  what the library does. Use both the debugger and many print statements to visualize what's
  going on.
  
- You can't run all tests at once normally, you must either do so one-by-one or with
  the provided shell script(`./runall.sh`. When running one by one, you can use CLion and the
  debugger
  
## Installation guide

The tests assume your project is using CMake on a modern Linux distribution, 
Windows and OSX aren't supported/tested

1. Within your project's root directory, type:
 
   `git clone https://github.cs.huji.ac.il/danielkerbel/tests_os_ex2 tests`
   
   (Use your CSE credentials to login)
   
2. Within your own project's `CMakeLists.txt`
   - Ensure you're compiling a library called `uthreads`
   - Add the tests directory via `add_subdirectory(tests)`
   
   An example of such `CMakeLists.txt`:
   ```cmake
   cmake_minimum_required(VERSION 3.1)
   project(threads VERSION 1.0 LANGUAGES C CXX)
   
   add_library(uthreads uthreads.h uthreads.cpp PUT_OTHER_SOURCE_FILES_HERE)
   
   set_property(TARGET uthreads PROPERTY CXX_STANDARD 11)
   target_compile_options(uthreads PUBLIC -Wall -Wextra)
   
   add_subdirectory(tests)
   
   ```

## Usage guide

### Using CLion to run a single test at once

- Open the tests .cpp source code, go to a test, click on the green arrow near `TEST(...)` and run it,
  either normally or via debug mode.

### Using the shell script to run all tests at once

- `cd tests`
- `chmod +x ./runall.sh`  (should only be necessary once)
- `./runall.sh`


## Advanced 

The following tools are NOT supported by these tests, this section is for your general knowledge
These may or may not help you, I do not recommend using these with my tests as you will get very
confusing error messages(especially due to googletest behavior)

### Valgrind
Generally speaking, you CAN use valgrind for this exercise, however, you need to make it aware of stacks that are
allocated dynamically, you can do so as follows:

- Add the include: 
  
  `#include <valgrind/valgrind.h>`
  
- Register the stack upon thread creation:

  `valgrind_stack_id = VALGRIND_STACK_REGISTER(buffer, buffer + STACK_SIZE);`
  
- Upon deletion the thread, de-register the stack:

  `VALGRIND_STACK_DEREGISTER(valgrind_stack_id)`
  
This won't detect all errors as in a traditional program, but it won't yell at you for every operation done within a
user thread.

Valgrind doesn't work with the tests - for example, the very first call
to `uthread_get_total_quantums` might fail, returning 2 instead of 1 (and the rest
of the test will run in an undefined way, more tests may fail as a result, even
though they would've succeeded if running normally)

I don't know what is the reason for this, maybe it's how googletest's framework deals
with tests, maybe the valgrind virtual machine does stuff differently, or their interaction
together causes this. If anyone finds a solution, let me know.
  

## Address Sanitizer
As an alternative to Valgrind, modern GCC and Clang compilers include built in support for ASan, read 
more [here](https://github.com/google/sanitizers/wiki/AddressSanitizer)

To use it, you can add the following lines to your project's `CMakeLists.txt` (not the tests):
```cmake
set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
set (CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
```

And, when running your own executable normally in CLion (with/without a debugger), the sanitizer will be enabled
and you can see what it reports in the `Sanitizer` tab (near `Console` at the bottom of the screen).

Like with Valgrind, this will cause many false positives and may cause tests to fail for mysterious reasons. 