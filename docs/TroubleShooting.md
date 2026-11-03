# ClipDock 문제 해결

[문서 목록](README.md) · [프로젝트 README](../README.md)

이 문서의 명령과 경로는 저장소 루트를 기준으로 합니다.

## macOS: Qt 설치 중단 또는 빌드 구성 실패

`.tools/qt/`에 폴더가 있다고 설치가 완료된 것은 아닙니다. `qmake`, Qt CMake 구성,
Multimedia 구성 및 FFmpeg 플러그인을 함께 확인합니다. `./scripts/test-qt.sh`도
`./run-macos.command`와 같은 빌드 경로를 사용해 필요한 SDK를 설치합니다.
불완전한 SDK를 `QT_ROOT`로 지정했다면 `unset QT_ROOT` 후 다시 실행하세요.

최신 Qt에서 `Qt6::MultimediaPrivate`를 찾지 못하던 CMake 설정은 별도 private 패키지를
명시적으로 찾도록 처리했습니다. Homebrew Qt에 FFmpeg 백엔드가 없으면 이 구성 문제가
해결되어도 화면 녹화는 불가능하므로 공식 Qt SDK를 사용해야 합니다.

## macOS: 권한 스위치는 켜져 있는데 미리보기가 거부되는 경우

`도움말 → 권한 상태 확인`에서 **현재 프로세스의 화면 기록 허용 여부**, 앱 ID,
실행 중인 `.app` 경로를 확인합니다. 기본 실행 대상은 `build/qt/ClipDock.app`,
앱 ID는 `local.clipdock.qt`이며, 표시 이름과 실행 파일은 `ClipDock`입니다.
이전 ScreenEncoder 앱과 ID가 달라 화면 기록·마이크 권한은 새 앱에서 다시 허용해야 합니다.
이전 Swift 앱이나 검증용
`ScreenEncoderPickerCheck`의 권한을 켜는 것으로 기본 앱의 권한을 대신할 수 없습니다.

macOS 빌드는 실행 파일에만 붙는 링커 서명을 앱 번들 전체 서명으로 교체합니다.
`Info.plist`와 리소스를 포함해 서명을 검증하고 앱 ID와 서명 ID를 일치시킵니다.
변경 없는 앱의 유효한 서명은 유지합니다. 이전에 만든 번들도 `run-macos.command`로
다시 빌드하면 서명을 확인·복구합니다.

서명 복구 후에도 거부되면 앱을 완전히 종료하고, 시스템 설정의 **화면 및 시스템
오디오 녹음**에서 기존 ClipDock 또는 ScreenEncoder 항목을 제거한 뒤 `+`로 위 기본 `.app`을
다시 추가·허용하고 재실행합니다. 이 작업은 사용자가 직접 승인해야 하며,
빌드 스크립트는 macOS 권한 데이터베이스나 다른 앱의 권한을 변경하지 않습니다.

기본값은 로컬 ad-hoc 서명입니다. 코드가 바뀌면 macOS가 새 빌드로 판단하여 다시
허용해야 할 수 있습니다. 개발용 서명 인증서가 있다면 같은 인증서를 계속 사용합니다.
이름 변경 이전의 `ScreenEncoder.app`이 `build/` 또는 `dist/`에 남아 있으면 새 앱과
혼동하지 않도록 `ClipDock.app`을 실행하세요. 새 실행 스크립트는 `ClipDock.app`을 엽니다.

```sh
CODE_SIGN_IDENTITY="Apple Development: 이름 (인증서 ID)" ./run-macos.command
```

설정 페이지의 이름에 '시스템 오디오'가 포함되어 있어도, 현재 버전은 시스템 소리
직접 캡처를 구현하지 않았습니다. 오디오는 선택한 마이크 또는 루프백 입력 장치로만
녹음하며, 마이크 접근 허용은 별도의 권한입니다.

## macOS: 배포 서명에서 `resource fork, Finder information` 오류

빌드와 패키징은 서명 전에 `com.apple.FinderInfo`와 `com.apple.ResourceFork`만 정리합니다.
quarantine·provenance 등 다른 확장 속성은 지우지 않습니다.
Dropbox File Provider가 메타데이터를 즉시 다시 만드는 환경에서는 동기화 폴더 안의 앱을
서명하는 것만으로 해결되지 않습니다. 패키징 스크립트는 `~/Library/CloudStorage/` 경로를
감지해 로컬 임시 폴더로 옮겨 패키징하고, 결과를 `dist/ClipDock-macOS.zip`으로 보관합니다.
ZIP을 동기화 폴더 밖에 풀어서 실행하세요.
