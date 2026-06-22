# Google C++ Style 요약

Google C++ Style Guide에서 자주 필요한 규칙만 간단히 정리한다.

## 파일명

- 파일명은 모두 소문자로 쓴다.
- 단어 구분은 `_`를 쓴다.
- C++ 구현 파일은 `.cc`, 헤더 파일은 `.h`를 쓴다.
- 특정 위치에 텍스트로 include되는 파일은 `.inc`를 쓴다.
- 테스트 파일은 `*_test.cc` 형식을 쓴다.
- 너무 일반적인 이름보다 구체적인 이름을 쓴다.
  - 좋음: `http_server_logs.h`
  - 피함: `logs.h`
- `/usr/include`에 이미 있는 이름은 피한다.
  - 예: `db.h`

예:

```text
foo_bar.h
foo_bar.cc
foo_bar_test.cc
```

## 헤더

- 보통 `.cc` 파일마다 대응하는 `.h` 파일을 둔다.
- `main()`만 있는 작은 `.cc` 파일이나 테스트 파일은 예외일 수 있다.
- 헤더는 혼자 include해도 컴파일되는 self-contained 형태여야 한다.

## include 순서

관련 헤더를 가장 먼저 include하고, 나머지는 그룹별로 빈 줄을 둔다.

```cpp
#include "foo/bar.h"

#include <unistd.h>

#include <string>
#include <vector>

#include "base/logging.h"
```

각 그룹 안에서는 알파벳순으로 정렬한다.

## 이름 규칙

- 타입: `PascalCase`
  - 예: `FooBar`, `UrlTable`
- 함수: 보통 `PascalCase`
  - 예: `OpenFileOrDie()`
- 변수와 매개변수: `snake_case`
  - 예: `file_name`
- 클래스 멤버 변수: `snake_case_`
  - 예: `file_name_`
- 네임스페이스: `snake_case`
  - 예: `storage_util`
- 매크로: `UPPER_SNAKE_CASE`

## 포맷

- 이 프로젝트에서는 `clang-format -i app/main.cc`로 포맷한다.
- 기존 코드 스타일이 있으면 그 스타일을 우선한다.

참고: [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html)
