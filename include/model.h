#include <whisper.h>

#include <cstdio>
#include <string>

#ifndef MODEL_H
#define MODEL_H
typedef struct model_parpams {
    int32_t n_threads = 4;
    int32_t step_ms = 3000;
    int32_t length_ms = 10000;
    int32_t keep_ms = 200;
    int32_t capture_id = 1;
    int32_t max_tokens = 32;
    int32_t audio_ctx = 0;
    int32_t beam_size = -1;

    float vad_thold = 0.6f;
    float freq_thold = 100.0f;

    bool translate = false;
    bool no_fallback = false;
    bool print_special = false;
    bool no_context = true;
    bool no_timestamps = false;
    bool tinydiarize = false;
    bool save_audio = false; // save audio to wav file
    bool use_gpu = true;
    bool flash_attn = false;

    std::string language = "en";
    std::string model = "models/ggml-base.en.bin";
    std::string fname_out;
}model_parpams_t;

// class model_controllor
// unsafe threads
class model_controllor {
public:
    model_controllor() : ctx(nullptr) {
    }

    ~model_controllor() { free_model(); }

    int load_model(const std::string &model_path, model_parpams_t *params);

    void free_model();

    int fill_in_samples(struct whisper_full_params params, const float *samples, int n_samples);

    std::string get_result();

private:
    whisper_context *ctx;
};

#endif