# rados-drip

RADOS 접근을 실험하기 위한 작은 C++ 프로젝트입니다.

## 의존성

Arch Linux에서는 AUR에서 `librados`를 설치합니다.

```sh
yay -S librados
```

## 새 설정 파일

`<config>`는 `hosts`, `key`만 읽습니다. 파일 권한은 반드시 `0600`이어야
합니다.

```ini
# rados-drip.conf
hosts = 10.0.0.11:6789,10.0.0.12:6789,10.0.0.13:6789
key = AQB...
```

```sh
chmod 600 rados-drip.conf
./build/rados-drip rados-drip.conf <pool>
```

## 테스트

GTest를 설치한 뒤 테스트를 포함해 프로젝트를 빌드하고 CTest로 실행합니다.

```sh
cmake -S . -B build -G Ninja -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

특정 테스트만 실행하려면 `-R`에 테스트 이름의 정규식을 전달합니다.

```sh
ctest --test-dir build -R ParserTest --output-on-failure
```

## 파일 라인 수 검사

C/C++ 소스 파일은 기본적으로 100줄까지 허용합니다. 응집된 책임을 유지하기
위해 더 긴 파일이 필요하면 `.line-limits.json`에 사유와 함께 100줄 단위로
제한을 올릴 수 있으며, 절대 상한은 500줄입니다. 빈 줄과 주석을 포함한 실제
파일 라인 수를 계산합니다.

```sh
python3 scripts/check_file_lines.py
```

예외는 다음과 같이 기록합니다.

```json
{
  "overrides": {
    "client/client.cc": {
      "max_lines": 200,
      "reason": "하나의 응집된 프로토콜 구현을 함께 유지"
    }
  }
}
```

`max_lines`에는 `200`, `300`, `400`, `500`만 사용할 수 있습니다. 검사를
통과하면 종료 코드 `0`, 라인 제한을 위반하면 `1`, 설정이나 파일을 읽을 수
없으면 `2`를 반환합니다.
