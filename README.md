# cpprun-cxx
A simple utility to compile and run C++ programs with a single command

# Usage

Let's say you have a (standalone) C++ source file like:

```c++
// hello.cpp
#include <iostream>
int main(int argc, const char **argv)
{
    std::cout << "Hello World!" << std::endl;
    for (int i = 1; i < argc; ++i)
    {
        std::cout << "argv[" << i << "]: " << argv[i] << std::endl;
    }
}
```

## Simple usage
You can run and build it directly with:
```bash
$ cpprun hello.cpp
Hello World!
```

## Verbose usage
To see what is going on under the hood, run:

```bash
$ env CPPRUN_VERBOSE=1 cpprun hello.cpp
>>> c++ -std=c++23 -Wall -Wextra -pedantic -g hello.cpp -o /var/folders/82/423hfkdjfhsh8h3hfdsf/T/cpprun-1926840765-22767/artifact.exe
>>> /var/folders/82/423hfkdjfhsh8h3hfdsf/T/cpprun-1926840765-22767/artifact.exe
Hello World!
>>> Cleaning up temporary directory: "/var/folders/82/423hfkdjfhsh8h3hfdsf/T/cpprun-1926840765-22767"
```

## Build vs. run argument separation
Most command line arguments are passed as-is to the compiler.

If you want to pass arguments to the final executable invocation, use the `--` convention:

```bash
$ env CPPRUN_VERBOSE=1 cpprun hello.cpp -O3 -- foo bar baz
>>> c++ -std=c++23 -Wall -Wextra -pedantic -g hello.cpp -O3 -o /var/folders/82/423hfkdjfhsh8h3hfdsf/T/cpprun-2754959787-28887/artifact.exe
>>> /var/folders/82/423hfkdjfhsh8h3hfdsf/T/cpprun-2754959787-28887/artifact.exe foo bar baz
Hello World!
argv[1]: foo
argv[2]: bar
argv[3]: baz
>>> Cleaning up temporary directory: "/var/folders/82/423hfkdjfhsh8h3hfdsf/T/cpprun-2754959787-28887"
```
Here you see that `-O3` was passed to the compiler, while `foo bar baz` were passed to the built executable.

## Special arguments and environment variables

`cpprun` tool detects and handles some compiler arguments:
- `-c`: only compile the source, do not link or run it (run arguments are simply ignored)
- `-o`: path to where compiler should write the output artifact, overriding the internal temporary file path. Example: `cpprun hello.cpp -o hello` produces a binary `hello` in the current directory, and also runs it.
- `-std=`: set the C++ standard used. Overrides the internal default (see below).

`cpprun` also supports some overridable environment variables:
- `CPPRUN_CXX`: select the compiler used. Default value: `c++`.
- `CPPRUN_CXXFLAGS`: default value `-Wall -Wextra -pedantic -g`. Any options passed in the command line are simply appended to this one. Disable these defaults by setting the env var to `""`.
- `CPPRUN_CXX_STANDARD`: default value is `-std=c++23`. Note: any `-std=` argument in the command line overrides this setting. You can disable the default standard by setting the env var to `""`.

# Build and install

The recommended way is to use CMake:

```bash
# configure
$ cmake -B out -S . -G Ninja  # etc

# build
$ cmake --build out
```

alternatively, you can build it with manual compiler invocation:

```bash
$ mkdir out
$ c++ -std=c++17 -Wall -Wextra -pedantic -o out/cpprun cpprun.cpp
```

(**NOTE**: The required compiler flags depend on the compiler and version used. Prefer using CMake since it knows how to figure out the details automatically.)

The program should now be runnable:

```bash
$ ./out/cpprun --cpprun-compiler-info
>>> c++ --version
Apple clang version 17.0.0 (clang-1700.0.13.5)
Target: arm64-apple-darwin24.5.0
Thread model: posix
InstalledDir: /Library/Developer/CommandLineTools/usr/bin
```

The binary should be standalone, meaning you can place it somewhere in your `$PATH` for easy invocation. Example:

```bash
$ cp out/cpprun ~/.local/bin/
$ cpprun --cpprun-compiler-info
```

# Testing

```bash
$ cmake -B out -S . -G Ninja -DBUILD_TESTING=ON

$ cmake --build out

$ ctest --test-dir out
Test project /Users/markus/dev/github.com/cpprun-cxx/out
    Start 1: cpprun_test
1/1 Test #1: cpprun_test ......................   Passed    0.66 sec

100% tests passed, 0 tests failed out of 1

Total Test time (real) =   0.66 sec
```

# License

Copyright 2026 Markus Holmström (MawKKe)

The works under this repository are licenced under Apache License 2.0. See file LICENSE for more information.