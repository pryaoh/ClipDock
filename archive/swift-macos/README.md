# Swift macOS prototype archive

2026-10-16에 C++ / Qt 6 전환을 위해 보관한 macOS 전용 초안입니다.
현재 개발 대상은 저장소 루트의 `CMakeLists.txt`와 `qt/`입니다.

- `macOS/`: Swift Package 소스, 설정, 인코더 검사 코드와 기존 빌드 캐시
- `scripts/`: 기존 Swift 빌드·검사 스크립트
- `run-macos.command`: 기존 Swift 실행 진입점
- `dist/ScreenEncoder.app`: 보관 시점에 빌드한 Swift 앱

보관 시점의 검증: Apple Silicon macOS에서 앱 빌드 및 UI 실행 확인.
실제 화면 녹화는 검증하지 않았습니다. 합성 미디어 검사에서 AVFoundation의
`Cannot Encode (-11834)`가 발생했으며, 해결되지 않은 상태로 보관했습니다.
보관된 앱은 마지막 소스 수정 전의 빌드일 수 있습니다.

다시 빌드하려면 이 폴더에서 `./run-macos.command`를 실행합니다.
폴더 이동으로 기존 Swift 빌드 캐시의 절대 경로가 달라져 문제가 생기면
`swift package --package-path macOS reset` 후 다시 빌드합니다.
Swift 앱과 Qt 앱은 서로 다른 bundle identifier를 사용합니다.
