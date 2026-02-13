cmake_minimum_required(VERSION 3.21)

project(COMTerminal VERSION 1.0.0 LANGUAGES CXX RC)

if(NOT WIN32)
    message(FATAL_ERROR "COMTerminal supports only Windows 10/11")
endif()

# Проверка версии Windows
add_compile_definitions(
    UNICODE
    _UNICODE
    _WIN32_WINNT=0x0A00      # Windows 10
    WINVER=0x0A00            # Windows 10
    NOMINMAX
    _CRT_SECURE_NO_WARNINGS
)

# Включаем визуальные стили
add_compile_definitions(
    # Включаем современные контролы
    _WTL_USE_CSTRING
)

# Используем статическую CRT для изоляции
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

add_executable(COMTerminal WIN32
    src/main.cpp
    src/ui/MainWindow.cpp
    src/ui/WindowBuilder.cpp
    src/ui/WindowLayout.cpp
    src/ui/WindowActions.cpp
    src/core/SafeHandle.cpp
    src/core/BufferPool.cpp
    src/core/Crc.cpp
    src/core/LogVirtualizer.cpp
    src/serial/PortScanner.cpp
    src/serial/SerialPort.cpp
    resources/app.rc
)

target_include_directories(COMTerminal PRIVATE
    include
    src
)

# Устанавливаем стандарт C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
target_compile_features(COMTerminal PRIVATE cxx_std_20)

if(MSVC)
    target_compile_options(COMTerminal PRIVATE 
        /W4 
        # /WX  # Не убиваем сборку варнингами
        /permissive-
        /utf-8
        /Zc:__cplusplus
        /EHsc
    )
endif()

target_link_libraries(COMTerminal PRIVATE
    setupapi      # Для COM-портов
)

# Указываем версию Common Controls
set_target_properties(COMTerminal PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    WIN32_EXECUTABLE TRUE
)

# Добавляем manifest для современных контролов
target_sources(COMTerminal PRIVATE
    resources/app.manifest
)