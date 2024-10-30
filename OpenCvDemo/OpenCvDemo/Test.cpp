#include "pch.h"
#include "Test.h"

#include <opencv2/opencv.hpp>
#include <windows.h>
#include <iostream>

using namespace cv;
using namespace std;

// ����������Ļ����ͼ��
Mat CaptureScreen() {
    HWND desktop = GetDesktopWindow();
    if (!desktop) {
        cerr << "Failed to get desktop window!" << endl;
        return Mat(); // ���ؿյ� Mat ����
    }

    HDC hdc = GetDC(desktop);
    if (!hdc) {
        cerr << "Failed to get device context!" << endl;
        return Mat(); // ���ؿյ� Mat ����
    }

    HDC hdcMem = CreateCompatibleDC(hdc);
    if (!hdcMem) {
        cerr << "Failed to create compatible DC!" << endl;
        ReleaseDC(desktop, hdc);
        return Mat(); // ���ؿյ� Mat ����
    }

    RECT desktopRect;
    if (!GetWindowRect(desktop, &desktopRect)) {
        cerr << "Failed to get window rectangle!" << endl;
        DeleteDC(hdcMem);
        ReleaseDC(desktop, hdc);
        return Mat(); // ���ؿյ� Mat ����
    }

    int width = desktopRect.right - desktopRect.left;
    int height = desktopRect.bottom - desktopRect.top;

    HBITMAP hBitmap = CreateCompatibleBitmap(hdc, width, height);
    if (!hBitmap) {
        cerr << "Failed to create compatible bitmap!" << endl;
        DeleteDC(hdcMem);
        ReleaseDC(desktop, hdc);
        return Mat(); // ���ؿյ� Mat ����
    }

    SelectObject(hdcMem, hBitmap);

    // ����Ļ����ͼ��
    if (!BitBlt(hdcMem, 0, 0, width, height, hdc, 0, 0, SRCCOPY)) {
        cerr << "Failed to BitBlt!" << endl;
        DeleteObject(hBitmap);
        DeleteDC(hdcMem);
        ReleaseDC(desktop, hdc);
        return Mat(); // ���ؿյ� Mat ����
    }

    // ���� BITMAPINFO �ṹ��
    BITMAPINFOHEADER bi;
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = width;
    bi.biHeight = -height;  // ע�⣺���߶ȱ�ʾ���϶��µ�λͼ
    bi.biPlanes = 1;
    bi.biBitCount = 32;  // ÿ������32λ��RGB + Alphaͨ����
    bi.biCompression = BI_RGB;
    bi.biSizeImage = 0;
    bi.biXPelsPerMeter = 0;
    bi.biYPelsPerMeter = 0;
    bi.biClrUsed = 0;
    bi.biClrImportant = 0;

    // ����һ�� Mat �������洢�������Ļ����
    Mat screen(height, width, CV_8UC4); // ʹ�� 4 ͨ����BGRA��

    // �� HBITMAP ��ȡλͼ����
    if (GetDIBits(hdc, hBitmap, 0, height, screen.data, (BITMAPINFO*)&bi, DIB_RGB_COLORS) == 0) {
        cerr << "Failed to get DIB bits!" << endl;
        DeleteObject(hBitmap);
        DeleteDC(hdcMem);
        ReleaseDC(desktop, hdc);
        return Mat(); // ���ؿյ� Mat ����
    }

    // ת��Ϊ BGR ��ʽ
    cvtColor(screen, screen, COLOR_BGRA2BGR); // ת��Ϊ BGR

    // ����
    DeleteObject(hBitmap);
    DeleteDC(hdcMem);
    ReleaseDC(desktop, hdc);

    return screen;
}

// ���������ģ���Ƿ��������Ļ��
bool DetectTemplate(const Mat& screen, const Mat& templ, double threshold) {
    Mat result;
    // ƥ��ģ��
    matchTemplate(screen, templ, result, TM_CCOEFF_NORMED);

    // ��ȡ�����ƥ�������ֵ
    double minVal, maxVal;
    Point minLoc, maxLoc;
    minMaxLoc(result, &minVal, &maxVal, &minLoc, &maxLoc);

    // ���ƥ��ȳ�����ֵ����ʾ�ҵ�ģ��
    if (maxVal >= threshold) {
        // ����ƥ�䵽������
        rectangle(screen, maxLoc, Point(maxLoc.x + templ.cols, maxLoc.y + templ.rows), Scalar(0, 255, 0), 2);
        imshow("Detected", screen); // ��ʾ�����
        return true;
    }

    return false;
}

int test() {
    // ��ȡģ��ͼ��
    Mat templateImage = imread("res/template.png");
    if (templateImage.empty()) {
        cerr << "Failed to load template image!" << endl;
        return -1;
    }

    double threshold = 0.8; // ƥ����ֵ

    while (true) {
        // ������Ļ
        Mat screen = CaptureScreen();
        if (screen.empty()) {
            cerr << "Screen capture failed!" << endl;
            continue; // �����˴�ѭ��
        }

        // ���ģ��
        if (DetectTemplate(screen, templateImage, threshold)) {
            cout << "Template found!" << endl;
            // ������ִ������Ҫ���¼������紥������
        }

        // ����֡�ʣ��ȴ�100����
        if (waitKey(100) >= 0) {
            break;
        }
    }

    return 0;
}
