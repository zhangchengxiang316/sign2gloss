#pragma once
#include "smartsoc/ssne_api.h"
#include <array>

class CameraProcessor {
public:
    void Initialize(int img_w, int img_h, int crop_w, int crop_h, int crop_offset_y);
    void GetImage(ssne_tensor_t* img_sensor);
    void Release();

    std::array<int, 2> img_shape;
private:
    uint8_t format_online;
};