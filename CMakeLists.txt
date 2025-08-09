cmake_minimum_required(VERSION 3.14)

project(file_downloader LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 COMPONENTS Core REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core REQUIRED)

find_package(Threads REQUIRED)

# Check for the platform to configure CURL accordingly
if(WIN32)
    message(STATUS "Building for Windows")

    set(CURL_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/external/curl/include)
    set(CURL_LIB_DIR ${CMAKE_SOURCE_DIR}/external/curl/lib)

    include_directories(${CURL_INCLUDE_DIR})
    link_directories(${CURL_LIB_DIR})

    add_library(curl STATIC IMPORTED)
    set_target_properties(curl PROPERTIES
        IMPORTED_LOCATION ${CURL_LIB_DIR}/libcurl.dll.a
        INTERFACE_INCLUDE_DIRECTORIES ${CURL_INCLUDE_DIR}
    )

    set(CURL_LIB curl)

elseif(APPLE)
    message(STATUS "Building for macOS")

    find_package(CURL REQUIRED)
    set(CURL_LIB CURL::libcurl)

elseif(UNIX)
    message(STATUS "Building for Linux")

    find_package(CURL REQUIRED)
    set(CURL_LIB CURL::libcurl)

else()
    message(FATAL_ERROR "Unsupported platform")
endif()

file(GLOB_RECURSE HEADER_FILES CONFIGURE_DEPENDS "include/*.h")
file(GLOB_RECURSE CPP_SOURCES CONFIGURE_DEPENDS "src/*.cpp")
set(ALL_SOURCES main.cpp ${CPP_SOURCES} ${HEADER_FILES})

add_executable(file_downloader ${ALL_SOURCES})
target_link_libraries(file_downloader PRIVATE Qt${QT_VERSION_MAJOR}::Core ${CURL_LIB} Threads::Threads)
