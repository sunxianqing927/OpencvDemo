#pragma once

namespace cv {
    class Mat;
}

// ����������Ļ����ͼ��
cv::Mat CaptureScreen();

// ���������ģ��
bool DetectTemplate(const cv::Mat& screen, const cv::Mat& templateImage, double threshold);

int test();

