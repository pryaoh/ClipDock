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
| `main_window_capture_affinity` (Windows) | 대기/미리보기/중지에 따른 실제 창 제외·해제, 옵션 끄기, 앱 소유 대화상자, 네이티브 창 변경 후 재적용, 구버전 Windows의 대체 동작 |
| `global_shortcuts` | 키별 등록 성공/충돌, 앱 내 대체 경로, 중복·반복 입력 억제, 비동기 전달, 재등록 시 이전 명령 무효화 |
| `native_global_shortcuts` (Windows) | 실제 OS 키 등록 충돌, 메시지 수신, 재등록, 소유한 등록만 해제하는지 검증 |
| `mac_shortcut_keys` (macOS) | 시스템에 포함된 US·Dvorak 배열의 실제 키 변환 데이터와 배열 소실 시 대체 경로 검증 |
| `x11_global_shortcuts` (Linux) | Xvfb/XTest로 다른 X 클라이언트에 포커스를 둔 키 입력, 키 반복, Caps Lock, 충돌과 등록 해제 검증 |
| `portal_global_shortcuts` (Linux) | 격리된 D-Bus 서비스로 일부 승인·취소·미지원, 시스템 지정 키 표시, 반복 입력, 늦은 응답과 세션 정리 검증 |
| `linux_screen_capture` (Linux) | `xvfb`와 `xauth` 설치 시 가상 X11 화면의 실제 캡처 프레임 |

## 수동 확인 범위

Windows 그래픽 데스크톱에서는 `main_window_tests --capture-exclusion`으로 실제 화면 캡처를
추가 검사할 수 있습니다 (`QT_QPA_PLATFORM=windows`, `QT_MEDIA_BACKEND=ffmpeg`).
대기 상태에서 메인 창의 자홍색 표식이 보이는 것을 확인한 다음, 미리보기 상태로 전환해
제품 코드가 제외 설정을 적용하게 하고 뒤의 녹색 창이 연속 3프레임에 나타나는지 검사합니다.
중지 후에는 다시 원래 표식이 캡처되는지 확인합니다. Windows 10 버전 2004 이상에서 실행하세요.
화면 이미지는 저장하지 않습니다. 이 검사는 실제 데스크톱을 사용하므로 기본 CTest에서는 실행하지 않습니다.

`main_window`는 임시 INI 설정 저장소를 사용해 두 옵션의 기본값과 재생성을 통한 복원을 검사합니다.
offscreen 환경에서 화면/영역 겹침, 이동 후 미리보기 복원, 포털의 알 수 없는 대상,
권한 대기·시작 실패·종료·수동 최소화·최대화 복원 및 대체 동작의 일시정지/재개 명령을 검사합니다.
`main_window_startup_policy`는 화면 권한·공유 선택 중 창 유지와 인코더 시작 직전 최소화,
`main_window_move_into_capture`는 녹화 영역 밖에서 안으로 이동할 때의 일시정지 요청을 검사합니다.
실제 합성 영상 파이프라인 테스트는 녹화 준비 신호가 권한 승인·첫 프레임·출력 준비 후 한 번만 발생하고
미리보기나 재개 시에는 발생하지 않는지도 검사합니다.
이 검사는 macOS/Wayland 데스크톱의 실제 창 최소화·복원 타이밍 검증을 대신하지 않습니다.

Wayland 포털 승인 및 실제 마이크 입력은 그래픽 데스크톱에서 별도로 확인해야 합니다.
전역 단축키의 Windows/macOS 실제 키 입력과 Wayland 데스크톱 승인 UI도 실제 환경에서 확인해야 합니다.
Windows 자동 테스트는 실제 등록과 `WM_HOTKEY` 메시지 전달을 검사하며 물리 키 입력을 대신하지 않습니다.
X11 테스트는 첫 입력, 보조 키를 놓은 상태의 오입력 차단, 겹친 단축키 이후의 정상 입력도 확인합니다.
Wayland 테스트는 창 활성화 토큰의 비동기 전달도 확인하지만, 실제 창 포커스 전환은 데스크톱에서 확인해야 합니다.
포털 테스트는 프로토콜과 앱 상태 처리를 검증하며 모든 데스크톱 백엔드의 동작을 보장하지 않습니다.
`main_window`는 저장 창 재진입, 모달 대화상자 및 파일 마무리 상태의 단축키 차단도 검증합니다.
Linux 단축키 테스트에는 `libxtst-dev`, `xvfb`, `xauth`, `dbus-daemon`이 필요합니다.
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
