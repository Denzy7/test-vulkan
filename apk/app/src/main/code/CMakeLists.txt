cmake_minimum_required(VERSION 3.10)

project(testvk VERSION 0.1)

#set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
#export on create to prevent trimming by linker
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")

# now build app's shared lib
set(testvkpath "../../../../..")
add_library(GFX SHARED
    ${testvkpath}/triangle.c
    spv_andk.c
	main.c
    window_andk.c
	${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c
)

target_include_directories(GFX PRIVATE 
    ${testvkpath}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${ANDROID_NDK}/sources
    )
target_link_libraries(GFX PRIVATE android log vulkan)

