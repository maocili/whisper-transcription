#include <napi.h>
#include <thread>
#include <atomic>
#include <unistd.h>
#include <sys/stat.h>

#include "operator.h"

// ------ NodeAddon boilerplate --------

Object NodeTranscription::Init(Napi::Env env, Object exports) {
    Function func = DefineClass(env, "NodeTranscription", {
                                    InstanceMethod("start", &NodeTranscription::Start),
                                    InstanceMethod("stop", &NodeTranscription::Stop),
                                    InstanceMethod("loadModel", &NodeTranscription::LoadModel),
                                    InstanceMethod("freeModel", &NodeTranscription::FreeModel),
                                });

    exports.Set("NodeTranscription", func);
    return exports;
}

NodeTranscription::NodeTranscription(const CallbackInfo &info) : ObjectWrap<NodeTranscription>(info) {
    ctrl = std::make_unique<TranscriptionController>();
    running = false;

    memset(&details, 0, sizeof(details));
}

NodeTranscription::~NodeTranscription() {
}

void NodeTranscription::LoadModel(const CallbackInfo &info) {
    Napi::Env env = info.Env();

    if (info.Length() < 1 || !info[0].IsObject()) {
        Napi::Error::New(env, "Expected object as first argument").ThrowAsJavaScriptException();
        return;
    }

    Napi::Object settings = info[0].As<Napi::Object>();

    std::shared_ptr<model_parpams_t> p = std::make_shared<model_parpams_t>(model_parpams_t{
        .n_threads = settings.Has("nThreads")
                         ? settings.Get("nThreads").ToNumber().Int32Value()
                         : std::min(4, (int32_t) std::thread::hardware_concurrency()),
        .step_ms = settings.Has("stepMs") ? settings.Get("stepMs").ToNumber().Int32Value() : 3000,
        .length_ms = settings.Has("lengthMs") ? settings.Get("lengthMs").ToNumber().Int32Value() : 10000,
        .keep_ms = settings.Has("keepMs") ? settings.Get("keepMs").ToNumber().Int32Value() : 200,
        .capture_id = settings.Has("captureId") ? settings.Get("captureId").ToNumber().Int32Value() : 0,
        .max_tokens = settings.Has("maxTokens") ? settings.Get("maxTokens").ToNumber().Int32Value() : 32,
        .audio_ctx = settings.Has("audioCtx") ? settings.Get("audioCtx").ToNumber().Int32Value() : 0,
        .beam_size = settings.Has("beamSize") ? settings.Get("beamSize").ToNumber().Int32Value() : -1,
        .vad_thold = settings.Has("vadThreshold") ? settings.Get("vadThreshold").ToNumber().FloatValue() : 0.6f,
        .freq_thold = settings.Has("freqThreshold") ? settings.Get("freqThreshold").ToNumber().FloatValue() : 100.0f,
        .translate = settings.Has("translate") ? settings.Get("translate").ToBoolean() : false,
        .no_fallback = settings.Has("noFallback") ? settings.Get("noFallback").ToBoolean() : false,
        .print_special = settings.Has("printSpecial") ? settings.Get("printSpecial").ToBoolean() : false,
        .no_context = settings.Has("noContext") ? settings.Get("noContext").ToBoolean() : true,
        .no_timestamps = settings.Has("noTimestamps") ? settings.Get("noTimestamps").ToBoolean() : false,
        .tinydiarize = settings.Has("tinyDiarize") ? settings.Get("tinyDiarize").ToBoolean() : false,
        .save_audio = settings.Has("saveAudio") ? settings.Get("saveAudio").ToBoolean() : false,
        .use_gpu = settings.Has("useGpu") ? settings.Get("useGpu").ToBoolean() : true,
        .flash_attn = settings.Has("flashAttn") ? settings.Get("flashAttn").ToBoolean() : false,
        .language = settings.Has("language") ? settings.Get("language").ToString().Utf8Value() : "en",
        .model = settings.Has("modelPath")
                     ? settings.Get("modelPath").ToString().Utf8Value()
                     : "models/ggml-base.en.bin",
        .fname_out = "",
    });

    std::shared_ptr<modelDetails_t> m = std::make_shared<modelDetails_t>(modelDetails_t{
        .baseDir = "/Users/xuxifeng/Work/my-electron-app/models/",
        .modelName = "ggml-tiny.en.bin",
        .params = p.get(),
    });

    auto ret = ctrl->loadModel(m.get());
    if (ret != 0) {
        printf("Load model failed err_code:%d \n", ret);
    }
}

void NodeTranscription::FreeModel(const CallbackInfo &info) {
    ctrl->freeModel();
}

// event emitter push message
void NodeTranscription::PushResultToJs(const CallbackInfo &info, std::string *key, std::string *data) {
    Napi::Env env = info.Env();
    Napi::Function emit = info[0].As<Napi::Function>();
    emit.Call({Napi::String::New(env, *key), Napi::String::New(env, *data)});
}

// ----- Start/Stop
void NodeTranscription::Start(const CallbackInfo &info) {
    if (running) {
        return;
    }
    running = true;


    Napi::Env env = info.Env();

    Napi::Object settings = info[1].As<Napi::Object>();

    std::shared_ptr<model_parpams_t> p = std::make_shared<model_parpams_t>(model_parpams_t{
        .n_threads = settings.Has("nThreads")
                         ? settings.Get("nThreads").ToNumber().Int32Value()
                         : std::min(4, (int32_t) std::thread::hardware_concurrency()),
        .step_ms = settings.Has("stepMs") ? settings.Get("stepMs").ToNumber().Int32Value() : 3000,
        .length_ms = settings.Has("lengthMs") ? settings.Get("lengthMs").ToNumber().Int32Value() : 10000,
        .keep_ms = settings.Has("keepMs") ? settings.Get("keepMs").ToNumber().Int32Value() : 200,
        .capture_id = settings.Has("captureId") ? settings.Get("captureId").ToNumber().Int32Value() : 0,
        .max_tokens = settings.Has("maxTokens") ? settings.Get("maxTokens").ToNumber().Int32Value() : 32,
        .audio_ctx = settings.Has("audioCtx") ? settings.Get("audioCtx").ToNumber().Int32Value() : 0,
        .beam_size = settings.Has("beamSize") ? settings.Get("beamSize").ToNumber().Int32Value() : -1,
        .vad_thold = settings.Has("vadThreshold") ? settings.Get("vadThreshold").ToNumber().FloatValue() : 0.6f,
        .freq_thold = settings.Has("freqThreshold")
                          ? settings.Get("freqThreshold").ToNumber().FloatValue()
                          : 100.0f,
        .translate = settings.Has("translate") ? settings.Get("translate").ToBoolean() : false,
        .no_fallback = settings.Has("noFallback") ? settings.Get("noFallback").ToBoolean() : false,
        .print_special = settings.Has("printSpecial") ? settings.Get("printSpecial").ToBoolean() : false,
        .no_context = settings.Has("noContext") ? settings.Get("noContext").ToBoolean() : true,
        .no_timestamps = settings.Has("noTimestamps") ? settings.Get("noTimestamps").ToBoolean() : false,
        .tinydiarize = settings.Has("tinyDiarize") ? settings.Get("tinyDiarize").ToBoolean() : false,
        .save_audio = settings.Has("saveAudio") ? settings.Get("saveAudio").ToBoolean() : false,
        .use_gpu = settings.Has("useGpu") ? settings.Get("useGpu").ToBoolean() : true,
        .flash_attn = settings.Has("flashAttn") ? settings.Get("flashAttn").ToBoolean() : false,
        .language = settings.Has("language") ? settings.Get("language").ToString().Utf8Value() : "en",
        .model = settings.Has("modelPath")
                     ? settings.Get("modelPath").ToString().Utf8Value()
                     : "models/ggml-base.en.bin",
        .fname_out = "",
    });

    std::shared_ptr<modelDetails_t> m = std::make_shared<modelDetails_t>(modelDetails_t{
        .baseDir = "/Users/xuxifeng/Work/my-electron-app/models/",
        .modelName = "ggml-tiny.en.bin",
        .params = p.get(),
    });

    // 你可以从 info[0] 提取参数 (demo里省略)
    // details.xxx = ...
    whisper_full_params wparams = whisper_full_default_params(
        p->beam_size > 1 ? WHISPER_SAMPLING_BEAM_SEARCH : WHISPER_SAMPLING_GREEDY);
    details.params = p.get();
    details.wparams = &wparams;

    // 让 run 方法能回调到本对象
    details.userData = this; // 你可以在 details_t 里加 void* userData
    auto ret = ctrl->loadModel(m.get());
    printf("ret: %d\n", ret);

    ctrl->run(info, &details);
}

// 用户 stop
void NodeTranscription::Stop(const CallbackInfo &info) {
    printf("Call Stop ");
    if (!running)
        return;
    running = false;
    ctrl->stop();
}

// ------- 导出 initializer
Object InitOperatorWrapper(Napi::Env env, Object exports) {
    return NodeTranscription::Init(env, exports);
}

// 主模块初始化函数
Napi::Object Init(Napi::Env env, Napi::Object exports) {
    // Operator  模块
    exports = InitOperatorWrapper(env, exports);
    return exports;
}

NODE_API_MODULE(transcribe_addon, Init)
