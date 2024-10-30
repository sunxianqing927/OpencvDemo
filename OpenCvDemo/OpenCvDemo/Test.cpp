#include "pch.h"
#include "Test.h"

#include <opencv2/opencv.hpp>
#include <windows.h>
#include <iostream>

using namespace cv;
using namespace std;

// 函数：从屏幕捕获图像
Mat CaptureScreen() {
    HWND desktop = GetDesktopWindow();
    if (!desktop) {
        cerr << "Failed to get desktop window!" << endl;
        return Mat(); // 返回空的 Mat 对象
    }

    HDC hdc = GetDC(desktop);
    if (!hdc) {
        cerr << "Failed to get device context!" << endl;
        return Mat(); // 返回空的 Mat 对象
    }

    HDC hdcMem = CreateCompatibleDC(hdc);
    if (!hdcMem) {
        cerr << "Failed to create compatible DC!" << endl;
        ReleaseDC(desktop, hdc);
        return Mat(); // 返回空的 Mat 对象
    }

    RECT desktopRect;
    if (!GetWindowRect(desktop, &desktopRect)) {
        cerr << "Failed to get window rectangle!" << endl;
        DeleteDC(hdcMem);
        ReleaseDC(desktop, hdc);
        return Mat(); // 返回空的 Mat 对象
    }

    int width = desktopRect.right - desktopRect.left;
    int height = desktopRect.bottom - desktopRect.top;

    HBITMAP hBitmap = CreateCompatibleBitmap(hdc, width, height);
    if (!hBitmap) {
        cerr << "Failed to create compatible bitmap!" << endl;
        DeleteDC(hdcMem);
        ReleaseDC(desktop, hdc);
        return Mat(); // 返回空的 Mat 对象
    }

    SelectObject(hdcMem, hBitmap);

    // 从屏幕复制图像
    if (!BitBlt(hdcMem, 0, 0, width, height, hdc, 0, 0, SRCCOPY)) {
        cerr << "Failed to BitBlt!" << endl;
        DeleteObject(hBitmap);
        DeleteDC(hdcMem);
        ReleaseDC(desktop, hdc);
        return Mat(); // 返回空的 Mat 对象
    }

    // 定义 BITMAPINFO 结构体
    BITMAPINFOHEADER bi;
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = width;
    bi.biHeight = -height;  // 注意：负高度表示自上而下的位图
    bi.biPlanes = 1;
    bi.biBitCount = 32;  // 每个像素32位（RGB + Alpha通道）
    bi.biCompression = BI_RGB;
    bi.biSizeImage = 0;
    bi.biXPelsPerMeter = 0;
    bi.biYPelsPerMeter = 0;
    bi.biClrUsed = 0;
    bi.biClrImportant = 0;

    // 创建一个 Mat 对象来存储捕获的屏幕数据
    Mat screen(height, width, CV_8UC4); // 使用 4 通道（BGRA）

    // 从 HBITMAP 获取位图数据
    if (GetDIBits(hdc, hBitmap, 0, height, screen.data, (BITMAPINFO*)&bi, DIB_RGB_COLORS) == 0) {
        cerr << "Failed to get DIB bits!" << endl;
        DeleteObject(hBitmap);
        DeleteDC(hdcMem);
        ReleaseDC(desktop, hdc);
        return Mat(); // 返回空的 Mat 对象
    }

    // 转换为 BGR 格式
    cvtColor(screen, screen, COLOR_BGRA2BGR); // 转换为 BGR

    // 清理
    DeleteObject(hBitmap);
    DeleteDC(hdcMem);
    ReleaseDC(desktop, hdc);

    return screen;
}

// 函数：检测模板是否出现在屏幕中
bool DetectTemplate(const Mat& screen, const Mat& templ, double threshold) {
    Mat result;
    // 匹配模板
    matchTemplate(screen, templ, result, TM_CCOEFF_NORMED);

    // 获取结果中匹配度最大的值
    double minVal, maxVal;
    Point minLoc, maxLoc;
    minMaxLoc(result, &minVal, &maxVal, &minLoc, &maxLoc);

    // 如果匹配度超过阈值，表示找到模板
    if (maxVal >= threshold) {
        // 画出匹配到的区域
        rectangle(screen, maxLoc, Point(maxLoc.x + templ.cols, maxLoc.y + templ.rows), Scalar(0, 255, 0), 2);
        imshow("Detected", screen); // 显示检测结果
        return true;
    }

    return false;
}

int test() {
    // 读取模板图像
    Mat templateImage = imread("res/template.png");
    if (templateImage.empty()) {
        cerr << "Failed to load template image!" << endl;
        return -1;
    }

    double threshold = 0.8; // 匹配阈值

    while (true) {
        // 捕获屏幕
        Mat screen = CaptureScreen();
        if (screen.empty()) {
            cerr << "Screen capture failed!" << endl;
            continue; // 跳过此次循环
        }

        // 检测模板
        if (DetectTemplate(screen, templateImage, threshold)) {
            cout << "Template found!" << endl;
            // 在这里执行你想要的事件，例如触发操作
        }

        // 控制帧率，等待100毫秒
        if (waitKey(100) >= 0) {
            break;
        }
    }

    return 0;
}
