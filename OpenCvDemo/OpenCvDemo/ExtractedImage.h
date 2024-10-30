#pragma once
#include <vector>
#include <opencv2/opencv.hpp>
#include<Windows.h>


const int LOAD_FROM_FILE = 0;
const int LOAD_FROM_SCREEN = 1;

class ExtractedImage{
public:
    int Extract();
    std::string GetDesFileName();;
    ExtractedImage(const int type);

private:
    static void OnMouse(int event, int x, int y, int flags, void* param);
    void ProcessAndSaveImages();

    bool m_drawing = false;
    std::string m_filePathString;
    std::vector<cv::Point> m_points;
    cv::Mat m_src;
    cv::Mat m_mask;
    cv::Mat m_result;

};

