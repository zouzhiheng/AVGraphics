cmake_minimum_required(VERSION 3.4.1)

# --- set variable
set(libs "${CMAKE_SOURCE_DIR}/libs")
set(dir_cpp src/main/cpp)
set(dir_common src/main/cpp/common)
set(dir_ffcodec src/main/cpp/ffcodec)
set(dir_sles src/main/cpp/sles)
set(dir_gles src/main/cpp/gles)
set(dir_libyuv src/main/cpp/libyuv)
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

add_library ( ffcodec
              SHARED
              ${dir_ffcodec}/ffcodec.cpp
              ${dir_ffcodec}/Decoder.cpp
              ${dir_ffcodec}/Encoder.cpp
              ${dir_ffcodec}/FrameFilter.cpp
              ${dir_ffcodec}/Transcoder.cpp
              ${dir_common}/AVModel.cpp
              ${dir_common}/format.cpp
              ${dir_common}/Yuv.cpp
              ${dir_common}/YuvProcessor.cpp )

add_library( ffmpeg
             SHARED
             IMPORTED )

add_library( gles
             SHARED
             ${dir_gles}/gles.cpp
             ${dir_gles}/glutil.cpp
             ${dir_gles}/EGLDemo.cpp
             ${dir_gles}/EGLCore.cpp
             ${dir_gles}/Triangle.cpp
             ${dir_gles}/Circle.cpp
             ${dir_gles}/Square.cpp
             ${dir_gles}/TextureImage.cpp
             ${dir_gles}/FboRenderer.cpp
             ${dir_gles}/GLCamera.cpp
             ${dir_gles}/Watermark.cpp
             ${dir_gles}/Beauty.cpp
             ${dir_gles}/YuvRenderer.cpp
             ${dir_common}/AVModel.cpp
             ${dir_common}/format.cpp
             ${dir_common}/Yuv.cpp
             ${dir_common}/YuvProcessor.cpp )

add_library( yuv
             STATIC
             ${yuv_src_list} )

add_library( sles
             SHARED
             ${dir_sles}/sles.cpp
             ${dir_sles}/AssetAudioPlayer.cpp
             ${dir_sles}/BQAudioPlayer.cpp
             ${dir_sles}/AudioRecorder.cpp )

set_target_properties( ffmpeg
                       PROPERTIES IMPORTED_LOCATION
                       ${libs}/armeabi-v7a/libffmpeg.so )

include_directories( ${dir_cpp}/common
                     ${dir_libyuv}/include
                     ${dir_cpp}/ffmpeg/include )


# --- link libraries

target_link_libraries( ffcodec

                       ffmpeg
                       yuv
                       log )

target_link_libraries( gles

                       EGL
                       GLESv3
                       ffcodec
                       yuv
                       android
                       log
                       m )

target_link_libraries( sles

                       android
                       log
                       OpenSLES )