#include "landmark_detector.hpp"
#include <iostream>

void LandmarkDetector::Initialize(const std::string& model_path, int input_w, int input_h) {
    input_width = input_w;
    input_height = input_h;
    pipe_offline = GetAIPreprocessPipe();
    
    model_id = ssne_loadmodel(const_cast<char*>(model_path.c_str()), SSNE_STATIC_ALLOC);
    
    // 获取模型需要的 dtype，例如 SSNE_FLOAT32
    int dtype;
    ssne_get_model_input_dtype(model_id, &dtype);
    inputs[0] = create_tensor(input_w, input_h, SSNE_RGB, SSNE_BUF_AI);
}

void LandmarkDetector::Predict(ssne_tensor_t* img_in, LandmarkResult* result) {
    result->Clear();

    // 1. 预处理：原图 resize 并装载入 inputs[0]
    RunAiPreprocessPipe(pipe_offline, *img_in, inputs[0]);

    // 2. 推理
    ssne_inference(model_id, 1, inputs);
    ssne_getoutput(model_id, 1, outputs);

    // 3. 后处理：提取坐标
    // 注意：这里需要根据你具体编译的模型的输出格式来解析 (例如 21x2 或 21x3)
    float* out_data = (float*)get_data(outputs[0]);
    int num_landmarks = 21; // 以21个手部关键点为例

    result->is_detected = true; // 可加入置信度判断逻辑
    for(int i = 0; i < num_landmarks; i++) {
        Point2D pt;
        // 假设模型输出是归一化的坐标 [0, 1]
        pt.x = out_data[i * 2] * input_width;   
        pt.y = out_data[i * 2 + 1] * input_height;
        result->landmarks.push_back(pt);
    }
}

void LandmarkDetector::Release() {
    release_tensor(inputs[0]);
    release_tensor(outputs[0]);
    ReleaseAIPreprocessPipe(pipe_offline);
}