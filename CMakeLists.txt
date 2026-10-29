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
    if(WIN32)
        find_program(CLIPDOCK_POWERSHELL NAMES powershell.exe pwsh.exe REQUIRED)
        add_test(NAME windows_qt_sdk_detection COMMAND "${CLIPDOCK_POWERSHELL}"
            -NoProfile -ExecutionPolicy Bypass -File "${CMAKE_CURRENT_SOURCE_DIR}/scripts/test-windows-qt.ps1")
    endif()
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
add_library(clipdock_capture STATIC qt/src/X11Capture.cpp qt/src/X11Capture.h)
target_include_directories(clipdock_capture PUBLIC qt/src)
target_link_libraries(clipdock_capture PUBLIC Qt6::Gui)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    find_library(CLIPDOCK_XCB_LIBRARY NAMES xcb REQUIRED)
    target_link_libraries(clipdock_capture PRIVATE "${CLIPDOCK_XCB_LIBRARY}")
endif()
add_library(clipdock_shortcuts STATIC qt/src/GlobalShortcuts.cpp qt/src/GlobalShortcuts.h
    qt/src/ShortcutBackend.h qt/src/NativeGlobalShortcuts.cpp)
target_include_directories(clipdock_shortcuts PUBLIC qt/src)
target_link_libraries(clipdock_shortcuts PUBLIC Qt6::Widgets)
if(MSVC)
    target_compile_options(clipdock_shortcuts PRIVATE /utf-8)
    target_link_libraries(clipdock_shortcuts PRIVATE user32)
elseif(APPLE)
    target_link_libraries(clipdock_shortcuts PRIVATE "-framework Carbon")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    find_package(X11 REQUIRED)
    find_package(Qt6 REQUIRED COMPONENTS DBus)
    target_sources(clipdock_shortcuts PRIVATE qt/src/PortalGlobalShortcuts.cpp qt/src/PortalGlobalShortcuts.h)
    target_compile_definitions(clipdock_shortcuts PRIVATE CLIPDOCK_HAS_X11 CLIPDOCK_HAS_PORTAL)
    target_link_libraries(clipdock_shortcuts PRIVATE X11::X11 PUBLIC Qt6::DBus)
endif()
qt_add_executable(ClipDock WIN32 MACOSX_BUNDLE
    qt/src/main.cpp
    qt/src/MainWindow.cpp qt/src/MainWindow.h qt/src/UiTheme.h
    qt/src/RecorderController.cpp qt/src/RecorderController.h
    qt/src/RecordingOptions.h qt/src/MediaFormat.cpp qt/src/MediaFormat.h
    qt/src/PlatformPermissions.cpp qt/src/PlatformPermissions.h
    qt/src/WindowPicker.cpp qt/src/WindowPicker.h
    qt/src/NativeWindowPicker.cpp qt/src/NativeWindowPicker.h
    qt/src/RegionPicker.cpp qt/src/RegionPicker.h qt/src/RegionCapture.h
)
target_link_libraries(ClipDock PRIVATE Qt6::Widgets Qt6::Multimedia Qt6::MultimediaWidgets clipdock_shortcuts clipdock_capture)
qt_add_resources(ClipDock ui_icons PREFIX /clipdock BASE qt/resources
    FILES qt/resources/chevron-down.xpm qt/resources/chevron-up.xpm qt/resources/checkmark.xpm)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    # Qt marks Wayland platform plugins as non-default, so deployment otherwise
    # includes only XCB. The module also supplies shell/decoration integrations.
    find_package(Qt6 6.8 REQUIRED COMPONENTS WaylandClient)
    target_link_libraries(ClipDock PRIVATE Qt6::WaylandClient)
    qt_import_plugins(ClipDock INCLUDE
        Qt6::QWaylandIntegrationPlugin Qt6::QWaylandEglPlatformIntegrationPlugin)
endif()
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
    qt_add_executable(global_shortcuts_tests qt/tests/GlobalShortcutsTests.cpp)
    target_link_libraries(global_shortcuts_tests PRIVATE clipdock_shortcuts)
    if(MSVC)
        target_compile_options(global_shortcuts_tests PRIVATE /utf-8)
    endif()
    add_test(NAME global_shortcuts COMMAND global_shortcuts_tests)
    set_tests_properties(global_shortcuts PROPERTIES TIMEOUT 15 ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    if(APPLE)
        add_executable(mac_shortcut_keys_tests qt/tests/MacShortcutKeysTests.cpp)
        target_include_directories(mac_shortcut_keys_tests PRIVATE qt/src)
        target_link_libraries(mac_shortcut_keys_tests PRIVATE "-framework Carbon")
        add_test(NAME mac_shortcut_keys COMMAND mac_shortcut_keys_tests)
        set_tests_properties(mac_shortcut_keys PROPERTIES TIMEOUT 15)
    endif()
    if(WIN32)
        qt_add_executable(native_global_shortcuts_tests qt/tests/NativeGlobalShortcutsTests.cpp)
        target_link_libraries(native_global_shortcuts_tests PRIVATE clipdock_shortcuts user32)
        target_compile_options(native_global_shortcuts_tests PRIVATE /utf-8)
        add_test(NAME native_global_shortcuts COMMAND native_global_shortcuts_tests)
        set_tests_properties(native_global_shortcuts PROPERTIES TIMEOUT 15)
    endif()
    qt_add_executable(region_picker_tests qt/tests/RegionPickerTests.cpp qt/src/RegionPicker.cpp qt/src/RegionPicker.h)
    target_include_directories(region_picker_tests PRIVATE qt/src)
    target_link_libraries(region_picker_tests PRIVATE Qt6::Widgets)
    if(MSVC)
        target_compile_options(region_picker_tests PRIVATE /utf-8)
    endif()
    add_test(NAME region_picker COMMAND region_picker_tests)
    set_tests_properties(region_picker PROPERTIES TIMEOUT 15 ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    add_test(NAME region_picker_multiple_screens COMMAND region_picker_tests --multi-screen
        -platform "offscreen:configfile=qt/tests/fixtures/multiple-screens.json")
    set_tests_properties(region_picker_multiple_screens PROPERTIES TIMEOUT 15
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        ENVIRONMENT "QT_SCALE_FACTOR_ROUNDING_POLICY=PassThrough")

    qt_add_executable(region_recording_tests qt/tests/RegionRecordingTests.cpp
        qt/src/RecorderController.cpp qt/src/RecorderController.h qt/src/MediaFormat.cpp qt/src/PlatformPermissions.cpp)
    target_include_directories(region_recording_tests PRIVATE qt/src)
    target_link_libraries(region_recording_tests PRIVATE Qt6::Gui Qt6::Multimedia clipdock_capture)
    if(APPLE)
        target_link_libraries(region_recording_tests PRIVATE "-framework CoreGraphics")
    elseif(MSVC)
        target_compile_options(region_recording_tests PRIVATE /utf-8)
    endif()
    add_test(NAME region_recording COMMAND region_recording_tests)
    add_test(NAME recording_capture_failure COMMAND region_recording_tests --capture-failure)
    add_test(NAME recording_delayed_permission COMMAND region_recording_tests --delayed-permission)
    set_tests_properties(region_recording recording_capture_failure recording_delayed_permission
        PROPERTIES TIMEOUT 30 ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_MEDIA_BACKEND=ffmpeg")

    qt_add_executable(recorder_lifecycle_tests qt/tests/RecorderLifecycleTests.cpp
        qt/src/RecorderController.cpp qt/src/RecorderController.h qt/src/MediaFormat.cpp qt/src/PlatformPermissions.cpp)
    target_include_directories(recorder_lifecycle_tests PRIVATE qt/src)
    target_link_libraries(recorder_lifecycle_tests PRIVATE Qt6::Gui Qt6::Multimedia clipdock_capture)
    if(APPLE)
        target_link_libraries(recorder_lifecycle_tests PRIVATE "-framework CoreGraphics")
    elseif(MSVC)
        target_compile_options(recorder_lifecycle_tests PRIVATE /utf-8)
    endif()
    add_test(NAME recorder_lifecycle COMMAND recorder_lifecycle_tests)
    set_tests_properties(recorder_lifecycle PROPERTIES TIMEOUT 15 ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

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

    qt_add_executable(main_window_tests qt/tests/MainWindowTests.cpp
        qt/src/MainWindow.cpp qt/src/MainWindow.h qt/src/UiTheme.h qt/src/RecorderController.cpp qt/src/RecorderController.h
        qt/src/MediaFormat.cpp qt/src/PlatformPermissions.cpp qt/src/RegionPicker.cpp qt/src/RegionPicker.h
        qt/src/WindowPicker.cpp qt/src/WindowPicker.h qt/src/NativeWindowPicker.cpp)
    target_include_directories(main_window_tests PRIVATE qt/src)
    target_link_libraries(main_window_tests PRIVATE Qt6::Widgets Qt6::Multimedia Qt6::MultimediaWidgets clipdock_shortcuts clipdock_capture)
    qt_add_resources(main_window_tests ui_icons PREFIX /clipdock BASE qt/resources
        FILES qt/resources/chevron-down.xpm qt/resources/chevron-up.xpm qt/resources/checkmark.xpm)
    if(APPLE OR WIN32)
        target_link_libraries(main_window_tests PRIVATE Qt6::MultimediaPrivate)
    endif()
    if(APPLE)
        target_link_libraries(main_window_tests PRIVATE "-framework CoreGraphics")
    elseif(WIN32)
        target_link_libraries(main_window_tests PRIVATE user32 dwmapi)
    endif()
    if(MSVC)
        target_compile_options(main_window_tests PRIVATE /utf-8)
    endif()
    add_test(NAME main_window COMMAND main_window_tests)
    set_tests_properties(main_window PROPERTIES TIMEOUT 45 ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    add_test(NAME main_window_startup_policy COMMAND main_window_tests --startup-policy)
    add_test(NAME main_window_move_into_capture COMMAND main_window_tests --move-into-capture)
    set_tests_properties(main_window_startup_policy main_window_move_into_capture
        PROPERTIES TIMEOUT 30 ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    if(WIN32)
        add_test(NAME main_window_capture_affinity COMMAND main_window_tests --capture-affinity)
        set_tests_properties(main_window_capture_affinity PROPERTIES TIMEOUT 30 ENVIRONMENT "QT_QPA_PLATFORM=windows")
    endif()
    add_test(NAME main_window_multiple_screens COMMAND main_window_tests --multi-screen
        -platform "offscreen:configfile=qt/tests/fixtures/multiple-screens.json")
    set_tests_properties(main_window_multiple_screens PROPERTIES TIMEOUT 15
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        ENVIRONMENT "QT_SCALE_FACTOR_ROUNDING_POLICY=PassThrough")

    qt_add_executable(media_smoke qt/tests/MediaSmoke.cpp qt/src/MediaFormat.cpp)
    target_include_directories(media_smoke PRIVATE qt/src)
    target_link_libraries(media_smoke PRIVATE Qt6::Gui Qt6::Multimedia)
    if(MSVC)
        target_compile_options(media_smoke PRIVATE /utf-8)
    endif()
    add_test(NAME media_encode_decode COMMAND media_smoke)
    set_tests_properties(media_encode_decode PROPERTIES TIMEOUT 45
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_MEDIA_BACKEND=ffmpeg")

    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        find_program(CLIPDOCK_DBUS_RUN_SESSION dbus-run-session REQUIRED)
        qt_add_executable(portal_global_shortcuts_tests qt/tests/PortalGlobalShortcutsTests.cpp)
        target_link_libraries(portal_global_shortcuts_tests PRIVATE clipdock_shortcuts)
        add_test(NAME portal_global_shortcuts COMMAND "${CLIPDOCK_DBUS_RUN_SESSION}" -- "$<TARGET_FILE:portal_global_shortcuts_tests>")
        set_tests_properties(portal_global_shortcuts PROPERTIES TIMEOUT 20 ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
        find_program(CLIPDOCK_XVFB_RUN xvfb-run)
        if(CLIPDOCK_XVFB_RUN)
            add_executable(x11_capture_tests qt/tests/X11CaptureTests.cpp)
            target_link_libraries(x11_capture_tests PRIVATE clipdock_capture X11::X11)
            add_test(NAME x11_capture_native_bounds COMMAND "${CLIPDOCK_XVFB_RUN}" -a
                -s "-screen 0 1280x800x24" "$<TARGET_FILE:x11_capture_tests>")
            set_tests_properties(x11_capture_native_bounds PROPERTIES TIMEOUT 20
                ENVIRONMENT "QT_QPA_PLATFORM=xcb;QT_SCALE_FACTOR=1.25;QT_SCALE_FACTOR_ROUNDING_POLICY=PassThrough")
            find_program(CLIPDOCK_OPENBOX openbox)
            find_program(CLIPDOCK_XDOTOOL xdotool)
            find_program(CLIPDOCK_XMESSAGE xmessage)
            find_program(CLIPDOCK_XPROP xprop)
            if(CLIPDOCK_OPENBOX AND CLIPDOCK_XDOTOOL AND CLIPDOCK_XMESSAGE AND CLIPDOCK_XPROP)
                qt_add_executable(x11_capture_exclusion_tests qt/tests/X11CaptureExclusionTests.cpp
                    qt/src/MainWindow.cpp qt/src/MainWindow.h qt/src/UiTheme.h
                    qt/src/RecorderController.cpp qt/src/RecorderController.h
                    qt/src/MediaFormat.cpp qt/src/PlatformPermissions.cpp
                    qt/src/RegionPicker.cpp qt/src/RegionPicker.h
                    qt/src/WindowPicker.cpp qt/src/WindowPicker.h qt/src/NativeWindowPicker.cpp)
                target_link_libraries(x11_capture_exclusion_tests PRIVATE
                    Qt6::Widgets Qt6::Multimedia Qt6::MultimediaWidgets clipdock_shortcuts clipdock_capture X11::X11)
                qt_add_resources(x11_capture_exclusion_tests ui_icons PREFIX /clipdock BASE qt/resources
                    FILES qt/resources/chevron-down.xpm qt/resources/chevron-up.xpm qt/resources/checkmark.xpm)
                foreach(mode preview direct region opt-out)
                    add_test(NAME x11_capture_exclusion_${mode}
                        COMMAND "${CLIPDOCK_DBUS_RUN_SESSION}" -- "${CLIPDOCK_XVFB_RUN}" -a
                        -s "-screen 0 1280x800x24" bash "${CMAKE_CURRENT_SOURCE_DIR}/scripts/test-x11-capture-exclusion.sh"
                        "$<TARGET_FILE:x11_capture_exclusion_tests>" "--${mode}")
                    set_tests_properties(x11_capture_exclusion_${mode} PROPERTIES TIMEOUT 90
                        ENVIRONMENT "QT_QPA_PLATFORM=xcb;QT_MEDIA_BACKEND=ffmpeg;WAYLAND_DISPLAY=;XDG_SESSION_TYPE=x11")
                endforeach()
            endif()
            if(X11_XTest_FOUND)
                qt_add_executable(x11_global_shortcuts_tests qt/tests/X11GlobalShortcutsTests.cpp)
                target_link_libraries(x11_global_shortcuts_tests PRIVATE clipdock_shortcuts X11::X11 X11::Xtst)
                add_test(NAME x11_global_shortcuts COMMAND "${CLIPDOCK_XVFB_RUN}" -a "$<TARGET_FILE:x11_global_shortcuts_tests>")
                set_tests_properties(x11_global_shortcuts PROPERTIES TIMEOUT 15
                    ENVIRONMENT "QT_QPA_PLATFORM=xcb;WAYLAND_DISPLAY=;XDG_SESSION_TYPE=x11")
            endif()
            qt_add_executable(linux_screen_capture_tests qt/tests/LinuxScreenCaptureTests.cpp)
            target_link_libraries(linux_screen_capture_tests PRIVATE Qt6::Widgets Qt6::Multimedia)
            add_test(NAME linux_screen_capture COMMAND "${CLIPDOCK_XVFB_RUN}" -a
                -s "-screen 0 1280x720x24" "$<TARGET_FILE:linux_screen_capture_tests>")
            set_tests_properties(linux_screen_capture PROPERTIES TIMEOUT 20
                ENVIRONMENT "QT_QPA_PLATFORM=xcb;QT_MEDIA_BACKEND=ffmpeg")
        endif()
    endif()
endif()

include(GNUInstallDirs)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set_target_properties(ClipDock PROPERTIES INSTALL_RPATH "$ORIGIN/../${CMAKE_INSTALL_LIBDIR}")
    install(FILES qt/platform/linux/io.github.pryaoh.ClipDock.desktop
        DESTINATION "${CMAKE_INSTALL_DATADIR}/applications")
endif()
if(MSVC)
    # windeployqt supplies the redistributable installer, not app-local CRT DLLs.
    # Include the release runtime so the packaged app also starts on a clean PC.
    set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION "${CMAKE_INSTALL_BINDIR}")
    include(InstallRequiredSystemLibraries)
    if(NOT CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS)
        message(FATAL_ERROR "MSVC redistributable libraries were not found. Install the Visual C++ runtime tools.")
    endif()
endif()
install(TARGETS ClipDock BUNDLE DESTINATION . RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
qt_generate_deploy_app_script(TARGET ClipDock OUTPUT_SCRIPT deploy_script NO_UNSUPPORTED_PLATFORM_ERROR)
install(SCRIPT ${deploy_script})
