cmake_minimum_required( VERSION 3.22 )
project( camcap )

set( CMAKE_CXX_STANDARD 20 )

add_executable( camcap main.cpp )

set( OpenCV_DIR /opt/local/libexec/opencv4/cmake )

find_package( OpenCV REQUIRED )
include_directories( ${OpenCV_INCLUDE_DIRS} )

target_link_libraries( camcam ${OpenCV_LIBS} )



