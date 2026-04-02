#pragma once
#include "smartsoc/ssne_api.h"
#include "data_types.hpp"
#include <string>

// ---------------------------------------------------------------------------
// Output tensor size constants — must match your .a1model output specification.
// kNumLandmarks and kLandmarkDims are defined in data_types.hpp.
// kLandmarkOutputElements is the total number of float values in the output:
//   21 keypoints × 2 (x, y) = 42 floats.
// TODO: If the model outputs 3D landmarks (x, y, z), ensure kLandmarkDims == 3
//       in data_types.hpp and recompile.
// ---------------------------------------------------------------------------
static constexpr int kLandmarkOutputElements = kNumLandmarks * kLandmarkDims;

class LandmarkDetector {
public:
    // Initialize the landmark detector.
    // input_w / input_h : model input resolution (e.g. 256×256 for MediaPipe).
    // crop_w  / crop_h  : camera crop resolution (e.g. 720×540).
    //   Needed to map normalized model output back to crop-image pixel coordinates.
    void Initialize(const std::string& model_path,
                    int input_w, int input_h,
                    int crop_w,  int crop_h);
    void Predict(ssne_tensor_t* img_in, LandmarkResult* result);
    void Release();

private:
    uint16_t model_id = 0;
    ssne_tensor_t inputs[1]  = {};  // zero-initialized; allocated in Initialize()
    ssne_tensor_t outputs[1] = {};  // zero-initialized; allocated in Initialize()
    AiPreprocessPipe pipe_offline = nullptr;

    // Model input dimensions
    int input_width  = 0;
    int input_height = 0;

    // Camera crop dimensions — used to convert normalized model output to crop-space pixels
    int crop_width  = 0;
    int crop_height = 0;
};
