#include "operator.h"
#include "model.h"
#include "common.h"
#include "common-sdl.h"


#include <thread>

class NodeTranscription;

int TranscriptionController::loadModel(modelDetails_t *details) {
    std::string pullPath = details->baseDir + details->modelName;

    int ret = mc.load_model(pullPath, details->params);
    if (ret != 0) {
        fprintf(stderr, "Failed to load model from %s\n", pullPath.c_str());
        return -1;
    }

    updateModelStatus(ModelStatus_Loaded);
    return 0;
}

void TranscriptionController::freeModel() {
    mc.free_model();
    updateModelStatus(ModelStatus_Empty);
    return;
}

void start_stream(void *argc) {
    auto data = *(run_stream_t *) argc;

    streamDetails_t details = *data.details;
    auto params = *(details.params);
    auto mc = details.mc;
    CallbackInfo &info = data.info;

    audio_async audio(params.length_ms);
    if (!audio.init(params.capture_id, WHISPER_SAMPLE_RATE)) {
        fprintf(stderr, "%s: audio.init() failed!\n", __func__);
        return (void) 1;
    }
    audio.resume();


    const int n_samples_step = (1e-3 * params.step_ms) * WHISPER_SAMPLE_RATE;
    const int n_samples_len = (1e-3 * params.length_ms) * WHISPER_SAMPLE_RATE;
    const int n_samples_keep = (1e-3 * params.keep_ms) * WHISPER_SAMPLE_RATE;
    const int n_samples_30s = (1e-3 * 30000.0) * WHISPER_SAMPLE_RATE;

    const bool use_vad = n_samples_step <= 0; // sliding window mode uses VAD


    std::vector<float> pcmf32(n_samples_30s, 0.0f);
    std::vector<float> pcmf32_old;
    std::vector<float> pcmf32_new(n_samples_30s, 0.0f);

    std::vector<whisper_token> prompt_tokens;

    wav_writer wavWriter;
    // save wav file
    if (params.save_audio) {
        // Get current date/time for filename
        time_t now = time(0);
        char buffer[80];
        strftime(buffer, sizeof(buffer), "%Y%m%d%H%M%S", localtime(&now));
        std::string filename = std::string(buffer) + ".wav";

        wavWriter.open(filename, WHISPER_SAMPLE_RATE, 16, 1);
    }
    printf("[Start speaking]\n");
    fflush(stdout);

    auto t_last = std::chrono::high_resolution_clock::now();
    const auto t_start = t_last;

    bool is_running = true;

    while (is_running) {
        if (params.save_audio) {
            wavWriter.write(pcmf32_new.data(), pcmf32_new.size());
        }
        is_running = sdl_poll_events();

        // stream control
        if (*(details.st) != StreamStatus_RUNNING) {
            is_running = false;
        }

        if (!is_running) {
            break;
        }

        // get audio
        {
            while (true) {
                // handle Ctrl + C
                is_running = sdl_poll_events();
                if (!is_running) {
                    break;
                }
                audio.get(params.step_ms, pcmf32_new);

                if ((int) pcmf32_new.size() > 2 * n_samples_step) {
                    fprintf(stderr, "\n\n%s: WARNING: cannot process audio fast enough, dropping audio ...\n\n",
                            __func__);
                    audio.clear();
                    continue;
                }

                if ((int) pcmf32_new.size() >= n_samples_step) {
                    audio.clear();
                    break;
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }

            const int n_samples_new = pcmf32_new.size();

            // take up to params.length_ms audio from previous iteration
            const int n_samples_take = std::min((int) pcmf32_old.size(),
                                                std::max(0, n_samples_keep + n_samples_len - n_samples_new));

            //printf("processing: take = %d, new = %d, old = %d\n", n_samples_take, n_samples_new, (int) pcmf32_old.size());

            pcmf32.resize(n_samples_new + n_samples_take);

            for (int i = 0; i < n_samples_take; i++) {
                pcmf32[i] = pcmf32_old[pcmf32_old.size() - n_samples_take + i];
            }

            memcpy(pcmf32.data() + n_samples_take, pcmf32_new.data(), n_samples_new * sizeof(float));

            pcmf32_old = pcmf32;
        }

        // run the interface
        {
            whisper_full_params wparams = whisper_full_default_params(
                params.beam_size > 1 ? WHISPER_SAMPLING_BEAM_SEARCH : WHISPER_SAMPLING_GREEDY);

            wparams.print_progress = false;
            wparams.print_special = params.print_special;
            wparams.print_realtime = false;
            wparams.print_timestamps = !params.no_timestamps;
            wparams.translate = params.translate;
            wparams.single_segment = !use_vad;
            wparams.max_tokens = params.max_tokens;
            wparams.language = params.language.c_str();
            wparams.n_threads = params.n_threads;
            wparams.beam_search.beam_size = params.beam_size;

            wparams.audio_ctx = params.audio_ctx;

            wparams.tdrz_enable = params.tinydiarize; // [TDRZ]

            // disable temperature fallback
            //wparams.temperature_inc  = -1.0f;
            wparams.temperature_inc = params.no_fallback ? 0.0f : wparams.temperature_inc;

            wparams.prompt_tokens = params.no_context ? nullptr : prompt_tokens.data();
            wparams.prompt_n_tokens = params.no_context ? 0 : prompt_tokens.size();

            if (mc->fill_in_samples(wparams, pcmf32.data(), pcmf32.size()) != 0) {
                fprintf(stderr, "failed to process audio\n");
                return (void) 6;
            }

            std::shared_ptr<std::string> ret = std::make_shared<std::string>(mc->get_result());
            NodeTranscription *parent = static_cast<NodeTranscription *>(details.userData);
            if (parent) {
                parent->PushResultToJs(info, ret.get(), ret.get()); // 传递结果字符串指针
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }
        }
    }
}

void TranscriptionController::run(const CallbackInfo &info, streamDetails_t *details) {
    printf("TranscriptionController::run\n");
    if (details->mc == nullptr) {
        details->mc = &mc;
    }
    if (details->st == nullptr) {
        details->st = &streamSt;
    }

    if (streamSt == StreamStatus_STOPPED) {
        streamSt = StreamStatus_RUNNING;
        CallbackInfo &ninfo = const_cast<CallbackInfo &>(info);
        auto data = run_stream_t{
            .info = ninfo,
            .details = details,
        };
        start_stream((void*)&data);
    }
    printf("TranscriptionController::run end\n");
}

void TranscriptionController::stop() {
    if (streamSt == StreamStatus_RUNNING) {
        streamSt = StreamStatus_STOPPED;
    }
    SDL_Event event;

    event.type = SDL_QUIT;
    SDL_PushEvent(&event);
}
