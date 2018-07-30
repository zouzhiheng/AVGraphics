cmake_minimum_required(VERSION 3.4.1)

# --- set variable
set(dir_sles src/main/cpp/sles)
set(dir_gles src/main/cpp/gles)


# --- add library
add_library( sles

             SHARED

             ${dir_sles}/sles.cpp
             ${dir_sles}/AssetAudioPlayer.cpp
             ${dir_sles}/BQAudioPlayer.cpp
             ${dir_sles}/AudioRecorder.cpp )

add_library( gles

             SHARED

             ${dir_gles}/glutil.cpp
             ${dir_gles}/EGLDemo.cpp
             ${dir_gles}/EGLCore.cpp
             ${dir_gles}/Square.cpp
             ${dir_gles}/Circle.cpp
             ${dir_gles}/Triangle.cpp )


include_directories( src/main/cpp/util )

# --- link libraries
target_link_libraries( sles

                       android
                       log
                       OpenSLES )

target_link_libraries( gles

                       EGL
                       GLESv3
                       android
                       log
                       m )