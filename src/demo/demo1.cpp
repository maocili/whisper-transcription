#include <chrono>
#include <thread>
#include "operator.h"
#include <unistd.h>



int main() {
    TranscriptionController c = TranscriptionController();

    auto p = model_parpams_t{
        . n_threads = std::min(4, (int32_t) std::thread::hardware_concurrency()),
        . step_ms = 3000,
        . length_ms = 10000,
        . keep_ms = 200,
        . capture_id = 0,
        . max_tokens = 32,
        . audio_ctx = 0,
        . beam_size = -1,
        . vad_thold = 0.6f,
        . freq_thold = 100.0f,
        . translate = false,
        . no_fallback = false,
        . print_special = false,
        . no_context = true,
        . no_timestamps = false,
        . tinydiarize = false,
        . save_audio = false, // save audio to wav fil,
        . use_gpu = true,
        . flash_attn = false,
        . language = "en",
        .model = "models/ggml-base.en.bin",
        .fname_out = "",
    };
    modelDetails_t details = {
        .baseDir = "/Users/xuxifeng/Work/my-electron-app/models/",
        .modelName = "ggml-tiny.en.bin",
        .params = &p,
    };
    int ret = c.loadModel(&details);
    printf("loadModel ret: %d\n", ret);


    whisper_full_params wparams = whisper_full_default_params(
        p.beam_size > 1 ? WHISPER_SAMPLING_BEAM_SEARCH : WHISPER_SAMPLING_GREEDY);

    auto sdetails  = streamDetails_t{
        .params = &p,
        .wparams = &wparams,
        .mc = nullptr,
        .st = nullptr,
    };

    c.run(&sdetails);
    return 0;
}
