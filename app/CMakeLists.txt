cmake_minimum_required(VERSION 3.4.1)

# --- set variable
set(dir_cpp src/main/cpp)
set(dir_sles src/main/cpp/sles)
set(dir_gles src/main/cpp/gles)
set(dir_libyuv src/main/cpp/libyuv)
set(dir_yuv src/main/cpp/yuv)
set(yuv_src "src/main/cpp/libyuv/source")

set( yuv_src_list
     ${yuv_src}/compare.cc
     ${yuv_src}/compare_common.cc
     ${yuv_src}/convert.cc
     ${yuv_src}/convert_argb.cc
     ${yuv_src}/convert_from.cc
     ${yuv_src}/convert_from_argb.cc
     ${yuv_src}/convert_to_argb.cc
     ${yuv_src}/convert_to_i420.cc
     ${yuv_src}/cpu_id.cc
     ${yuv_src}/planar_functions.cc
     ${yuv_src}/rotate.cc
     ${yuv_src}/rotate_any.cc
     ${yuv_src}/rotate_argb.cc
     ${yuv_src}/rotate_common.cc
     ${yuv_src}/row_any.cc
     ${yuv_src}/row_common.cc
     ${yuv_src}/scale.cc
     ${yuv_src}/scale_any.cc
     ${yuv_src}/scale_argb.cc
     ${yuv_src}/scale_common.cc
     ${yuv_src}/video_common.cc )

if(${ANDROID_ABI} STREQUAL "armeabi-v7a")
    set( yuv_src_list
         ${yuv_src_list}
         ${yuv_src}/compare_neon.cc
         ${yuv_src}/rotate_neon.cc
         ${yuv_src}/row_neon.cc
         ${yuv_src}/scale_neon.cc )

    set_property( SOURCE
                  ${yuv_src_list}
                  APPEND_STRING PROPERTY COMPILE_FLAGS " -mfpu=neon")
endif()


# --- add library

add_library( gles
             SHARED
             ${dir_gles}/gles.cpp
             ${dir_gles}/glutil.cpp
             ${dir_gles}/EGLDemo.cpp
             ${dir_gles}/EGLCore.cpp
             ${dir_gles}/Triangle.cpp
             ${dir_gles}/Circle.cpp
             ${dir_gles}/Square.cpp
             ${dir_gles}/Texture.cpp
             ${dir_gles}/FboRenderer.cpp
             ${dir_gles}/GLCamera.cpp
             ${dir_gles}/Beauty.cpp
             ${dir_gles}/YuvRenderer.cpp )

add_library( yuv
             STATIC
             ${yuv_src_list}
             ${dir_yuv}/Yuv.cpp
             ${dir_yuv}/AVModel.cpp
             ${dir_yuv}/util.cpp
             ${dir_yuv}/YuvProcessor.cpp )

add_library( sles
             SHARED
             ${dir_sles}/sles.cpp
             ${dir_sles}/AssetAudioPlayer.cpp
             ${dir_sles}/BQAudioPlayer.cpp
             ${dir_sles}/AudioRecorder.cpp )


include_directories( ${dir_cpp}/util
                     ${dir_libyuv}/include )


# --- link libraries
target_link_libraries( gles

                       EGL
                       GLESv3
                       yuv
                       android
                       log
                       m )

target_link_libraries( yuv

                       log )

target_link_libraries( sles

                       android
                       log
                       OpenSLES )