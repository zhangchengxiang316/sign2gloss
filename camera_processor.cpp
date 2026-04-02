#include "camera_processor.hpp"
#include <iostream>

void CameraProcessor::Initialize(int img_w, int img_h, int crop_w, int crop_h, int crop_offset_y) {
    img_shape = {img_w, img_h};
    format_online = SSNE_RGB; // 假设MediaPipe需要RGB，如果是灰度用SSNE_Y_8
    
    // 根据传感器画面大小设置裁剪
    OnlineSetCrop(kPipeline0, 0, img_w, crop_offset_y, crop_offset_y + crop_h);
    OnlineSetOutputImage(kPipeline0, format_online, crop_w, crop_h);
    
    int res = OpenOnlinePipeline(kPipeline0);
    if (res != 0) {
        printf("[ERROR] Failed to open camera pipeline: %d\n", res);
    }
}

void CameraProcessor::GetImage(ssne_tensor_t* img_sensor) {
    if (GetImageData(img_sensor, kPipeline0, kSensor0, 0) != 0) {
        printf("[WARN] Failed to get image data!\n");
    }
}

void CameraProcessor::Release() {
    CloseOnlinePipeline(kPipeline0);
}