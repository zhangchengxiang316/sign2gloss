#pragma once
#include "smartsoc/ssne_api.h"
#include "data_types.hpp"
#include <deque>
#include <string>

// ---------------------------------------------------------------------------
// TCN/CTC model output constant.
// kVocabSize must equal the number of output classes in your TCN model,
// INCLUDING the CTC blank token.  Adjust this value to match your .a1model.
//
// To find the correct value:
//   1. Ask the model-conversion team for the output tensor shape specification.
//   2. Or inspect the ONNX model with:  python -c "import onnx; m=onnx.load('tcn.onnx'); print(m.graph.output)"
//   3. Once known, replace 100 below and remove this TODO.
// ---------------------------------------------------------------------------
// TODO: Set kVocabSize to match the actual number of output classes
//       (gloss vocabulary size + 1 blank token) in your TCN/CTC model.
static constexpr int kVocabSize = 100;

class SignRecognizer {
public:
    void Initialize(const std::string& tcn_model_path, int seq_length, int feature_dim);
    // Feed one frame of landmarks; returns the recognised gloss string when
    // enough frames have been buffered, or an empty string otherwise.
    std::string ProcessFrame(const LandmarkResult& current_frame);
    void Release();

private:
    uint16_t model_id = 0;
    int max_seq_length = 0;
    int feature_dim    = 0;

    std::deque<std::vector<Point2D>> frame_buffer;  // sliding window

    ssne_tensor_t inputs[1]  = {};  // zero-initialized; allocated in Initialize()
    ssne_tensor_t outputs[1] = {};  // zero-initialized; allocated in Initialize()

    std::string CTC_Decode(float* probs, int length);
};
