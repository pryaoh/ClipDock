# ClipDock

[![Build and Test](https://github.com/pryaoh/ClipDock/actions/workflows/qt-build.yml/badge.svg)](https://github.com/pryaoh/ClipDock/actions/workflows/qt-build.yml)

## 프로그램 소개

ClipDock는 C++20과 Qt 6 기반의 macOS·Windows·Linux용 화면 녹화 프로그램입니다.
디스플레이·창·지정 영역을 미리 보고 MP4로 녹화하며, 일시정지·재개와 마이크 입력 녹음을 지원합니다.
출력 크기, FPS, 비트레이트를 설정할 수 있고, 영상은 H.264 또는 MPEG-4, 오디오는 AAC로 저장합니다.
캡처 대상 선택 방식은 운영체제와 데스크톱 환경에 따라 다릅니다.

## 프로젝트 구조

| 경로 | 내용 |
| --- | --- |
| `qt/src/` | Qt Widgets UI, 공통 녹화 제어, 인코딩 설정, OS별 권한 처리 |
| `qt/tests/` | 설정 검증 및 MP4 생성·디코딩 검사 |
| `CMakeLists.txt` | 공통 빌드·테스트·배포 설정 |
| `scripts/` | macOS/Linux 및 Windows 빌드 도구 |
| `run-macos.command`, `run-windows.cmd`, `run-linux.sh` | 운영체제별 빌드·실행 진입점 |
| `.github/workflows/qt-build.yml` | Windows·macOS·Linux 빌드·테스트 워크플로 |
| [`docs/`](docs/README.md) | 사용 안내, 배포, 테스트 상세, 문제 해결, 기능 확장 계획, 참고 자료 |
| `archive/swift-macos/` | 이전 Swift 초안의 소스·스크립트·실행 파일 |
| `archive/windows-original/` | 이전 Windows MFC 원본·프로젝트·라이브러리·실행 파일·빌드 결과 |

## 빌드·실행 방법

아래 명령은 저장소 루트에서 실행합니다. 화면 캡처에는 FFmpeg 미디어 플러그인이 포함된 Qt가 필요합니다.
권한 설정과 데스크톱 환경별 요구사항은 [사용 안내](docs/usage.md), 배포 폴더 생성은
[패키징과 배포](docs/packaging.md)를 참고하세요.

### macOS

Xcode Command Line Tools와 CMake 3.24 이상이 필요합니다.
화면 캡처에는 **FFmpeg 미디어 플러그인을 포함한 Qt**가 필요합니다.
현재 확인한 Homebrew Qt 6.12.0은 Darwin 백엔드만 제공하므로 `brew install qt`만으로는
녹화를 실행할 수 없습니다. FFmpeg 명령행 도구만 별도 설치하는 것으로도 해결되지 않습니다.

```sh
xcode-select --install   # 이미 설치되어 있다면 생략
brew install cmake ninja
./run-macos.command
```

`run-macos.command`는 Qt 미디어 플러그인을 확인하고, 필요하면 `aqtinstall`을 통해
공식 Qt 6.8.3 macOS SDK와 Multimedia/Shader Tools를 `.tools/qt/`에 자동 설치합니다.
최초 설치에는 인터넷 연결과 Python 3가 필요합니다. 설치 도구는 `.tools/qt-installer/`에
격리되며 기존 Homebrew Qt는 그대로 유지합니다. Qt SDK를 바꾸면 CMake 캐시를 새로 구성해
서로 다른 Qt 버전이 섞이지 않도록 합니다.

설치만 별도로 실행하려면 `./scripts/setup-macos-qt.sh`를 사용합니다.

Qt Online Installer로 설치했다면 해당 kit 경로를 지정합니다.

```sh
QT_ROOT="$HOME/Qt/6.8.3/macos" ./scripts/build-qt.sh
open build/qt/ClipDock.app
```

### Windows

Windows 10/11 64비트에서 사용할 수 있습니다. Visual Studio 2022 이상 또는 Build Tools에서
**C++를 사용한 데스크톱 개발**과 **Windows용 C++ CMake 도구**를 설치합니다.
Visual Studio 2026을 사용한다면 개별 구성 요소에서 **MSVC v143 - VS 2022 C++ x64/x86 빌드 도구**도
선택합니다. 빌드 스크립트는 Qt kit에 맞는 v143 도구 집합을 자동 선택합니다.
Qt가 없다면 Python 3.9 이상도 필요합니다.

`run-windows.cmd`를 더블 클릭하면 빌드 도구를 찾고, 필요한 경우 공식 Qt 6.8.3 MSVC x64
SDK와 Multimedia/Shader Tools를 프로젝트의 `.tools/qt/`에 설치한 뒤 빌드·패키징·실행합니다.
최초 Qt 설치에는 인터넷 연결이 필요합니다. 기존 Qt와 시스템 PATH는 변경하지 않습니다.

PowerShell에서 빌드·실행하려면:

```powershell
./scripts/build-qt.ps1 -Run
```

이미 Qt를 설치했다면 해당 MSVC x64 kit 경로를 지정할 수 있습니다. 화면 캡처에 필요한
FFmpeg 미디어 플러그인이 포함되어 있어야 하며, MinGW kit와는 혼용하지 않습니다.

```powershell
./scripts/build-qt.ps1 -QtRoot C:/Qt/6.8.3/msvc2022_64 -Run
```

SDK는 `-QtRoot`/`QT_ROOT` → 프로젝트 전용 SDK → `QTDIR` → `C:/Qt/*/msvc2022_64`
순서로 찾습니다. Qt 또는 컴파일러 경로가 바뀌면 CMake 캐시를 새로 구성합니다.
SDK만 설치하려면 `./scripts/setup-windows-qt.ps1`을 실행합니다.

### Linux

Ubuntu 24.04 x86_64를 기준으로 설치·빌드·검사를 제공합니다.
Qt 6.8 이상(FFmpeg Multimedia 플러그인 및 Wayland Client 포함), CMake 3.24 이상과 C++20 컴파일러가 필요합니다.
Ubuntu 기본 Qt가 요구 버전보다 낮으므로 실행 스크립트는 공식 Qt 6.8.3을 프로젝트 내부에 설치합니다.

Ubuntu 24.04에서 필요한 빌드 도구와 런타임을 먼저 설치합니다.

```sh
sudo apt update
sudo apt install build-essential cmake ninja-build python3-venv \
  libgl1-mesa-dev libegl1 libopengl0 libglib2.0-0t64 libpulse0 libasound2t64 \
  libxkbcommon-x11-0 libxcb-cursor0 libxcb-icccm4 libxcb-image0 \
  libxcb-keysyms1 libxcb-render-util0 libxcb-xinerama0 libxcb-xfixes0 \
  libx11-xcb1 libwayland-dev libwayland-client0 libwayland-cursor0 libwayland-egl1 \
  libpipewire-0.3-0
bash run-linux.sh
```

`run-linux.sh`는 Qt Multimedia·Shader Tools·Wayland 모듈을 `.tools/qt/`에 설치하고
`build/qt-linux/ClipDock`을 빌드해 실행합니다. 최초 설치에는 인터넷 연결이 필요하며
설치 도구는 `.tools/qt-installer-linux/`에 격리합니다. 시스템 Qt는 변경하지 않습니다.
Windows 빌드 폴더와 Linux 빌드 폴더를 분리하여 WSL에서도 CMake 캐시가 섞이지 않게 합니다.

기존 공식 Qt kit를 사용하거나 빌드만 실행하려면:

```sh
QT_ROOT="$HOME/Qt/6.8.3/gcc_64" bash run-linux.sh
bash scripts/build-qt.sh -G Ninja
```

SDK만 설치하려면 `bash scripts/setup-linux-qt.sh`를 사용합니다.
자동 설치는 x86_64용입니다. 다른 아키텍처/배포판에서는 해당 환경에 맞는 Qt 6.8 이상 SDK를
`QT_ROOT`로 지정하고 시스템 라이브러리도 배포판 패키지 관리자로 설치하세요.

## 테스트 방법

각 플랫폼의 빌드 환경을 준비한 뒤 저장소 루트에서 실행합니다. 아래 스크립트는 빌드 후 자동 테스트를 실행합니다.

### macOS

```sh
./scripts/test-qt.sh
```

### Windows

```powershell
./scripts/build-qt.ps1 -Test
```

### Linux

```sh
bash scripts/test-qt.sh -G Ninja
```

### Qt 없이 설정 검사

Qt 없이 설정 검사만 실행할 수도 있습니다.

```sh
clang++ -std=c++20 -Wall -Wextra -Werror -pedantic qt/tests/SettingsTests.cpp -Iqt/src -o /tmp/clipdock-settings
/tmp/clipdock-settings
```

자동 테스트별 검증 항목과 수동 확인 범위, GitHub Actions 구성은 [테스트 범위와 CI](docs/testing.md)를 참고하세요.
