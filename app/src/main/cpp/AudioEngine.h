//
// Created by Administrator on 2018/7/27 0027.
//

#ifndef AVGRAPHICS_AUDIOENGINE_H
#define AVGRAPHICS_AUDIOENGINE_H

#include <SLES/OpenSLES.h>
#include <assert.h>

class AudioEngine {
protected:
    SLObjectItf mEngineObj;
    SLEngineItf mEngine;

    SLObjectItf mOutputMixObj;

protected:
    void createEngine() {
        SLresult result;

        result = slCreateEngine(&mEngineObj, 0, nullptr, 0, nullptr, nullptr);
        assert(result == SL_RESULT_SUCCESS);
        (void) result;

        result = (*mEngineObj)->Realize(mEngineObj, SL_BOOLEAN_FALSE);
        assert(SL_RESULT_SUCCESS == result);
        (void) result;

        result = (*mEngineObj)->GetInterface(mEngineObj, SL_IID_ENGINE, &mEngine);
        assert(SL_RESULT_SUCCESS == result);
        (void) result;

        const SLInterfaceID ids[1] = {SL_IID_ENVIRONMENTALREVERB};
        const SLboolean req[1] = {SL_BOOLEAN_FALSE};
        (*mEngine)->CreateOutputMix(mEngine, &mOutputMixObj, 1, ids, req);

        result = (*mOutputMixObj)->Realize(mOutputMixObj, SL_BOOLEAN_FALSE);
        assert(SL_RESULT_SUCCESS == result);
        (void) result;
    }

    virtual void release() {
        if (mOutputMixObj) {
            (*mOutputMixObj)->Destroy(mOutputMixObj);
            mOutputMixObj = nullptr;
        }

        if (mEngineObj) {
            (*mEngineObj)->Destroy(mEngineObj);
            mEngineObj = nullptr;
            mEngine = nullptr;
        }
    }

public:
    AudioEngine() : mEngineObj(nullptr), mEngine(nullptr), mOutputMixObj(nullptr) {
        createEngine();
    }

    virtual ~AudioEngine() {
        release();
    }
};

#endif //AVGRAPHICS_AUDIOENGINE_H
