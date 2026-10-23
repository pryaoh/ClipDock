# ClipDock 테스트 범위와 CI

[문서 목록](README.md) · [프로젝트 README](../README.md)

테스트 실행 명령은 [README의 테스트 방법](../README.md#테스트-방법)에 정리되어 있습니다.

## 자동 테스트 범위

| 테스트 | 검증 항목 |
| --- | --- |
| `settings` | 입력값, 정수 오버플로 방지, Retina·울트라와이드·세로 화면 크기 |
| `macos_app_signing` (macOS) | 실제 `codesign`으로 앱 번들 서명, 앱 ID 일치, 변경 없는 서명의 유지, Info.plist 변경 뒤 서명 복구 |
| `window_picker` | 같은 제목/제목 없는 창의 ID 매칭(macOS/Windows), 목록 순서 변경과 대상 소실, Dock·시스템 창 제외, 겹친 창의 순서, 음수 모니터 좌표와 DPI 배율 변환, 누르기 없는 마우스 릴리스, Esc·오른쪽 클릭·창 닫기로 취소, 오버레이 정리 |
| `media_encode_decode` | 화면/마이크 권한 없이 합성 프레임으로 실제 MP4 생성·재생, 해상도, 프레임 시간 순서, 영상 길이 |
| `region_picker` | Retina·소수 배율의 좌표 변환, 영역 밖 픽셀 제외, 양방향 드래그, 최소 크기, 모니터 경계 제한, 취소와 오버레이 정리 |
| `region_picker_multiple_screens` | 왼쪽·오른쪽·위쪽 모니터와 100/125/150/200% 혼합 배율에서 선택 창 위치 및 화면 내부 좌표 검증 |
| `main_window_multiple_screens` | 모니터 사전 선택 없이 모든 화면에서 영역 드래그, 새 모니터의 대상 목록 반영, 녹화 대상 자동 변경, 전체 오버레이 숨김·정리, 취소 시 기존 영역 유지 |
| `region_recording` | OS 화면 입력만 합성 프레임으로 대체하고 실제 녹화 컨트롤러의 영역 미리보기 → MP4 저장 → 디코딩 검사, 영역 밖 픽셀 제외, 출력 크기, 시간 순서, 일시정지 시간 제외 |
| `recording_capture_failure` | 캡처 중단 전 영상의 저장·재생 |
| `recording_delayed_permission` | 권한 응답 대기 중 미리보기와 승인 후 녹화 |
| `recorder_lifecycle` | 권한 거부·취소·늦은 응답, 대상 소실, 인코더 오류 시 임시 파일 보존 |
| `main_window` | 저장 대화상자가 열린 동안 대상 목록이 바뀌어도 원래 선택 유지 |
| `linux_screen_capture` (Linux) | `xvfb`와 `xauth` 설치 시 가상 X11 화면의 실제 캡처 프레임 |

## 수동 확인 범위

Wayland 포털 승인 및 실제 마이크 입력은 그래픽 데스크톱에서 별도로 확인해야 합니다.
헤드리스 환경에서도 가능한 소프트웨어 MPEG-4 코덱을 사용하며 H.264 하드웨어 인코더 검증을
대체하지 않습니다.

## GitHub Actions

Windows·macOS·Linux 빌드 및 검사 워크플로는 [`.github/workflows/qt-build.yml`](../.github/workflows/qt-build.yml)에 있습니다.
`qt/`, `scripts/`, CMake 설정, 실행 스크립트 또는 해당 워크플로의 변경을 포함한 push·pull request에서
자동 실행되며, Actions 화면에서 수동 실행할 수도 있습니다. 문서만 변경한 push·pull request는 실행 대상이 아닙니다.

| 대상 | GitHub 호스팅 러너 | 기본 작업 |
| --- | --- | --- |
| Windows x64 | `windows-2022` | Release 빌드와 CTest |
| macOS ARM64 | `macos-15` | 네이티브 ARM64 Release 빌드와 CTest |
| macOS x64 | `macos-15-intel` | 네이티브 x64 Release 빌드와 CTest |
| Linux x64 | `ubuntu-24.04` | Release 빌드와 CTest, Xvfb를 이용한 X11 캡처 검사 |

Qt 6.8.3과 Multimedia·Shader Tools를 설치하고 각 대상에 맞는 Qt kit를 명시합니다.
macOS 빌드는 러너의 CPU 아키텍처를 CMake에 전달하며, 두 아키텍처의 바이너리를 각각 생성합니다.
한 대상이 실패해도 나머지 대상의 검사는 계속 진행합니다.

수동 실행의 `build_release`는 기본값이 `false`입니다. 켜면 해당 대상의 빌드·테스트가 통과한 뒤
배포 폴더 생성과 플랫폼별 바이너리 압축·업로드를 추가로 실행합니다.
사용법과 산출물 목록은 [패키징과 배포](packaging.md#github-actions에서-바이너리-생성)를 참고하세요.

OS 권한 승인과 실제 화면·오디오 캡처는 각 플랫폼에서 별도로 확인해야 합니다.
