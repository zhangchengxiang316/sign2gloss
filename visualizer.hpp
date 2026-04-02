#pragma once
#include "osd-device.hpp"
#include "data_types.hpp"
#include <vector>

class Visualizer {
public:
    void Initialize(int img_w, int img_h);
    // 画骨架连线
    void DrawSkeleton(const LandmarkResult& result, const std::vector<SkeletonConnection>& connections, int offset_y);
    // 清除画面
    void Clear();
    void Release();

private:
    sst::device::osd::OsdDevice osd_device;
    // 工具函数：将一条线段转为 OSD 的四边形
    sst::device::osd::OsdQuadRangle LineToQuadrangle(Point2D p1, Point2D p2, int thickness, int color);
};