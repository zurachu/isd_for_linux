cmake_minimum_required(VERSION 3.0)
project(isd_linux)
find_package(PkgConfig REQUIRED)

#set(THREADS_PREFER_PTHREAD_FLAG ON)
#find_package(Threads REQUIRED)

find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
endif(CCACHE_FOUND)

pkg_check_modules(LIBUSB_1_0 REQUIRED libusb-1.0)


add_compile_options(
    -fPIC 
    -g
    -Wall
)
include_directories(${LIBUSB_1_0_INCLUDE_DIRS})
link_directories(${LIBUSB_1_0_LIBRARY_DIRS})

add_executable(isd
    isd.cpp
    piecedev.cpp 
    debug.cpp
    piecefat.cpp
)
target_link_libraries(isd
    ${LIBUSB_1_0_LIBRARIES}
)

add_executable(mem
    mem.cpp
    piecedev.cpp
    debug.cpp
)
target_link_libraries(mem
    ${LIBUSB_1_0_LIBRARIES}
)
