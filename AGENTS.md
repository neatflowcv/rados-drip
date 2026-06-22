# Project Notes

This is a small C++ project using CMake.

## Layout

- `CMakeLists.txt`: root CMake configuration
- `app/main.cc`: application entry point
- `simple/`: simple library source, tests, and benchmarks
- `build/`: local CMake build output

Keep a feature's source, test, and benchmark files in the same feature
directory. For example, `simple/simple.cc`, `simple/simple_test.cc`, and
`simple/simple_benchmark.cc` live together under `simple/`.

## Build

```sh
cmake -S . -B build -G Ninja -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build
```

## Format

```sh
clang-format -i app/main.cc simple/*.cc simple/*.h
```

## Tidy

```sh
cmake -S . -B build -G Ninja -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build
```

The build runs `clang-tidy` automatically when it is available.

## Run

```sh
./build/cpp_structure
```
