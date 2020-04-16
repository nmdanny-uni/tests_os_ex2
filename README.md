# Tests for OS Exercise 2, 2019-2020
# Introduction

These are some basic tests for OS Exercise 2.

Note that the helpfulness of these tests may be limited, especially
due to the low level nature of this exercise - you will probably face weird segfaults
and other fun and exotic bugs, and sometimes you won't get very meaningful stackframes. 

Therefore, you should write your code using a lot of print statements, assert statements and use the debugger heavily

Moreover, I recommend you try writing your own tests(with/without the googletest framework here), this is a great way
of testing your assumptions and seeing how the thread library works.

## Important notes

  
- If you think the tests are factually incorrect(they assert wrong things), then please create a Github issue
  
  Note that crashes and segfaults are most likely because of problems with your own code and not with the tests
  (except for some things - see notes below) - remember that functions of the thread library shouldn't crash/terminate
  ungracefully either way, even if the tests are wrong - in the worst case, googletest assertions will fail, but this
  shouldn't lead to segfaults and other memory corruption issues.
  
- Sometimes you may get stack overflow issues(try using [Address Sanitizer](#address-sanitizer)), this depends on your library implementation

  I recommend you start with a large `STACK_SIZE` (e.g, `16384`), and only once you've fixed all bugs, try using a smaller stack
  size. (Modify your own `uthreads.h`)
  
  * Note that the Address sanitizer itself adds a non trivial amount of overhead to the stack
  
- You can't run all tests at once normally, you must run them one-by one or via all via a shell script.
  [See below](#usage-guide).
 
- Test5 (Times and priorities) is inaccurate, sometimes it will pass and sometimes it will fail, this is
  because it measures real time and not virtual time. You may need to run it multiple times or increase `TIME_EPSIOLON` to get
  the test to pass.
  
- A good observation was raised in [the forums](https://moodle2.cs.huji.ac.il/nu19/mod/forum/discuss.php?d=60001) - in 
  short, some operations such as allocation are not "signal-safe", that is, if a signal occurs during an allocation
  procedure or such, the program may be left in an undefined state.
  
  I did NOT address this issue in the test, some tests allocate either directly(e.g inserting into a map) or possibly
  indirectly by how Googletest assertions work.
  
  In practice, this didn't cause any problems to me, but there's a small probability for a test to fail due to such 
  issues, especially in tests with a small quantum length(Test4,Test6). Therefore you may want to run such tests
  multiple times or increase the quantum duration.
  
## Installation guide

The tests assume your project is using CMake on a modern Linux distribution(such as the Aquarium servers), 
Windows and OSX aren't supported/tested. 

1. Within your project's root directory, type:
 
   `git clone https://github.cs.huji.ac.il/danielkerbel/tests_os_ex2 tests`
   
   (Use your CSE credentials to login)
   
2. Within your own project's `CMakeLists.txt`
   - Ensure you're compiling a library called `uthreads`
   - Add the tests directory via `add_subdirectory(tests)`
   
   An example of such `CMakeLists.txt`:
   ```cmake
   cmake_minimum_required(VERSION 3.1)
   #set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
   #set (CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
   project(threads VERSION 1.0 LANGUAGES C CXX)
   
   add_library(uthreads uthreads.h uthreads.cpp PUT_OTHER_SOURCE_FILES_HERE)
   
   set_property(TARGET uthreads PROPERTY CXX_STANDARD 11)
   target_compile_options(uthreads PUBLIC -Wall -Wextra)
   
   add_subdirectory(tests)
   
   ```
   
   (You can uncomment the two `set` statements to enable Address Sanitizer)

## Usage guide

### Using CLion to run a single test at once

- Open the tests .cpp source code, go to a test, click on the green arrow near `TEST(...)` and run it,
  either normally or via debug mode.

### Using the shell script to run all tests at once

- `cd tests`
- `chmod +x ./runall.sh`  (should only be necessary once)
- `./runall.sh`  (this will also recompile automatically)

This script will print number of test successes and failures. 
Note that if Address Sanitizer is enabled, memory errors will be considered as test failures.

### Running a single test via terminal


- `cd tests`
- `cd PROJECT_ROOT/cmake-build-debug`
- `cmake .. && make` 
- `./tests/theTests --gtest_filter="Test3*"` 

  (replace `3` with the test number, keep the `*`)

## Reading the tests

The tests are written using the googletest framework. Statements of the form `EXPECT_XXX` are used to compare values,
but upon failure they don't immediately stop the test (unlike `ASSERT_XXX` statements)
Each test ends with `uthread_terminate(0)` and an `ASSERT_EXIT` statement. Note that behind the scenes, such statement
causes the process to fork and run the termination function in the forked process - see more [here](https://github.com/google/googletest/blob/master/googletest/docs/advanced.md#how-it-works)
This shouldn't really matter, but it's something you should keep in mind.

Other than googletest related stuff, all test code uses standard C++11 and the uthread library, and you should be
able to understand what each test does.


## Advanced 
### Address Sanitizer
As an alternative to Valgrind, modern GCC and Clang compilers include built in support for ASan, read 
more [here](https://github.com/google/sanitizers/wiki/AddressSanitizer)

To use it, you can add the following lines to your project's `CMakeLists.txt` (not the tests):
```cmake
set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
set (CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
```

And, when running your own executable normally in CLion (with/without a debugger), the sanitizer will be enabled
and you can see what it reports in the `Sanitizer` tab (near `Console` at the bottom of the screen)

Some important notes:

- **ASan increases the stack memory overhead in your program**, therefore, it is suggested that you increase the stack size,
  otherwise tests that may have passed normally, would fail with ASan, and warnings issued by ASan might not reflect
  issues in your program.
  
  (For me, a stack size of `8196` seems to work OK, but this is highly dependant on your environment and implementation)

- In some cases, running via CLion won't show the sanitizer's message, instead showing weird stuff like 
  `AddressSanitizer: nested bug in the same thread, aborting.`
  
  In that case, try running via the terminal, [see above](#usage-guide)

- Just like with Valgrind, the warnings that ASan issues might not make sense, there may be false positives, etc..

  - A common false positive is `__asan_handle_no_return`, which happens when jumping from a thread that is later
    terminated without returning to it.

### Valgrind(Not for these tests)

Generally speaking, it is possible to make valgrind recognize the runtime allocated stacks by using `VALGRIND_STACK_REGISTER` or
`VALGRIND_STACK_DEREGISTER` - read the documentation [here](https://valgrind.org/docs/manual/manual-core-adv.html)

However, for odd reasons(perhaps Valgrind's interaction with googletest?), it seems to change test behavior drastically
and cause even the most basic things to fail, such as the very first call to `uthread_get_total_quantums`.

Therefore, Valgrind is absolutely not supported for these tests - prefer using Address Sanitizer instead. You might have
more luck with Valgrind on your own tests though.

###
