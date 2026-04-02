#include "landmark_detector.hpp"
#include <cstdio>

void LandmarkDetector::Initialize(const std::string& model_path,
                                   int input_w, int input_h,
                                   int crop_w,  int crop_h) {
    input_width  = input_w;
    input_height = input_h;
    crop_width   = crop_w;
    crop_height  = crop_h;

    pipe_offline = GetAIPreprocessPipe();

    // Load the landmark model (.a1model / .m1model)
    model_id = ssne_loadmodel(const_cast<char*>(model_path.c_str()), SSNE_STATIC_ALLOC);
    if (model_id == 0) {
        printf("[ERROR] LandmarkDetector: Failed to load model: %s\n", model_path.c_str());
        return;
    }

    // Apply the normalization parameters stored inside the .a1model so that the
    // offline preprocess pipeline uses the exact mean/std the model was calibrated with.
    int ret = SetNormalize(pipe_offline, model_id);
    if (ret != SSNE_ERRCODE_NO_ERROR) {
        printf("[WARN] LandmarkDetector: SetNormalize returned %d — "
               "ensure the model file embeds normalization params.\n", ret);
    }

    // Allocate the input tensor: crop image will be resized to (input_w × input_h) RGB.
    inputs[0] = create_tensor(input_w, input_h, SSNE_RGB, SSNE_BUF_AI);

    // Allocate the output tensor.
    // Per ssne_api.h the output tensor MUST be pre-allocated before ssne_getoutput().
    // Expected layout: kNumLandmarks * kLandmarkDims float values (e.g. 21 × 2 = 42).
    // TODO: If your model outputs a different number of elements, update
    //       kLandmarkOutputElements in landmark_detector.hpp accordingly.
    outputs[0] = create_tensor(kLandmarkOutputElements, 1, SSNE_FLOAT32, SSNE_BUF_AI);
    if (outputs[0].data == nullptr) {
        printf("[ERROR] LandmarkDetector: Failed to allocate output tensor "
               "(expected %d float elements). Inference will be skipped.\n",
               kLandmarkOutputElements);
    }
}

void LandmarkDetector::Predict(ssne_tensor_t* img_in, LandmarkResult* result) {
    result->Clear();

    // Guard: output tensor must have been allocated successfully in Initialize().
    if (outputs[0].data == nullptr) {
        printf("[ERROR] LandmarkDetector: Output tensor not initialized; skipping inference.\n");
        return;
    }

    // 1. Preprocess: resize the crop image into the model input tensor (inputs[0]).
    int ret = RunAiPreprocessPipe(pipe_offline, *img_in, inputs[0]);
    if (ret != SSNE_ERRCODE_NO_ERROR) {
        printf("[ERROR] LandmarkDetector: RunAiPreprocessPipe failed (%d)\n", ret);
        return;
    }

    // 2. Run inference.
    ret = ssne_inference(model_id, 1, inputs);
    if (ret != SSNE_ERRCODE_NO_ERROR) {
        printf("[ERROR] LandmarkDetector: ssne_inference failed (%d)\n", ret);
        return;
    }

    // 3. Retrieve the output tensor (output_tensor must be pre-allocated — see above).
    ret = ssne_getoutput(model_id, 1, outputs);
    if (ret != SSNE_ERRCODE_NO_ERROR) {
        printf("[ERROR] LandmarkDetector: ssne_getoutput failed (%d)\n", ret);
        return;
    }

    // 4. Post-process: map model output coordinates to crop-image pixel coordinates.
    //
    //    Coordinate system chain:
    //      Model output  : normalized [0, 1] relative to the model input (input_w × input_h)
    //      Crop space    : pixel coordinates within the camera crop (crop_w × crop_h)
    //      Original image: crop_space + (crop_x_offset, crop_y_offset)  — applied in Visualizer
    //
    //    Because RunAiPreprocessPipe resizes crop → model input preserving the full crop area,
    //    the mapping is simply:  pixel_crop = normalized_output × crop_dimension.
    float* out_data = (float*)get_data(outputs[0]);
    if (out_data == nullptr) {
        printf("[ERROR] LandmarkDetector: get_data() returned null for output tensor.\n");
        return;
    }

    result->is_detected = true;
    for (int i = 0; i < kNumLandmarks; i++) {
        Point2D pt;
        pt.x          = out_data[i * kLandmarkDims]     * crop_width;
        pt.y          = out_data[i * kLandmarkDims + 1] * crop_height;
        // visibility is not produced by this model; set to 1.0 as a placeholder.
        // If your .a1model outputs a third value per keypoint (confidence/visibility),
        // set kLandmarkDims = 3 in data_types.hpp and extract it here.
        pt.visibility = 1.0f;
        result->landmarks.push_back(pt);
    }
}

void LandmarkDetector::Release() {
    release_tensor(inputs[0]);
    // Safe release: only free the output tensor if it was successfully allocated.
    if (outputs[0].data != nullptr) {
        release_tensor(outputs[0]);
        outputs[0].data = nullptr;
    }
    if (pipe_offline != nullptr) {
        ReleaseAIPreprocessPipe(pipe_offline);
        pipe_offline = nullptr;
    }
}
