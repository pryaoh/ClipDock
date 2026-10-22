# ClipDock — C++ / Qt 6

## 소스 구성

| 경로 | 내용 |
| --- | --- |
| `qt/src/` | Qt Widgets UI, 공통 녹화 제어, 인코딩 설정, OS별 권한 처리 |
| `qt/tests/` | 설정 검증 및 MP4 생성·디코딩 검사 |
| `CMakeLists.txt` | 공통 빌드·테스트·배포 설정 |
| `scripts/` | macOS/Linux 및 Windows 빌드 도구 |
| `archive/swift-macos/` | 이전 Swift 초안의 소스·스크립트·실행 파일 |
| `archive/windows-original/` | 이전 Windows MFC 원본·프로젝트·라이브러리·실행 파일·빌드 결과 |

기능 확장 계획은 [Todo.md](Todo.md)에 정리되어 있습니다.

## 구현 범위

- 디스플레이 또는 창 선택, 미리보기, 녹화 시작/중지
- macOS/Windows에서 `창 → 클릭으로 선택`으로 화면에 보이는 창을 직접 지정
- `영역 → 모니터 선택 → 영역 지정`에서 사각형을 드래그하여 해당 부분만 미리보기·녹화
- 일시정지·재개, 경과 시간 표시
- 원본 비율을 유지하는 최대 출력 크기, FPS, 비트레이트 설정
- H.264 또는 MPEG-4 비디오 / MP4 컨테이너
- 선택한 오디오 입력 장치 녹음, AAC 오디오
- 기존 파일 덮어쓰기 방지, 임시 파일에서 녹화 완료 후 최종 이름으로 이동
- 창을 닫을 때 녹화 파일 마무리를 기다린 후 종료
- 앱 내 단축키: Ctrl/⌘+Shift+R 시작, +P 일시정지/재개, +S 중지

## macOS

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

최초 캡처 시 시스템 설정 → 개인정보 보호 및 보안 → 화면 및 시스템 오디오 기록에서
ClipDock를 허용합니다. 필요하면 앱을 완전히 종료하고 다시 실행합니다.
Swift 초안과 Qt 앱의 권한 항목은 서로 다릅니다.

권한 스위치가 켜져 있는데 미리보기가 거부되면
[macOS 권한 문제 해결](TroubleShooting.md)을 참고하세요.

Qt/FFmpeg 런타임을 포함하는 로컬 앱 패키지:

```sh
./scripts/package-macos.sh
open dist/qt/ClipDock.app
```

기본 서명은 로컬 실행용 ad-hoc 서명입니다. 외부 배포용 Developer ID 서명 및 공증은
자동으로 수행하지 않습니다.

## Windows

Windows 10/11 64비트에서 사용할 수 있습니다. Visual Studio 2022 이상 또는 Build Tools에서
**C++를 사용한 데스크톱 개발**과 **Windows용 C++ CMake 도구**를 설치합니다.
Visual Studio 2026을 사용한다면 개별 구성 요소에서 **MSVC v143 - VS 2022 C++ x64/x86 빌드 도구**도
선택합니다. 빌드 스크립트는 Qt kit에 맞는 v143 도구 집합을 자동 선택합니다.
Qt가 없다면 Python 3.9 이상도 필요합니다.

`run-windows.cmd`를 더블 클릭하면 빌드 도구를 찾고, 필요한 경우 공식 Qt 6.8.3 MSVC x64
SDK와 Multimedia/Shader Tools를 프로젝트의 `.tools/qt/`에 설치한 뒤 빌드·패키징·실행합니다.
최초 Qt 설치에는 인터넷 연결이 필요합니다. 기존 Qt와 시스템 PATH는 변경하지 않습니다.

PowerShell에서 빌드·검사·패키징하려면:

```powershell
./scripts/build-qt.ps1 -Test -Package
./dist/qt/bin/ClipDock.exe
```

이미 Qt를 설치했다면 해당 MSVC x64 kit 경로를 지정할 수 있습니다. 화면 캡처에 필요한
FFmpeg 미디어 플러그인이 포함되어 있어야 하며, MinGW kit와는 혼용하지 않습니다.

```powershell
./scripts/build-qt.ps1 -QtRoot C:/Qt/6.8.3/msvc2022_64 -Test -Package
./dist/qt/bin/ClipDock.exe
```

SDK는 `-QtRoot`/`QT_ROOT` → 프로젝트 전용 SDK → `QTDIR` → `C:/Qt/*/msvc2022_64`
순서로 찾습니다. Qt 또는 컴파일러 경로가 바뀌면 CMake 캐시를 새로 구성합니다.
SDK만 설치하려면 `./scripts/setup-windows-qt.ps1`을 실행합니다.

Release 실행에 필요한 Qt·FFmpeg·Visual C++ 런타임 DLL과 플러그인은 `dist/qt/`에 함께 배포합니다.
다른 PC로 옮길 때는 **이 폴더 전체**를 복사하고 `bin/ClipDock.exe`를 실행하세요.
GitHub Actions의 Windows 빌드도 같은 스크립트로 검사·패키징하며,
완료된 실행의 `ClipDock-Windows-x64` 아티팩트에서 받을 수 있습니다.
마이크가 동작하지 않으면 Windows 설정 → 개인정보 및 보안 → 마이크에서
데스크톱 앱의 마이크 접근을 허용합니다. 실제 화면·창 캡처에는 로컬 대화형 데스크톱이 필요합니다.

## Linux

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

기존 공식 Qt kit를 사용하거나 빌드·검사만 실행하려면:

```sh
QT_ROOT="$HOME/Qt/6.8.3/gcc_64" bash run-linux.sh
bash scripts/test-qt.sh -G Ninja
```

SDK만 설치하려면 `bash scripts/setup-linux-qt.sh`를 사용합니다.
자동 설치는 x86_64용입니다. 다른 아키텍처/배포판에서는 해당 환경에 맞는 Qt 6.8 이상 SDK를
`QT_ROOT`로 지정하고 시스템 라이브러리도 배포판 패키지 관리자로 설치하세요.

- **X11:** 화면·창 목록 선택, 영역 드래그 녹화를 사용합니다. 창을 직접 클릭하는 선택기는 macOS/Windows용입니다.
- **Wayland:** 미리보기 또는 녹화 시작 후 시스템 공유 창에서 대상을 선택합니다.
  앱의 모니터 목록과 영역 지정은 사용하지 않습니다. 공유 선택 창을 기다리는 동안 자동으로 시간 초과하지 않으며
  앱의 중지 버튼으로 취소할 수 있습니다. Qt 6.8의 Wayland 화면 캡처는 실험적 기능입니다.
- Wayland에는 실행 중인 PipeWire와 데스크톱에 맞는 ScreenCast 포털 백엔드가 필요합니다.
  Ubuntu GNOME은 `pipewire`, `xdg-desktop-portal`, `xdg-desktop-portal-gnome` 패키지를 사용합니다.
  KDE는 KDE 포털 백엔드를 사용하세요. 설치 후 로그아웃·로그인하여 사용자 서비스를 시작합니다.
- 마이크는 PulseAudio 또는 PipeWire의 PulseAudio 호환 서비스(`pipewire-pulse`)를 사용합니다.
  실제 화면 공유는 그래픽 데스크톱 세션에서 실행해야 합니다.

Linux 배포 폴더를 만들려면:

```sh
bash scripts/package-linux.sh -G Ninja
./dist/linux/bin/ClipDock
```

`dist/linux/` 전체를 함께 배포합니다. 이 패키지는 같은 계열·버전의 Linux를 대상으로 하며
glibc, 그래픽 드라이버, PipeWire/포털 등 운영체제 구성 요소까지 포함하는 범용 AppImage는 아닙니다.
CI는 Ubuntu 24.04에서 빌드한 `ClipDock-Linux-x64` 아티팩트를 tar.gz로 제공합니다.
압축을 풀면 실행 권한과 라이브러리 심볼릭 링크가 유지됩니다.

## 검사

```sh
./scripts/test-qt.sh
```

`settings`는 입력값, 정수 오버플로 방지, Retina·울트라와이드·세로 화면 크기를 검사합니다.
macOS의 `macos_app_signing`은 앱 번들 서명, 앱 ID 일치, 변경 없는 서명의 유지,
Info.plist 변경 뒤 서명 복구를 실제 `codesign`으로 검사합니다.
`window_picker`는 같은 제목/제목 없는 창의 ID 매칭(macOS/Windows), 목록 순서 변경과
대상 소실, Dock·시스템 창 제외, 겹친 창의 순서, 음수 모니터 좌표와 DPI 배율 변환,
누르기 없는 마우스 릴리스, Esc·오른쪽 클릭·창 닫기로 취소, 오버레이 정리를 검사합니다.
`media_encode_decode`는 화면/마이크 권한 없이 합성 프레임으로 실제 MP4를 생성한 뒤
재생하여 해상도, 프레임 시간 순서, 영상 길이를 검사합니다.
`region_picker`는 Retina·소수 배율의 좌표 변환, 영역 밖 픽셀 제외, 양방향 드래그,
최소 크기, 모니터 경계 제한, 취소와 오버레이 정리를 검사합니다.
`region_recording`은 OS 화면 입력만 합성 프레임으로 대체하고 실제 녹화 컨트롤러의
영역 미리보기 → MP4 저장 → 디코딩을 검사합니다. 영역 밖 픽셀 제외, 출력 크기,
시간 순서와 일시정지 시간 제외도 확인합니다.
`recording_capture_failure`는 캡처 중단 전 영상의 저장·재생을,
`recording_delayed_permission`은 권한 응답 대기 중 미리보기와 승인 후 녹화를 검사합니다.
`recorder_lifecycle`은 권한 거부·취소·늦은 응답, 대상 소실, 인코더 오류 시 임시 파일 보존을 검사합니다.
`main_window`는 저장 대화상자가 열린 동안 대상 목록이 바뀌어도 원래 선택을 유지하는지 검사합니다.
Linux에서 `xvfb`와 `xauth`를 설치하면 `linux_screen_capture`가 가상 X11 화면의 실제 캡처 프레임도 검사합니다.
Wayland 포털 승인 및 실제 마이크 입력은 그래픽 데스크톱에서 별도로 확인해야 합니다.
헤드리스 환경에서도 가능한 소프트웨어 MPEG-4 코덱을 사용하며 H.264 하드웨어 인코더 검증을
대체하지 않습니다.

Qt 없이 설정 검사만 실행할 수도 있습니다.

```sh
clang++ -std=c++20 -Wall -Wextra -Werror -pedantic qt/tests/SettingsTests.cpp -Iqt/src -o /tmp/clipdock-settings
/tmp/clipdock-settings
```

Windows·macOS·Linux 빌드 및 검사 워크플로는 `.github/workflows/qt-build.yml`에 있습니다.
GitHub에 등록하면 코드 변경을 포함한 push·pull request에서 실행되며, Actions 화면에서
수동 실행할 수도 있습니다. Qt 6.8.3과 Multimedia·Shader Tools를 설치해 빌드하고
화면 권한이 필요 없는 자동 검사를 실행합니다. OS 권한 승인과 실제 화면·오디오 캡처는
각 플랫폼에서 별도로 확인해야 합니다.



## 참고

- [Qt Multimedia](https://doc.qt.io/qt-6.8/qtmultimedia-index.html)
- [Qt 화면 캡처 예제](https://doc.qt.io/qt-6.8/qtmultimedia-screencapture-example.html)
- [Qt Linux 캡처 요구사항](https://doc.qt.io/qt-6.8/qtmultimedia-linux.html)

Qt와 FFmpeg 등 배포되는 외부 라이브러리의 라이선스 및 고지문은 각 배포물의 내용을 따릅니다.
