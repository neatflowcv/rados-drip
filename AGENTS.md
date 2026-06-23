# Project Notes

This is a small C++ project using CMake.

## Layout

- `CMakeLists.txt`: root CMake configuration
- `app/main.cc`: application entry point
- `build/`: local CMake build output

## Build

```sh
cmake -S . -B build -G Ninja -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build
```

## Format

```sh
clang-format -i app/main.cc
```

## Lint

```sh
run-clang-tidy -p build -fix
```

## Verify

Before handing off changes, run format, lint, build, and help output checks.

```sh
clang-format -i app/main.cc app/config.cc app/config.h client/client.cc client/client.h client/object.cc client/object.h options/options.cc options/options.h
run-clang-tidy -p build -fix
cmake --build build
./build/rados-drip --help
```

## Run

```sh
./build/rados-drip
```
