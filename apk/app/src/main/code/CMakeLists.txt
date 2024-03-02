cmake_minimum_required(VERSION 3.10)

project(testvk VERSION 0.1)
find_package(Vulkan REQUIRED)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
#export on create to prevent trimming by linker
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")


set(testvkpath "${CMAKE_CURRENT_SOURCE_DIR}/../../../../..")

# now build app's shared lib
add_library(GFX SHARED
    ${testvkpath}/triangle.c
    ${testvkpath}/vkapp.c
    spv_andk.c
	main.c
    window_andk.c
	${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c
)
set(glslc_src
    vert.glsl.vert
    frag.glsl.frag)
foreach(src ${glslc_src})
    set(insrc ${testvkpath}/${src})
    get_filename_component(bare ${src} NAME_WE)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(flav "debug")
    else()
        set(flav "release")
    endif()
    set(outsrc ${testvkpath}/apk/app/src/main/assets/${bare}.spv)
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
add_dependencies(GFX shaders)

target_include_directories(GFX PRIVATE 
    ${testvkpath}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${ANDROID_NDK}/sources
    )
target_link_libraries(GFX PRIVATE android log vulkan)

