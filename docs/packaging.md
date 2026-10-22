# ClipDock 패키징과 배포

[문서 목록](README.md) · [프로젝트 README](../README.md)

아래 명령은 저장소 루트에서 실행합니다. 빌드 환경 준비는 [빌드·실행 방법](../README.md#빌드실행-방법)을 참고하세요.

## GitHub Actions에서 바이너리 생성

[ClipDock desktop build 워크플로](../.github/workflows/qt-build.yml)는 기본적으로 빌드와 테스트만 실행합니다.
릴리즈용 바이너리가 필요할 때 수동 실행의 `build_release` 스위치를 켭니다. 기본값은 `false`입니다.

1. GitHub 저장소의 **Actions → ClipDock desktop build → Run workflow**를 엽니다.
2. 빌드할 브랜치를 선택하고 **Create release binary archives for all platforms (no installers)**를 체크합니다.
3. **Run workflow**를 실행합니다.
4. 실행 결과의 **Artifacts**에서 필요한 플랫폼 파일을 다운로드합니다.

워크플로 파일이 기본 브랜치에 반영되어 있어야 수동 실행 메뉴를 사용할 수 있습니다.
GitHub CLI에서는 다음과 같이 실행합니다.

```sh
gh workflow run qt-build.yml -f build_release=true
```

| 대상 | 빌드 환경 | 아티팩트 안의 바이너리 압축 파일 | 압축 해제 후 실행 경로 |
| --- | --- | --- | --- |
| Windows x64 | Windows Server 2022 / MSVC v143 | `ClipDock-Windows-x64.zip` | `bin/ClipDock.exe` |
| macOS Apple Silicon | macOS 15 / ARM64 | `ClipDock-macOS-arm64.zip` | `ClipDock.app` |
| macOS Intel | macOS 15 / x64 | `ClipDock-macOS-x64.zip` | `ClipDock.app` |
| Linux x64 | Ubuntu 24.04 / GCC | `ClipDock-Linux-x64.tar.gz` | `bin/ClipDock` |

각 대상은 Release 구성으로 빌드·테스트에 성공한 뒤 Qt·FFmpeg 런타임을 포함한 배포 폴더를 압축합니다.
Windows에는 Visual C++ 런타임 DLL도 포함합니다. macOS는 앱 번들 서명과 아키텍처를 확인하고,
Linux는 배포 폴더를 다른 경로로 옮겨 의존성과 X11 앱 시작을 검사한 뒤 압축합니다.
GitHub에서 다운로드한 아티팩트 ZIP 안에 위 바이너리 압축 파일이 들어 있습니다.

`build_release`를 끄거나 push·pull request로 실행하면 바이너리 생성과 업로드를 건너뜁니다.
산출물은 Actions 아티팩트로 제공하며 GitHub Release 게시와 설치파일 생성은 수행하지 않습니다.
macOS Developer ID 서명·공증은 별도 작업이며, 기본 서명은 아래와 같이 ad-hoc입니다.

## macOS

Qt/FFmpeg 런타임을 포함하는 로컬 앱 패키지:

```sh
./scripts/package-macos.sh
open dist/qt/ClipDock.app
```

기본 서명은 로컬 실행용 ad-hoc 서명입니다. 외부 배포용 Developer ID 서명 및 공증은
자동으로 수행하지 않습니다.

## Windows

```powershell
./scripts/build-qt.ps1 -Test -Package
./dist/qt/bin/ClipDock.exe
```

Release 실행에 필요한 Qt·FFmpeg·Visual C++ 런타임 DLL과 플러그인은 `dist/qt/`에 함께 배포합니다.
다른 PC로 옮길 때는 **이 폴더 전체**를 복사하고 `bin/ClipDock.exe`를 실행하세요.
GitHub Actions에서 `build_release`를 켜면 같은 스크립트로 패키징하며,
완료된 실행의 `ClipDock-Windows-x64` 아티팩트에서 받을 수 있습니다.

## Linux

Linux 배포 폴더를 만들려면:

```sh
bash scripts/package-linux.sh -G Ninja
./dist/linux/bin/ClipDock
```

`dist/linux/` 전체를 함께 배포합니다. 이 패키지는 같은 계열·버전의 Linux를 대상으로 하며
glibc, 그래픽 드라이버, PipeWire/포털 등 운영체제 구성 요소까지 포함하는 범용 AppImage는 아닙니다.
GitHub Actions에서 `build_release`를 켜면 Ubuntu 24.04에서 빌드한 tar.gz를
`ClipDock-Linux-x64` 아티팩트로 제공합니다.
압축을 풀면 실행 권한과 라이브러리 심볼릭 링크가 유지됩니다.
