//
// Created by Administrator on 2018/7/27 0027.
//

#ifndef AVGRAPHICS_ASSET_AUDIO_PLAYER_H
#define AVGRAPHICS_ASSET_AUDIO_PLAYER_H

#include <SLES/OpenSLES.h>
#include <stdint.h>
#include <android/asset_manager.h>
#include "AudioEngine.h"

class AssetAudioPlayer : public AudioEngine {
private:
    SLObjectItf mPlayerObj;
    SLPlayItf mPlayer;
    SLSeekItf mSeek;
    SLMuteSoloItf mMuteSolo;
    SLVolumeItf mVolume;

private:
    void initPlayer(AAsset *asset);

    void setPlayerState(SLuint32 state);

    void release() override;

public:
    AssetAudioPlayer(AAsset *asset);

    void start();

    void pause();

    void stop();

    virtual ~AssetAudioPlayer();
};

#endif //AVGRAPHICS_ASSET_AUDIO_PLAYER_H
