#pragma once
#include "osd-device.hpp"
#include "data_types.hpp"
#include <vector>

class Visualizer {
public:
    void Initialize(int img_w, int img_h);

    // Draw skeleton lines (solid thin lines).
    // offset_y: crop-to-original Y offset (e.g. 370)
    void DrawSkeleton(const LandmarkResult& result,
                      const std::vector<SkeletonConnection>& connections,
                      int offset_y);

    void Clear();
    void Release();

    // Optional settings
    void SetLineThickness(int t) { line_thickness_ = (t < 1 ? 1 : t); }
    void SetClearEachFrame(bool en) { clear_each_frame_ = en; }

private:
    sst::device::osd::OsdDevice osd_device;

    // Create a solid thin quadrangle representing a line segment p1->p2
    sst::device::osd::OsdQuadRangle LineToQuadSolid(Point2D p1, Point2D p2,
                                                    int thickness, int color);

private:
    int line_thickness_ = 2;     // thin but visible
    bool clear_each_frame_ = true; // avoid ghosting by default
};