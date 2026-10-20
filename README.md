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

Visual Studio 2022 C++ 빌드 도구, CMake, Qt 6.8 이상 MSVC kit 및 Multimedia 모듈을 설치합니다.

```powershell
./scripts/build-qt.ps1 -QtRoot C:/Qt/6.8.3/msvc2022_64 -Test -Package
./dist/qt/bin/ClipDock.exe
```

## Linux

Qt 6.8 이상과 Multimedia 모듈, CMake, C++20 컴파일러가 필요합니다.
배포판의 Qt가 6.8보다 낮으면 Qt Online Installer에서 데스크톱 kit를 설치합니다.

```sh
QT_ROOT="$HOME/Qt/6.8.3/gcc_64" ./scripts/build-qt.sh
./build/qt/ClipDock
```

Wayland에서는 PipeWire 및 데스크톱 환경에 맞는 `xdg-desktop-portal` 백엔드도 설치되어 있어야 합니다.
Linux 배포 시 Qt 외에 FFmpeg 및 시스템 라이브러리 의존성도 확인해야 합니다.

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
