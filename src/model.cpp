#include "model.h"

int model_controllor::load_model(const std::string &model_path, model_parpams_t *params) {
    // 如果已经加载了模型，先释放它
    if (ctx != nullptr) {
        free_model();
    }

    // 设置 GPU 相关参数
    auto wparams = whisper_context_default_params();
    wparams.use_gpu = params->use_gpu;
    wparams.flash_attn = params->flash_attn;

    // 加载模型
    ctx = whisper_init_from_file_with_params(model_path.c_str(), wparams);
    if (ctx == nullptr) {
        fprintf(stderr, "Failed to load model from %s\n", model_path.c_str());
        return -1;
    }

    return 0;
}

void model_controllor::free_model() {
    if (ctx != nullptr) {
        whisper_free(ctx);
        ctx = nullptr;
    }
}

int model_controllor::fill_in_samples(struct whisper_full_params wparams, const float *samples, int n_samples) {
    // progress paramas
    if (whisper_full(ctx, wparams, samples, n_samples) != 0) {
        return 6;
    }
    return 0;
}

std::string model_controllor::get_result() {
    std::string output = "";

    const int n_segments = whisper_full_n_segments(ctx);
    for (int i = 0; i < n_segments; ++i) {
        const char *text = whisper_full_get_segment_text(ctx, i);

        output += text;
        if (whisper_full_get_segment_speaker_turn_next(ctx, i)) {
            output += " [SPEAKER_TURN]";
        }

        output += "\n";
    }
    return output;
}
