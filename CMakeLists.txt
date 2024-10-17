cmake_minimum_required(VERSION 3.20)

project(coroserver)
set(CMAKE_CXX_STANDARD 20)
find_package(asio REQUIRED)

set(SOURCES
        src/main.cpp)

if (WIN32)
    add_executable(coroserver
            ${SOURCES}
            )
    target_compile_definitions(coroserver PUBLIC
            "_WIN32_WINNT=0x0A00"
            "_DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR")
    target_link_libraries(coroserver PUBLIC
            asio::asio)
endif ()


