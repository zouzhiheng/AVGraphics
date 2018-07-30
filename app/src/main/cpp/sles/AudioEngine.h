//
// Created by zzh on 2018/7/27 0027.
//

#ifndef AVGRAPHICS_AUDIOENGINE_H
#define AVGRAPHICS_AUDIOENGINE_H

#include <SLES/OpenSLES.h>
#include <assert.h>

class AudioEngine {
public:
    SLObjectItf engineObj;
    SLEngineItf engine;

    SLObjectItf outputMixObj;

private:
    void createEngine() {
        SLresult result;

        result = slCreateEngine(&engineObj, 0, nullptr, 0, nullptr, nullptr);
        assert(result == SL_RESULT_SUCCESS);
        (void) result;

        result = (*engineObj)->Realize(engineObj, SL_BOOLEAN_FALSE);
        assert(SL_RESULT_SUCCESS == result);
        (void) result;

        result = (*engineObj)->GetInterface(engineObj, SL_IID_ENGINE, &engine);
        assert(SL_RESULT_SUCCESS == result);
        (void) result;

        const SLInterfaceID ids[1] = {SL_IID_ENVIRONMENTALREVERB};
        const SLboolean req[1] = {SL_BOOLEAN_FALSE};
        // outputMixObj 用于输出声音数据
        (*engine)->CreateOutputMix(engine, &outputMixObj, 1, ids, req);

        result = (*outputMixObj)->Realize(outputMixObj, SL_BOOLEAN_FALSE);
        assert(SL_RESULT_SUCCESS == result);
        (void) result;
    }

    virtual void release() {
        if (outputMixObj) {
            (*outputMixObj)->Destroy(outputMixObj);
            outputMixObj = nullptr;
        }

        if (engineObj) {
            (*engineObj)->Destroy(engineObj);
            engineObj = nullptr;
            engine = nullptr;
        }
    }

public:
    AudioEngine() : engineObj(nullptr), engine(nullptr), outputMixObj(nullptr) {
        createEngine();
    }

    virtual ~AudioEngine() {
        release();
    }
};

#endif //AVGRAPHICS_AUDIOENGINE_H
