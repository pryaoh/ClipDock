# Recording Preferences Implementation Plan

> **For agentic workers:** Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** 앱 재실행과 모니터 변경에 안전한 설정/영역 복원.
**Architecture:** Qt QSettings 기반 RecordingPreferences 모듈, MainWindow 이벤트 연결 및 복원.
**Tech Stack:** C++20, Qt 6.8 Core/Gui/Multimedia/Widgets, CTest.
**Spec:** docs/superpowers/specs/2026-11-04-recording-preferences-design.md

## Global Constraints
- 사용자 설정과 실제 녹화 파일을 테스트에서 변경하지 않는다.
- 모니터 인덱스 및 창 핸들을 영구 식별자로 사용하지 않는다.
- 기본값: 긴 변 1920, FPS 25, 비트레이트 5000, 녹음 안 함.

## Review Focus
- 저장 폴더 삭제/손상 및 파일 대화상자 취소 시 경로 처리.
- 중복/빈 모니터 식별자와 순서 변경 시 잘못된 화면 복원 금지.
- 같은 논리 크기에서 배율만 변경된 경우 영역 무효화.
- 초기화 뒤 영역 재진입/재실행 시 이전 영역 부활 금지.
- 대상/오디오 목록 갱신 중 임시 선택 신호가 저장값을 덮어쓰지 않음.

### Task 1: 저장 및 검증 모듈
**Files:** qt/src/RecordingPreferences.h/.cpp, qt/tests/RecordingPreferencesTests.cpp, CMakeLists.txt.
**Interfaces:** RecordingPreferences::load/save/reset, ScreenDescriptor, SavedRegion::matchingScreen.
- [x] 임시 저장소 테스트를 작성하고 미구현 실패 확인.
- [x] 항목별 검증, 장치/코덱 가용성, 화면 매칭 구현.
- [x] recording_preferences CTest 통과 확인.

### Task 2: UI 복원 및 초기화
**Files:** qt/src/MainWindow.h/.cpp, qt/tests/MainWindowTests.cpp, CMakeLists.txt.
**Interfaces:** 위 모듈을 사용하며 restorePreferences/savePreferences/resetPreferences와 영역 선택 저장을 연결한다.
- [x] UI 재생성/폴더/영역/초기화 회귀 테스트 실패 확인.
- [x] 목록 변경 신호 보호와 초기화 UI, 화면 구성 변경 감지를 구현.
- [x] 관련 테스트와 전체 scripts/test-qt.sh 통과 확인.

### Task 3: 병합 정리 및 검증 기록
**Files:** docs/Todo.md, docs/usage.md, docs/testing.md.
- [x] PR #8 main 병합 및 Windows 재검증 상태 정리.
- [x] 사용법, 자동 검증 범위 및 남은 실환경 검증 기록.
- [x] 전체 변경 리뷰 후 필요한 수정과 검증, 작업 브랜치에 커밋.

## 실행 기록
- main `755194a` 동기화 후 기존 작업 폴더의 새 기능 브랜치 사용. 기존 요청의 실행 승인을 유지하여 추가 승인 절차 없이 진행.
- 기준 테스트 22/22 통과. UI 출력 크기 복원 RED 확인 후 저장 연결 구현.
- 모듈/화면 연결 및 자동 테스트 구현. 저장창 승인 후 배율 재검증은 독립 리뷰 지적을 재현하고 수정.
- 정수 오버플로 설정은 테스트 실패를 확인한 뒤 64비트 범위 검사로 수정.
- Dropbox 파일 읽기 지연 및 공간 부족으로 `/tmp` 검증 복사본과 Qt 6.8.3 SDK 사용; 최종 소스는 원래 브랜치로 동기화.
- 최종 전체 24/24 통과. 실제 장치 변경 및 Windows/Linux 실행은 #3에 유지.
- 리뷰의 오디오 실장치 재열거 확인은 실환경 검증으로 남긴다. ID 기반 매칭은 임시 저장소에서 검증했지만 OS 장치 열거를 합성하지 않았다.
