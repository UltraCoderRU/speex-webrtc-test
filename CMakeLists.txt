cmake_minimum_required(VERSION 3.14)
project(speex_webrtc_test)

set(CMAKE_CXX_STANDARD 14)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

find_package(Threads REQUIRED)
find_package(Qt5 COMPONENTS Core Widgets Multimedia REQUIRED)
find_package(WebRTC REQUIRED)

add_subdirectory(external)
add_subdirectory(src)
