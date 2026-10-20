cmake_minimum_required(VERSION 3.24)
project(ClipDock VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
include(CTest)

option(CLIPDOCK_CORE_ONLY "Build portable configuration tests without Qt" OFF)
if(BUILD_TESTING)
    add_executable(settings_tests qt/tests/SettingsTests.cpp)
    target_include_directories(settings_tests PRIVATE qt/src)
    if(MSVC)
        target_compile_options(settings_tests PRIVATE /utf-8)
    endif()
    add_test(NAME settings COMMAND settings_tests)
    if(UNIX)
        add_test(NAME qt_sdk_detection COMMAND bash "${CMAKE_CURRENT_SOURCE_DIR}/scripts/test-qt-sdk.sh")
    endif()
    if(APPLE)
        add_test(NAME macos_app_signing COMMAND bash "${CMAKE_CURRENT_SOURCE_DIR}/scripts/test-macos-signing.sh")
        set_tests_properties(macos_app_signing PROPERTIES TIMEOUT 30)
    endif()
endif()
if(CLIPDOCK_CORE_ONLY)
    return()
endif()

find_package(Qt6 6.8 REQUIRED COMPONENTS Widgets Multimedia MultimediaWidgets)
# Qt 6.8's OpenGL wrapper exports the legacy Carbon AGL framework even though
# 64-bit Qt does not use its API. macOS 26 SDKs no longer ship its linker stub.
# Remove only that obsolete dependency; keep Qt's real OpenGL dependency.
# Apply this to the imported target, not to the installed/shared Qt SDK files.
if(APPLE AND TARGET WrapOpenGL::WrapOpenGL)
    get_target_property(qt_opengl_libraries WrapOpenGL::WrapOpenGL INTERFACE_LINK_LIBRARIES)
    if(qt_opengl_libraries)
        list(FILTER qt_opengl_libraries EXCLUDE REGEX "(^-framework AGL$|(^|/)AGL\\.framework(/.*)?$)")
        set_property(TARGET WrapOpenGL::WrapOpenGL PROPERTY INTERFACE_LINK_LIBRARIES "${qt_opengl_libraries}")
    endif()
endif()
qt_standard_project_setup()
qt_add_executable(ClipDock WIN32 MACOSX_BUNDLE
    qt/src/main.cpp
    qt/src/MainWindow.cpp qt/src/MainWindow.h
    qt/src/RecorderController.cpp qt/src/RecorderController.h
    qt/src/RecordingOptions.h qt/src/MediaFormat.cpp qt/src/MediaFormat.h
    qt/src/PlatformPermissions.cpp qt/src/PlatformPermissions.h
    qt/src/WindowPicker.cpp qt/src/WindowPicker.h
    qt/src/NativeWindowPicker.cpp qt/src/NativeWindowPicker.h
    qt/src/RegionPicker.cpp qt/src/RegionPicker.h qt/src/RegionCapture.h
)
target_link_libraries(ClipDock PRIVATE Qt6::Widgets Qt6::Multimedia Qt6::MultimediaWidgets)
# Qt 6.8 exposes no public native window ID. Use matching private headers only
# for the OS window picker bridge, so duplicate titles remain unambiguous.
if(APPLE OR WIN32)
    target_link_libraries(ClipDock PRIVATE Qt6::MultimediaPrivate)
endif()
if(WIN32)
    target_link_libraries(ClipDock PRIVATE user32 dwmapi)
endif()
if(APPLE)
    target_link_libraries(ClipDock PRIVATE "-framework CoreGraphics")
    set_target_properties(ClipDock PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/qt/platform/macos/Info.plist.in"
        MACOSX_BUNDLE_GUI_IDENTIFIER "local.clipdock.qt"
        MACOSX_BUNDLE_BUNDLE_NAME "ClipDock"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}")
    add_custom_command(TARGET ClipDock POST_BUILD
        COMMAND /bin/bash "${CMAKE_CURRENT_SOURCE_DIR}/scripts/sign-macos-app.sh"
            "$<TARGET_BUNDLE_DIR:ClipDock>"
        COMMENT "Signing the macOS app bundle for its privacy permission identity"
        VERBATIM)
endif()
if(MSVC)
    target_compile_options(ClipDock PRIVATE /W4 /utf-8)
else()
    target_compile_options(ClipDock PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(BUILD_TESTING)
    qt_add_executable(region_picker_tests qt/tests/RegionPickerTests.cpp qt/src/RegionPicker.cpp qt/src/RegionPicker.h)
    target_include_directories(region_picker_tests PRIVATE qt/src)
    target_link_libraries(region_picker_tests PRIVATE Qt6::Widgets)
    if(MSVC)
        target_compile_options(region_picker_tests PRIVATE /utf-8)
    endif()
    add_test(NAME region_picker COMMAND region_picker_tests)
    set_tests_properties(region_picker PROPERTIES TIMEOUT 15 ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

    qt_add_executable(region_recording_tests qt/tests/RegionRecordingTests.cpp
        qt/src/RecorderController.cpp qt/src/RecorderController.h qt/src/MediaFormat.cpp qt/src/PlatformPermissions.cpp)
    target_include_directories(region_recording_tests PRIVATE qt/src)
    target_link_libraries(region_recording_tests PRIVATE Qt6::Gui Qt6::Multimedia)
    if(APPLE)
        target_link_libraries(region_recording_tests PRIVATE "-framework CoreGraphics")
    elseif(MSVC)
        target_compile_options(region_recording_tests PRIVATE /utf-8)
    endif()
    add_test(NAME region_recording COMMAND region_recording_tests)
    set_tests_properties(region_recording PROPERTIES TIMEOUT 30 ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_MEDIA_BACKEND=ffmpeg")

    qt_add_executable(window_picker_tests qt/tests/WindowPickerTests.cpp
        qt/src/WindowPicker.cpp qt/src/WindowPicker.h qt/src/NativeWindowPicker.cpp)
    target_include_directories(window_picker_tests PRIVATE qt/src)
    target_link_libraries(window_picker_tests PRIVATE Qt6::Widgets Qt6::Multimedia)
    if(APPLE OR WIN32)
        target_link_libraries(window_picker_tests PRIVATE Qt6::MultimediaPrivate)
    endif()
    if(APPLE)
        target_link_libraries(window_picker_tests PRIVATE "-framework CoreGraphics")
    elseif(WIN32)
        target_link_libraries(window_picker_tests PRIVATE user32 dwmapi)
    endif()
    if(MSVC)
        target_compile_options(window_picker_tests PRIVATE /utf-8)
    endif()
    add_test(NAME window_picker COMMAND window_picker_tests)
    set_tests_properties(window_picker PROPERTIES TIMEOUT 15 ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

    qt_add_executable(media_smoke qt/tests/MediaSmoke.cpp qt/src/MediaFormat.cpp)
    target_include_directories(media_smoke PRIVATE qt/src)
    target_link_libraries(media_smoke PRIVATE Qt6::Gui Qt6::Multimedia)
    if(MSVC)
        target_compile_options(media_smoke PRIVATE /utf-8)
    endif()
    add_test(NAME media_encode_decode COMMAND media_smoke)
    set_tests_properties(media_encode_decode PROPERTIES TIMEOUT 45
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_MEDIA_BACKEND=ffmpeg")
endif()

include(GNUInstallDirs)
install(TARGETS ClipDock BUNDLE DESTINATION . RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
qt_generate_deploy_app_script(TARGET ClipDock OUTPUT_SCRIPT deploy_script NO_UNSUPPORTED_PLATFORM_ERROR)
install(SCRIPT ${deploy_script})
