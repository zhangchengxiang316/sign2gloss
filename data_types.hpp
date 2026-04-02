#pragma once
#include <vector>
#include <array>
#include <string>

// 2D 关键点
struct Point2D {
    float x;
    float y;
    float visibility; // 可选，关键点置信度
};

// 单帧关键点检测结果
struct LandmarkResult {
    std::vector<Point2D> landmarks; // 例如：21个手部关键点或33个身体关键点
    bool is_detected;               // 当前帧是否检测到有效目标
    
    void Clear() {
        landmarks.clear();
        is_detected = false;
    }
};

// 骨架连线规则 (起点索引，终点索引)
using SkeletonConnection = std::pair<int, int>;