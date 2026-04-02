#include <iostream>
#include <thread>
#include <mutex>
#include <unistd.h>
#include "camera_processor.hpp"
#include "landmark_detector.hpp"
#include "sign_recognizer.hpp"
#include "visualizer.hpp"

using namespace std;

bool g_exit_flag = false;
std::mutex g_mtx;

void keyboard_listener() {
    string input;
    while (true) {
        cin >> input;
        if (input == "q" || input == "Q") {
            lock_guard<mutex> lock(g_mtx);
            g_exit_flag = true;
            break;
        }
    }
}

int main() {
    // 1. 参数配置
    int img_w = 720, img_h = 1280;
    int crop_w = 720, crop_h = 540;
    int crop_offset_y = 370;

    string mediapipe_path = "/app_demo/app_assets/models/mediapipe.m1model";
    string tcn_path = "/app_demo/app_assets/models/tcn_ctc.m1model";

    // MediaPipe 手部连接拓扑规则
    std::vector<SkeletonConnection> hand_connections = {
        {0,1}, {1,2}, {2,3}, {3,4},       // 大拇指
        {0,5}, {5,6}, {6,7}, {7,8},       // 食指
        {5,9}, {9,10}, {10,11}, {11,12},  // 中指
        {9,13}, {13,14}, {14,15}, {15,16},// 无名指
        {13,17}, {0,17}, {17,18}, {18,19}, {19,20} // 小拇指
    };

    // 2. 初始化环境
    ssne_initial();

    CameraProcessor camera;
    camera.Initialize(img_w, img_h, crop_w, crop_h, crop_offset_y);

    LandmarkDetector detector;
    detector.Initialize(mediapipe_path, 256, 256); // 假设 MediaPipe 输入 256x256

    SignRecognizer recognizer;
    recognizer.Initialize(tcn_path, 30, 42); // 假设序列长30帧，特征维度42(21点*2)

    Visualizer visualizer;
    visualizer.Initialize(img_w, img_h);

    thread listener(keyboard_listener);
    ssne_tensor_t img_sensor;
    LandmarkResult landmark_result;
    string current_gloss = "";

    // 3. 主循环
    while (true) {
        {
            lock_guard<mutex> lock(g_mtx);
            if (g_exit_flag) break;
        }

        // A. 获取图像
        camera.GetImage(&img_sensor);

        // B. 特征提取 (Sign2Gloss -> 第一阶段)
        detector.Predict(&img_sensor, &landmark_result);

        // C. TCN时序分析 & CTC解码 (Sign2Gloss -> 第二阶段)
        string new_gloss = recognizer.ProcessFrame(landmark_result);
        
        // 只有识别出新结果时才打印终端
        if (!new_gloss.empty() && new_gloss != current_gloss) {
            current_gloss = new_gloss;
            cout << "\033[32m[Sign Recognition]: " << current_gloss << "\033[0m" << endl;
        }

        // D. Aurora 图像与骨架可视化
        visualizer.DrawSkeleton(landmark_result, hand_connections, crop_offset_y);
    }

    // 4. 资源释放
    listener.join();
    visualizer.Release();
    recognizer.Release();
    detector.Release();
    camera.Release();
    ssne_release();

    return 0;
}