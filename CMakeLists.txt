cmake_minimum_required(VERSION 3.21)

project(COMTerminal VERSION 0.1.0 LANGUAGES CXX RC)

if(NOT WIN32)
    message(FATAL_ERROR "COMTerminal supports only Windows")
endif()

add_executable(COMTerminal WIN32
    src/main.cpp
    src/ui/MainWindow.cpp
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

target_compile_features(COMTerminal PRIVATE cxx_std_17)

target_compile_definitions(COMTerminal PRIVATE
    UNICODE
    _UNICODE
    _WIN32_WINNT=0x0A00
    WINVER=0x0A00
    NOMINMAX
)

if(MSVC)
    target_compile_options(COMTerminal PRIVATE /W4 /WX /permissive-)
endif()

target_link_libraries(COMTerminal PRIVATE
    setupapi
    comctl32
    dwmapi
)

set_target_properties(COMTerminal PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)
