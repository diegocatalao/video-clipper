cmake_minimum_required(VERSION 3.18)
project(squark LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(GNUInstallDirs)
find_package(PkgConfig REQUIRED)
find_package(OpenCV REQUIRED)

# just separete the sources from differents folders
file(GLOB_RECURSE SOURCES "src/*.cc" "src/*.cpp" "src/*.hh" "src/*.hpp")

# compile the sahred library for gstream plugin
pkg_check_modules(GST REQUIRED gstreamer-1.0>=1.18 gstreamer-base-1.0>=1.18)
pkg_check_modules(GST_VIDEO REQUIRED gstreamer-video-1.0>=1.14)

include_directories(${GST_INCLUDE_DIRS} ${GST_VIDEO_INCLUDE_DIRS})
link_directories(${GST_LIBRARY_DIRS} ${GST_VIDEO_LIBRARY_DIRS})

add_library(${PROJECT_NAME} SHARED ${SOURCES})

# compile with gstreamer
target_compile_options(${PROJECT_NAME} PRIVATE ${GST_CFLAGS_OTHER})
target_include_directories(${PROJECT_NAME} PRIVATE ${GST_INCLUDE_DIRS} ${GST_VIDEO_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PRIVATE ${GST_LIBRARIES} ${GST_VIDEO_LIBRARIES})

# compile with opencv
target_include_directories(${PROJECT_NAME} PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PRIVATE ${OpenCV_LIBS})

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "libgst" OUTPUT_NAME "squark")

# set the compile commands to root path of this project, 
# usefull if you need to set the configuration of your ide
set(COMPILE_COMMANDS_PATH "${CMAKE_BINARY_DIR}/compile_commands.json")
set(LINK_PATH "${CMAKE_SOURCE_DIR}/compile_commands.json")

if(NOT EXISTS "${LINK_PATH}")
    execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink "${COMPILE_COMMANDS_PATH}" "${LINK_PATH}")
endif()

