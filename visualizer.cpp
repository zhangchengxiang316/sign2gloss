#include "visualizer.hpp"
#include <cmath>
#include <algorithm>

void Visualizer::Initialize(int img_w, int img_h) {
    osd_device.Initialize(img_w, img_h);
}

sst::device::osd::OsdQuadRangle Visualizer::LineToQuadSolid(Point2D p1, Point2D p2,
                                                            int thickness, int color) {
    using namespace sst::device::osd;

    OsdQuadRangle q;
    q.shape_type = OsdShapeType::VERTEX;
    q.color = color;
    q.alpha = fdevice::TYPE_ALPHA75;
    q.type  = fdevice::TYPE_SOLID;  // solid line
    q.border = 0;

    float dx = p2.x - p1.x;
    float dy = p2.y - p1.y;
    float len = std::sqrt(dx * dx + dy * dy);

    // If too short, still draw a tiny square-ish quad
    if (len < 1e-3f) {
        float half = thickness * 0.5f;
        int x0 = (int)std::round(p1.x - half);
        int y0 = (int)std::round(p1.y - half);
        int x1 = (int)std::round(p1.x + half);
        int y1 = (int)std::round(p1.y + half);

        q.vertex_out.points[0] = {x0, y0};
        q.vertex_out.points[1] = {x0, y1};
        q.vertex_out.points[2] = {x1, y1};
        q.vertex_out.points[3] = {x1, y0};
        q.vertex_in = q.vertex_out;
        return q;
    }

    // Unit normal vector scaled by half thickness
    float half = thickness * 0.5f;
    float nx = -dy / len * half;
    float ny =  dx / len * half;

    // Outer quad vertices (convex, thin rectangle around the segment)
    // A = p1 + n
    // B = p2 + n
    // C = p2 - n
    // D = p1 - n
    int ax = (int)std::round(p1.x + nx);
    int ay = (int)std::round(p1.y + ny);
    int bx = (int)std::round(p2.x + nx);
    int by = (int)std::round(p2.y + ny);
    int cx = (int)std::round(p2.x - nx);
    int cy = (int)std::round(p2.y - ny);
    int dx2 = (int)std::round(p1.x - nx);
    int dy2 = (int)std::round(p1.y - ny);

    q.vertex_out.points[0] = {ax, ay};
    q.vertex_out.points[1] = {bx, by};
    q.vertex_out.points[2] = {cx, cy};
    q.vertex_out.points[3] = {dx2, dy2};

    // For solid quadrangle, set vertex_in equal to vertex_out (implementation-friendly)
    q.vertex_in = q.vertex_out;

    return q;
}

void Visualizer::DrawSkeleton(const LandmarkResult& result,
                              const std::vector<SkeletonConnection>& connections,
                              int offset_y) {
    if (clear_each_frame_) {
        Clear();
    }

    if (!result.is_detected) {
        return;
    }

    std::vector<sst::device::osd::OsdQuadRangle> quads;
    quads.reserve(connections.size());

    for (const auto& conn : connections) {
        int idx1 = conn.first;
        int idx2 = conn.second;

        if (idx1 < 0 || idx2 < 0) continue;
        if ((size_t)idx1 >= result.landmarks.size() || (size_t)idx2 >= result.landmarks.size()) continue;

        Point2D p1 = result.landmarks[idx1];
        Point2D p2 = result.landmarks[idx2];

        // crop -> original image coordinate (only y offset in your crop strategy)
        p1.y += offset_y;
        p2.y += offset_y;

        // draw thin solid line
        quads.push_back(LineToQuadSolid(p1, p2, line_thickness_, /*color=*/2));
    }

    osd_device.Draw(quads);
}

void Visualizer::Clear() {
    osd_device.ClearAll();
}

void Visualizer::Release() {
    osd_device.Release();
}