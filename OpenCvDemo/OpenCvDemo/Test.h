#pragma once

namespace cv {
    class Mat;
}

// 函数：从屏幕捕获图像
cv::Mat CaptureScreen();

// 函数：检测模板
bool DetectTemplate(const cv::Mat& screen, const cv::Mat& templateImage, double threshold);

int test();

