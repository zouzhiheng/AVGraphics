cmake_minimum_required(VERSION 3.4.1)

add_library( sles
             SHARED
             src/main/cpp/sles.cpp
             src/main/cpp/AssetAudioPlayer.cpp
             src/main/cpp/BQAudioPlayer.cpp
             src/main/cpp/AudioRecorder.cpp )

target_link_libraries( sles

                       android
                       log
                       OpenSLES )