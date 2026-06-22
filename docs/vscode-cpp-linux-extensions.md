# VS Code C++ 리눅스 확장 추천

이 프로젝트처럼 CMake를 쓰는 작은 C++ 프로젝트는 빌드 정보가 `compile_commands.json`로
연결되는 구성이 가장 다루기 쉽다. 리눅스 기준으로는 `clangd` + `CMake Tools` 조합을
기본값으로 추천한다.

## 추천 구성

| 우선순위 | 확장 | 식별자 | 용도 |
| --- | --- | --- | --- |
| 필수 | clangd | `llvm-vs-code-extensions.vscode-clangd` | C/C++ 코드 완성, 진단, 이동, 리팩터링 |
| 필수 | CMake Tools | `ms-vscode.cmake-tools` | CMake configure/build/test 실행과 빌드 디렉터리 관리 |
| 필수 | CodeLLDB | `vadimcn.vscode-lldb` | LLDB 기반 네이티브 디버깅 |
| 권장 | Clang-Format | `xaver.clang-format` | `clang-format` 기반 수동/저장 시 포맷 |
| 권장 | EditorConfig | `editorconfig.editorconfig` | 들여쓰기, 줄 끝, 최종 개행 같은 편집 규칙 통일 |
| 선택 | GitLens | `eamodio.gitlens` | blame, 파일 이력, 커밋 탐색 |
| 선택 | Code Spell Checker | `streetsidesoftware.code-spell-checker` | 주석과 문서의 영문 오타 확인 |

## 설치

VS Code 명령줄 도구가 있으면 아래처럼 설치한다.

```sh
code --install-extension llvm-vs-code-extensions.vscode-clangd
code --install-extension ms-vscode.cmake-tools
code --install-extension vadimcn.vscode-lldb
code --install-extension xaver.clang-format
code --install-extension editorconfig.editorconfig
```

선택 확장까지 설치하려면 아래를 추가한다.

```sh
code --install-extension eamodio.gitlens
code --install-extension streetsidesoftware.code-spell-checker
```

## 리눅스 패키지

확장만으로 컴파일러와 빌드 도구가 설치되지는 않는다. Debian/Ubuntu 계열에서는 보통
아래 패키지가 필요하다.

```sh
sudo apt install build-essential cmake ninja-build clangd clang-format clang-tidy lldb
```

배포판 패키지 이름은 다를 수 있다. Fedora 계열은 `gcc-c++`, `cmake`, `ninja-build`,
`clang-tools-extra`, `lldb`를 확인한다.

## 프로젝트 설정

`clangd`가 정확한 include path와 compile flag를 알 수 있도록 CMake의 compile database를
켜는 것이 중요하다.

```sh
cmake -S . -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build
```

VS Code 워크스페이스 설정 예시는 아래와 같다.

```json
{
  "cmake.buildDirectory": "${workspaceFolder}/build",
  "cmake.configureSettings": {
    "CMAKE_EXPORT_COMPILE_COMMANDS": "ON"
  },
  "clangd.arguments": [
    "--compile-commands-dir=${workspaceFolder}/build",
    "--clang-tidy"
  ],
  "[cpp]": {
    "editor.defaultFormatter": "xaver.clang-format",
    "editor.formatOnSave": true
  },
  "[c]": {
    "editor.defaultFormatter": "xaver.clang-format",
    "editor.formatOnSave": true
  }
}
```

## Microsoft C/C++ 확장과의 관계

Microsoft C/C++ 확장(`ms-vscode.cpptools`)도 좋은 선택지지만, `clangd`와 동시에 켜면
IntelliSense와 진단이 중복될 수 있다. 이 프로젝트에서는 `clangd`를 기본으로 쓰고,
`ms-vscode.cpptools`는 Microsoft 디버거 통합이나 cpptools 전용 기능이 필요할 때만
추가하는 편이 단순하다.

둘을 함께 설치해야 한다면 cpptools IntelliSense를 끄는 설정을 고려한다.

```json
{
  "C_Cpp.intelliSenseEngine": "disabled"
}
```

## 디버깅

CodeLLDB를 사용할 때는 Debug 빌드와 디버그 심볼을 켠 빌드가 편하다.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build
```

간단한 `launch.json` 예시는 아래와 같다.

```json
{
  "version": "0.2.0",
  "configurations": [
    {
      "type": "lldb",
      "request": "launch",
      "name": "Debug cpp_structure",
      "program": "${workspaceFolder}/build/cpp_structure",
      "args": [],
      "cwd": "${workspaceFolder}",
      "preLaunchTask": "CMake: build"
    }
  ]
}
```

## 참고 자료

- clangd VS Code 확장: <https://marketplace.visualstudio.com/items?itemName=llvm-vs-code-extensions.vscode-clangd>
- CMake Tools: <https://marketplace.visualstudio.com/items?itemName=ms-vscode.cmake-tools>
- CodeLLDB: <https://marketplace.visualstudio.com/items?itemName=vadimcn.vscode-lldb>
- Clang-Format: <https://marketplace.visualstudio.com/items?itemName=xaver.clang-format>
- EditorConfig: <https://marketplace.visualstudio.com/items?itemName=editorconfig.editorconfig>
- Microsoft C/C++: <https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools>
