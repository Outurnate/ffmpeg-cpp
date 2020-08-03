cmake_minimum_required(VERSION 3.5)

project(ffmpegcpp VERSION 1.0.0)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

file(GLOB_RECURSE ffmpegcpp_files "include/ffmpegcpp/*.h" "src/*.cpp")
file(GLOB_RECURSE header_files "include/ffmpegcpp/*.h")

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${ffmpegcpp_files})

add_library(ffmpegcpp STATIC ${ffmpegcpp_files})

find_package(FFMPEG REQUIRED)
find_package(ZLIB REQUIRED)

find_library(X264 NAMES x264)
find_library(POSTPROC NAMES postproc)

set_target_properties(ffmpegcpp PROPERTIES 
    VERSION ${PROJECT_VERSION}
    PUBLIC_HEADER ${header_files}
    CXX_STANDARD 17
)

set_target_properties(ffmpegcpp PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
)

target_include_directories(ffmpegcpp PRIVATE 
    include/ffmpegcpp
)

target_link_libraries(ffmpegcpp
    ${FFMPEG_LIBRARIES}
    ${X264}
    ${POSTPROC}
    ZLIB::ZLIB
    va
    va-drm
    va-x11
    vdpau
)

install(TARGETS ffmpegcpp
    ARCHIVE DESTINATION ${CMAKE_BINARY_DIR}/lib
    LIBRARY DESTINATION ${CMAKE_BINARY_DIR}/lib
    PUBLIC_HEADER DESTINATION ${CMAKE_BINARY_DIR}/lib/include
)

install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/ffmpeg/bin/ DESTINATION ${CMAKE_BINARY_DIR}/bin/Release FILES_MATCHING PATTERN "*.dll")
install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/samples/ DESTINATION ${CMAKE_BINARY_DIR}/bin/Release/samples)

add_subdirectory(examples)
