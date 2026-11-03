# ClipDock 테스트 범위와 CI

[문서 목록](README.md) · [프로젝트 README](../README.md)

테스트 실행 명령은 [README의 테스트 방법](../README.md#테스트-방법)에 정리되어 있습니다.

## 자동 테스트 범위

| 테스트 | 검증 항목 |
| --- | --- |
| `settings` | 입력값, 정수 오버플로 방지, Retina·울트라와이드·세로 화면 크기 |
| `qt_sdk_detection` | OS별 SDK 선택, 공백 경로, 불완전한 설치 제외, macOS에서 FFmpeg 없는 명시적 SDK의 빌드 차단 |
| `macos_app_signing` (macOS) | 실제 `codesign`으로 앱 번들 서명, 앱 ID 일치, 변경 없는 서명의 유지, Info.plist 변경 뒤 서명 복구, Finder 메타데이터 정리와 다른 확장 속성 보존 |
| `window_picker` | 같은 제목/제목 없는 창의 ID 매칭(macOS/Windows), 목록 순서 변경과 대상 소실, Dock·시스템 창 제외, 겹친 창의 순서, 음수 모니터 좌표와 DPI 배율 변환, 누르기 없는 마우스 릴리스, Esc·오른쪽 클릭·창 닫기로 취소, 오버레이 정리 |
| `media_encode_decode` | 화면/마이크 권한 없이 합성 프레임으로 실제 MP4 생성·재생, 해상도, 프레임 시간 순서, 영상 길이 |
| `region_picker` | Retina·소수 배율의 좌표 변환, 영역 밖 픽셀 제외, 양방향 드래그, 최소 크기, 모니터 경계 제한, 취소와 오버레이 정리 |
| `region_picker_multiple_screens` | 왼쪽·오른쪽·위쪽 모니터와 100/125/150/200% 혼합 배율에서 선택 창 위치 및 화면 내부 좌표 검증 |
| `main_window_multiple_screens` | 모니터 사전 선택 없이 모든 화면에서 영역 드래그, 새 모니터의 대상 목록 반영, 녹화 대상 자동 변경, 전체 오버레이 숨김·정리, 취소 시 기존 영역 유지 |
| `region_recording` | OS 화면 입력만 합성 프레임으로 대체하고 실제 녹화 컨트롤러의 영역 미리보기 → MP4 저장 → 디코딩 검사, 영역 밖 픽셀 제외, 출력 크기, 시간 순서, 일시정지 시간 제외 |
| `recording_capture_failure` | 캡처 중단 전 영상의 저장·재생 |
| `recording_delayed_permission` | 권한 응답 대기 중 미리보기와 승인 후 녹화 |
| `recording_fps25`, `recording_fps60` | 입력 캡처 주기와 다른 출력 FPS, 저장 파일의 각 프레임 시각·평균 FPS, 일시정지 시간 제외 |
| `recording_fps30_static` | 새 캡처가 없는 화면도 30 FPS 유지, 반복 프레임 전송이 이전 프레임의 시각 정보를 변경하지 않는지 검사 |
| `recording_fps25_full_frame` | 영역 자르기가 없는 전체 프레임도 동일한 25 FPS 저장 경로 사용 |
| `recorder_lifecycle` | 권한 거부·취소·늦은 응답, 대상 소실, 인코더 오류 시 임시 파일 보존 |
| `main_window` | 저장 대화상자가 열린 동안 대상 목록이 바뀌어도 원래 선택 유지 |
| `linux_screen_capture` (Linux) | `xvfb`와 `xauth` 설치 시 가상 X11 화면의 실제 캡처 프레임 |

## 수동 확인 범위

macOS의 일반 CTest는 `offscreen` 플랫폼과 합성 프레임을 사용하므로 실제 Cocoa 창,
화면 기록 권한, 실제 화면·창·영역 캡처가 정상이라는 증거가 아닙니다.
macOS 변경 후에는 다음 절차도 확인합니다.

1. `./run-macos.command`로 `build/qt/ClipDock.app`을 실행하고 창과 입력 장치 목록을 확인합니다.
2. `도움말 → 권한 상태 확인`에서 실행 경로와 화면 기록 권한을 확인합니다.
3. 디스플레이·창·영역별로 미리보기를 시작하고 실제 화면이 갱신되는지 확인합니다.
4. MP4 녹화 → 일시정지 → 재개 → 저장을 실행하고 생성한 파일을 재생합니다.
5. `./scripts/package-macos.sh`가 출력하는 앱 경로도 별도로 실행합니다. 동기화 폴더의 경우
   로컬 임시 폴더에서 서명하며 자세한 경로는 [패키징 안내](packaging.md#macos)를 참고합니다.

화면 기록 권한이 거부되면 이를 성공으로 처리하지 않고 미검증 범위로 남깁니다.
새 ad-hoc 빌드는 기존 권한이 이어지지 않을 수 있으며 사용자 승인이 필요합니다.

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

## macOS 로컬 실행 확인 (2026-11-02)

macOS 26.5.2 / Apple Silicon / 공식 Qt 6.8.3에서 확인했습니다.

- `./scripts/test-qt.sh`: 13개 테스트 통과.
- Cocoa 앱 실행, 실제 화면·창·640×480 영역 미리보기 확인.
- 사용자가 현재 앱의 화면 기록 권한을 다시 등록한 뒤 실제 캡처가 시작됨.
- H.264 전체 화면: 일시정지·재개·저장 후 AVFoundation으로 716프레임 모두 디코딩
  (1920×1080, 12.3초).
- H.264 영역: 지정한 640×480으로 저장 후 195프레임 모두 디코딩 (10.152초).
- 기본 경로 및 `PACKAGE_DIR=dist/qt`로 패키징·엄격 서명 검증 통과.
  생성한 ZIP을 동기화 폴더 밖에 풀어 서명 재검증 및 독립 앱 실행 확인.

초기 실측에서 25 FPS 설정과 달리 전체 화면 약 58.21 FPS, 영역 약 19.21 FPS가 확인되어
후속 수정했습니다. 화면·창·영역을 공통 프레임 입력 경로로 연결하고, 프레임 번호 기준의
고정 시각으로 저장합니다. 정지 화면에서는 마지막 이미지를 재사용하되 각 프레임의
시각 정보는 독립적으로 생성합니다. FPS 및 프레임 공유 회귀 검사를 포함한 **17개 테스트가
통과**했습니다. 현재 빌드 앱을 macOS 앱 목록과 화면 기록 권한에 다시 등록한 뒤,
실제 H.264 녹화 파일을 AVFoundation으로 전부 디코딩하여 다음을 확인했습니다.

| 녹화 대상 | 설정 FPS | 실제 FPS | 해상도 | 프레임 수 | 길이 |
| --- | ---: | ---: | --- | ---: | ---: |
| 전체 화면 | 25 | 25.0 | 1920×1080 | 221 | 8.84초 |
| 영역 | 25 | 25.0 | 640×480 | 181 | 7.24초 |
| 영역 | 60 | 60.0 | 640×480 | 554 | 9.234초 |

모든 프레임의 표시 시각이 `프레임 번호 / 설정 FPS`와 1ms 이내로 일치하고,
프레임 수를 파일 길이로 나눈 평균 FPS도 설정값과 0.1 FPS 이내로 일치했습니다.
검증 파일은 `~/Movies/:tmp:clipdock-fps25-screen.mp4`,
`~/Movies/clipdock-fps25-region.mp4`, `~/Movies/clipdock-fps60-region.mp4`입니다.
실제 마이크 녹음과 다중 물리 모니터 간 영역 선택은 이번 수동 검증에 포함하지 않았습니다.
