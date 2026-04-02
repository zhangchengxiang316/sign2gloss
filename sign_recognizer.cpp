#include "sign_recognizer.hpp"
#include <algorithm>
#include <cstdio>

void SignRecognizer::Initialize(const std::string& tcn_model_path,
                                 int seq_length, int feature_dim) {
    max_seq_length    = seq_length;
    this->feature_dim = feature_dim;

    model_id = ssne_loadmodel(const_cast<char*>(tcn_model_path.c_str()), SSNE_STATIC_ALLOC);
    if (model_id == 0) {
        printf("[ERROR] SignRecognizer: Failed to load model: %s\n", tcn_model_path.c_str());
        return;
    }

    // Input tensor: [seq_length × feature_dim] floats
    // feature_dim should equal kNumLandmarks * kLandmarkDims (e.g. 21 * 2 = 42).
    inputs[0] = create_tensor(feature_dim, seq_length, SSNE_FLOAT32, SSNE_BUF_AI);

    // Output tensor: [seq_length × kVocabSize] floats (per-timestep class log-probabilities).
    // Per ssne_api.h the output tensor MUST be pre-allocated before ssne_getoutput().
    // TODO: Confirm kVocabSize matches your actual .a1model output shape.
    outputs[0] = create_tensor(kVocabSize, seq_length, SSNE_FLOAT32, SSNE_BUF_AI);
    if (outputs[0].data == nullptr) {
        printf("[ERROR] SignRecognizer: Failed to allocate output tensor "
               "(vocab_size=%d, seq_len=%d). Inference will be skipped.\n",
               kVocabSize, seq_length);
    }
}

std::string SignRecognizer::ProcessFrame(const LandmarkResult& current_frame) {
    // 1. Build a fixed-size frame entry.
    //    Use kNumLandmarks as the canonical landmark count to guarantee the input
    //    tensor is always filled completely regardless of what the detector returns.
    //    If the frame has no valid detection, fill with zeros to maintain temporal
    //    continuity (zero-padding strategy).
    std::vector<Point2D> frame_entry(kNumLandmarks, {0.0f, 0.0f, 0.0f});
    if (current_frame.is_detected) {
        int n = std::min(static_cast<int>(current_frame.landmarks.size()), kNumLandmarks);
        for (int i = 0; i < n; i++) {
            frame_entry[i] = current_frame.landmarks[i];
        }
    }

    // 2. Maintain the sliding-window frame buffer.
    frame_buffer.push_back(frame_entry);
    if (frame_buffer.size() < static_cast<size_t>(max_seq_length)) {
        return "";  // Still collecting frames — wait for a full window.
    }
    if (frame_buffer.size() > static_cast<size_t>(max_seq_length)) {
        frame_buffer.pop_front();
    }

    // 3. Fill the input tensor from the current sliding window.
    float* input_ptr = (float*)get_data(inputs[0]);
    if (input_ptr == nullptr) {
        printf("[ERROR] SignRecognizer: Input tensor data pointer is null.\n");
        return "";
    }
    for (int t = 0; t < max_seq_length; ++t) {
        for (int p = 0; p < kNumLandmarks; ++p) {
            // base_idx layout: [time, landmark, coord] stored as flat array
            int base_idx = t * feature_dim + p * kLandmarkDims;
            input_ptr[base_idx]     = frame_buffer[t][p].x;
            input_ptr[base_idx + 1] = frame_buffer[t][p].y;
        }
    }

    // 4. Run TCN inference.
    int ret = ssne_inference(model_id, 1, inputs);
    if (ret != SSNE_ERRCODE_NO_ERROR) {
        printf("[ERROR] SignRecognizer: ssne_inference failed (%d)\n", ret);
        return "";
    }

    // 5. Retrieve output tensor (must be pre-allocated — see Initialize()).
    ret = ssne_getoutput(model_id, 1, outputs);
    if (ret != SSNE_ERRCODE_NO_ERROR) {
        printf("[ERROR] SignRecognizer: ssne_getoutput failed (%d)\n", ret);
        return "";
    }

    // 6. CTC greedy decode.
    float* out_probs = (float*)get_data(outputs[0]);
    if (out_probs == nullptr) {
        printf("[ERROR] SignRecognizer: Output data pointer is null.\n");
        return "";
    }
    return CTC_Decode(out_probs, max_seq_length);
}

std::string SignRecognizer::CTC_Decode(float* probs, int length) {
    // CTC greedy decode: at each time step pick the most probable class;
    // collapse consecutive duplicates and remove blank tokens (class index 0).
    // TODO: Replace the stub below with a real vocab lookup table once the
    //       gloss vocabulary is finalised.
    (void)probs;
    (void)length;
    return "HELLO";  // stub — replace with actual decoding logic
}

void SignRecognizer::Release() {
    release_tensor(inputs[0]);
    // Safe release: only free the output tensor if it was successfully allocated.
    if (outputs[0].data != nullptr) {
        release_tensor(outputs[0]);
        outputs[0].data = nullptr;
    }
}
