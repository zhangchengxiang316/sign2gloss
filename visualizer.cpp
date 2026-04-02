#include "visualizer.hpp"
#include <cmath>

void Visualizer::Initialize(int img_w, int img_h) {
    osd_device.Initialize(img_w, img_h);
}

// 核心：把两点之间的线段，变成一个有宽度的矩形（四边形）
sst::device::osd::OsdQuadRangle Visualizer::LineToQuadrangle(Point2D p1, Point2D p2, int thickness, int color) {
    sst::device::osd::OsdQuadRangle q;
    q.color = color;
    q.alpha = fdevice::TYPE_ALPHA75;
    q.type = fdevice::TYPE_SOLID; // 实心四边形充当线段
    q.border = 0;

    // 计算法向量
    float dx = p2.x - p1.x;
    float dy = p2.y - p1.y;
    float len = std::sqrt(dx * dx + dy * dy);
    
    if (len < 1e-3) return q;

    float nx = -dy / len * (thickness / 2.0f);
    float ny = dx / len * (thickness / 2.0f);

    // 四边形的四个顶点 (基于 OSD device 中 box 的特殊存储方式，这里退化为矩形边界)
    // 注意：OSD demo中的 OsdQuadRangle.box 是 [xmin, ymin, xmax, ymax]，如果是斜线，
    // 原生 osd_device 需要修改以支持真正任意四个点。如果 OSD 只支持正交框，下面为包围盒：
    q.box = {
        std::min(p1.x, p2.x) - thickness, 
        std::min(p1.y, p2.y) - thickness, 
        std::max(p1.x, p2.x) + thickness, 
        std::max(p1.y, p2.y) + thickness
    };
    
    return q;
}

void Visualizer::DrawSkeleton(const LandmarkResult& result, const std::vector<SkeletonConnection>& connections, int offset_y) {
    if (!result.is_detected) {
        Clear();
        return;
    }

    std::vector<sst::device::osd::OsdQuadRangle> quads;

    for (const auto& conn : connections) {
        int idx1 = conn.first;
        int idx2 = conn.second;

        if (idx1 < result.landmarks.size() && idx2 < result.landmarks.size()) {
            Point2D p1 = result.landmarks[idx1];
            Point2D p2 = result.landmarks[idx2];
            
            // 加上裁剪的偏移量，回到原图坐标系
            p1.y += offset_y;
            p2.y += offset_y;

            quads.push_back(LineToQuadrangle(p1, p2, 4, 2)); // 线宽4，颜色索引2
        }
    }

    osd_device.Draw(quads);
}

void Visualizer::Clear() {
    std::vector<sst::device::osd::OsdQuadRangle> empty;
    osd_device.Draw(empty);
}

void Visualizer::Release() {
    osd_device.Release();
}