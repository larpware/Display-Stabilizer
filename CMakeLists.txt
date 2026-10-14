cmake_minimum_required(VERSION 3.20)
project(DisplayStabilizer VERSION 1.0.0 LANGUAGES CXX RC)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4 /permissive- /utf-8)
endif()

add_compile_definitions(UNICODE _UNICODE)

# Core sources
set(CORE_SOURCES
    src/core/JsonHelper.h
    src/core/JsonHelper.cpp
    src/core/Profile.h
    src/core/Profile.cpp
    src/core/ProfileManager.h
    src/core/ProfileManager.cpp
    src/core/ColorEngine.h
    src/core/ColorEngine.cpp
    src/core/DisplayManager.h
    src/core/DisplayManager.cpp
    src/core/Settings.h
    src/core/Settings.cpp
)

# Diagnostics sources
set(DIAGNOSTICS_SOURCES
    src/diagnostics/TestPatterns.h
    src/diagnostics/TestPatterns.cpp
    src/diagnostics/VgaWorkflow.h
    src/diagnostics/VgaWorkflow.cpp
    src/diagnostics/HardwareAdvisory.h
    src/diagnostics/HardwareAdvisory.cpp
)

# UI sources
set(UI_SOURCES
    src/ui/Controls.h
    src/ui/PatternWindow.h
    src/ui/PatternWindow.cpp
    src/ui/RollbackDialog.h
    src/ui/RollbackDialog.cpp
    src/ui/TrayIcon.h
    src/ui/TrayIcon.cpp
    src/ui/MainWindow.h
    src/ui/MainWindow.cpp
)

# Main Application Executable (DisplayStabilizer.exe)
add_executable(DisplayStabilizer WIN32
    src/main.cpp
    ${CORE_SOURCES}
    ${DIAGNOSTICS_SOURCES}
    ${UI_SOURCES}
    resources/app.manifest
)

target_include_directories(DisplayStabilizer PRIVATE src)
target_link_libraries(DisplayStabilizer PRIVATE
    user32
    gdi32
    shell32
    comctl32
    shlwapi
    ole32
    advapi32
)

set_target_properties(DisplayStabilizer PROPERTIES
    OUTPUT_NAME "DisplayStabilizer"
)

# Automated Test Executable (DisplayStabilizerTests.exe)
add_executable(DisplayStabilizerTests
    tests/TestMain.cpp
    ${CORE_SOURCES}
    ${DIAGNOSTICS_SOURCES}
)

target_include_directories(DisplayStabilizerTests PRIVATE src)
target_link_libraries(DisplayStabilizerTests PRIVATE
    user32
    gdi32
    shell32
    comctl32
    shlwapi
    advapi32
)

enable_testing()
add_test(NAME DisplayStabilizerTestSuite COMMAND DisplayStabilizerTests)
