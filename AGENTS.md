# Project Notes

This is a small C++ project using CMake.

## Layout

- `CMakeLists.txt`: root CMake configuration
- `app/`: application entry points and executable-specific options
- `cli/`: shared command-line parsing
- `config/`: configuration parsing
- `build/`: local CMake build output

## Build

```sh
cmake -S . -B build -G Ninja -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build
```

## Format

```sh
clang-format -i $(find . -path ./build -prune -o -type f \( -name '*.cc' -o -name '*.h' \) -print)
```

## Lint

```sh
run-clang-tidy -p build -fix
```

## Verify

Before handing off changes, run format, lint, build, tests, and help output checks.

```sh
clang-format -i $(find . -path ./build -prune -o -type f \( -name '*.cc' -o -name '*.h' \) -print)
run-clang-tidy -p build -fix
cmake --build build
ctest --test-dir build --output-on-failure
./build/rados-drip --help
./build/rados-delete --help
```

## Run

```sh
./build/rados-drip
```
