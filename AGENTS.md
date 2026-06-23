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

## Run

```sh
./build/rados-drip
```
