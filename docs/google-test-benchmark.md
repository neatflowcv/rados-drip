# GoogleTest와 Google Benchmark 사용 정리

이 프로젝트는 `simple` 라이브러리를 만들고, 같은 코드를 앱, 테스트, 벤치마크에서
공유하도록 구성한다.

## 구성

- `simple/simple.h`, `simple/simple.cc`: 테스트와 벤치마크 대상 함수
- `simple/simple_test.cc`: GoogleTest 단위 테스트
- `simple/simple_benchmark.cc`: Google Benchmark 마이크로벤치마크
- `CMakeLists.txt`: `FetchContent`로 GoogleTest와 Google Benchmark를 가져오고
  각각 `simple_test`, `simple_benchmark` 타깃을 만든다.

## 빌드

기본 설정은 테스트와 벤치마크를 모두 빌드한다.

```sh
cmake -S . -B build
cmake --build build
```

벤치마크가 필요 없으면 아래처럼 끌 수 있다.

```sh
cmake -S . -B build -DCPP_STRUCTURE_BUILD_BENCHMARKS=OFF
cmake --build build
```

테스트가 필요 없으면 CMake의 표준 옵션인 `BUILD_TESTING`을 끈다.

```sh
cmake -S . -B build -DBUILD_TESTING=OFF
cmake --build build
```

## GoogleTest 실행

전체 테스트는 `ctest`로 실행한다.

```sh
ctest --test-dir build
```

테스트 바이너리를 직접 실행할 수도 있다.

```sh
./build/simple_test
```

GoogleTest의 공식 CMake quickstart는 `GTest::gtest_main`에 링크하고
`gtest_discover_tests()`로 CTest 테스트를 자동 등록하는 방식을 안내한다. 이
프로젝트도 같은 방식을 사용한다.

## Google Benchmark 실행

벤치마크 바이너리를 직접 실행한다.

```sh
./build/simple_benchmark
```

실행 시간을 짧게 확인하려면 dry run을 사용할 수 있다.

```sh
./build/simple_benchmark --benchmark_dry_run
```

일부 벤치마크만 실행하려면 필터를 사용한다.

```sh
./build/simple_benchmark --benchmark_filter=BM_Add
```

결과 파일이 필요하면 출력 형식을 지정한다.

```sh
./build/simple_benchmark --benchmark_out=build/simple_benchmark.json --benchmark_out_format=json
```

Google Benchmark 문서는 CMake 프로젝트에서 `benchmark::benchmark` 또는
`benchmark::benchmark_main` 타깃에 링크하는 방식을 권장한다. 이 프로젝트는 별도
`main()`을 두지 않고 `benchmark::benchmark_main`을 사용한다.

성능 수치를 실제로 비교할 때는 Debug 빌드가 아니라 Release 빌드를 사용한다.

```sh
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release
./build-release/simple_benchmark
```

## 참고 자료

- GoogleTest CMake quickstart: <https://google.github.io/googletest/quickstart-cmake.html>
- GoogleTest releases: <https://github.com/google/googletest/releases>
- Google Benchmark README: <https://github.com/google/benchmark>
- Google Benchmark user guide: <https://github.com/google/benchmark/blob/main/docs/user_guide.md>
- Google Benchmark releases: <https://github.com/google/benchmark/releases>
