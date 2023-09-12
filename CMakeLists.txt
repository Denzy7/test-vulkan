cmake_minimum_required(VERSION 3.10)
project(test_vulkan)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
find_package(X11)
find_package(Vulkan)
set(window_src) 
set(window_libs ${Vulkan_LIBRARIES})
set(window_include)
if(X11_FOUND)
    message("using x11")
    list(APPEND window_src window_x11.c)
    list(APPEND window_libs ${X11_LIBRARIES})
    list(APPEND window_include ${X11_INCLUDE_DIR})
elseif(WIN32)
    message("using win32")
    list(APPEND window_src window_win32.c)
endif()

#spv loader
if(ANDROID_NDK)
    #list(APPEND window_src spv_android.c)
else()
    list(APPEND window_src spv_stdio.c)
endif()

set(exes 
    triangle 
    test_windowbasic)

foreach(exe ${exes})
    add_executable(${exe} ${exe}.c ${window_src})
    target_link_libraries(${exe} ${window_libs})
    target_include_directories(${exe} PRIVATE ${window_include})
endforeach()
