#include "model.h"
#include <string>
#include <napi.h>

#include <uv.h>


enum modelStatus {
    ModelStatus_Empty,
    ModelStatus_Loaded,
    ModelStatus_Occupied,
};

// Transcribe status
// init -> running -> stopped -> init
enum streamStatus {
    StreamStatus_RUNNING,
    StreamStatus_STOPPED,
};

typedef struct modelDetails {
    std::string baseDir; // 移除 const，允许拷贝
    std::string modelName; // 移除 const，允许拷贝
    model_parpams_t *params;
} modelDetails_t;


typedef struct streamDetails {
    model_parpams_t *params;
    whisper_full_params *wparams;
    model_controllor *mc;
    std::atomic<streamStatus> *st;

    void *userData;
} streamDetails_t;

typedef struct streamEvent {
    streamStatus *oldSt;
    streamStatus newSt;
} streamEvent_t;


class TranscriptionController {
public:
    int loadModel(modelDetails_t *details);

    void freeModel();

    void run(const Napi::CallbackInfo &info, streamDetails_t *details);

    void stop();

    TranscriptionController() {
        modSt = ModelStatus_Empty;
        streamSt = StreamStatus_STOPPED;
        mc = model_controllor();
    }

    ~TranscriptionController() {
    }

private:
    model_controllor mc;
    modelStatus modSt; // unused
    std::atomic<streamStatus> streamSt;

    void updateModelStatus(modelStatus st) {
        modSt = st;
    }
};


// wrapper
using namespace Napi;

using Context = Reference<Napi::Value>;
using DataType = std::string;

void CallJs(Napi::Env env, Function callback, Context *context, DataType *data);

using TSFN = TypedThreadSafeFunction<Context, DataType, CallJs>;
using FinalizerDataType = void;


class NodeTranscription : public ObjectWrap<NodeTranscription> {
public:
    static Object Init(Napi::Env env, Object exports);

    NodeTranscription(const CallbackInfo &info);

    ~NodeTranscription();

    void LoadModel(const CallbackInfo &info);

    void FreeModel(const CallbackInfo &info);

    void Start(const CallbackInfo &info);

    void Stop(const CallbackInfo &info);

    void PushResultToJs(const CallbackInfo &info, std::string *key, std::string *value);

private:
    std::unique_ptr<TranscriptionController> ctrl;
    streamDetails_t details; // 传到线程

    std::atomic<bool> running;
};

void CallJs(Napi::Env env, Function callback, Context *context, DataType *data) {
    // Is the JavaScript environment still available to call into, eg. the TSFN is
    // not aborted
    printf("Call Js : %s \n", data->c_str());
    if (env != nullptr) {
        // On Node-API 5+, the `callback` parameter is optional; however, this
        // example does ensure a callback is provided.
        if (callback != nullptr) {
            callback.Call(context->Value(), {String::New(env, *data)});
        }
    }
    if (data != nullptr) {
        // We're finished with the data.
        delete data;
    }
}


typedef struct run_stream {
    CallbackInfo &info;
    streamDetails_t *details;
} run_stream_t;
