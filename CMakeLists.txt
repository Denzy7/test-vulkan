cmake_minimum_required(VERSION 3.10)
project(test_vulkan LANGUAGES C)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
find_package(X11)
find_package(Vulkan REQUIRED)
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
list(APPEND window_src spv_stdio.c)

set(exes 
    triangle 
    test_windowbasic)

foreach(exe ${exes})
    add_executable(${exe} ${exe}.c ${window_src})
    target_link_libraries(${exe} ${window_libs})
    target_include_directories(${exe} PRIVATE ${window_include})
    set_target_properties(
        ${exe} PROPERTIES
        C_STANDARD 90)
    if(NOT MSVC)
        target_compile_options(
            ${exe} PRIVATE
            -Wall -Werror -pedantic)
    endif()
endforeach()

set(glslc_src
    vert.glsl.vert
    frag.glsl.frag)

if(Vulkan_GLSLC_EXECUTABLE)
    foreach(src ${glslc_src})
        set(insrc ${test_vulkan_SOURCE_DIR}/${src})
        get_filename_component(bare ${src} NAME_WE)
        set(outsrc ${CMAKE_CURRENT_BINARY_DIR}/${bare}.spv)    
        add_custom_command(
            OUTPUT ${outsrc}
            COMMAND ${Vulkan_GLSLC_EXECUTABLE} ${insrc} -o ${outsrc}
            DEPENDS ${insrc} 
            VERBATIM
            )
        list(APPEND spirv ${outsrc})
    endforeach()
    add_custom_target(shaders 
        DEPENDS ${spirv})
    add_dependencies(triangle shaders)
else()
    message(WARNING "glslc not found. make sure you compiled the shader sources manually into the run directory")
endif()
