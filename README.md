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
