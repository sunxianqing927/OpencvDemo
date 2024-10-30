#include"pch.h"
#include "ExtractedImage.h"
#include <filesystem>
#include "Tool.h"
#include "Logger.h"
namespace fs = std::filesystem;


ExtractedImage::ExtractedImage(const int type) {
    if (type== LOAD_FROM_FILE) {
        std::string srcFileName = OpenImageFileDialogA();
        m_src = cv::imread(srcFileName);
    }
    else if (type == LOAD_FROM_SCREEN) {
        m_src = CaptureScreen(cv::IMREAD_COLOR);
    }               
}

void ExtractedImage::ProcessAndSaveImages() {
    if (m_mask.empty() || m_mask.channels() != 1) {
        LOG_ERROR("");
        return;
    }

    cv::Rect boundingBox = cv::boundingRect(m_mask);
    cv::Mat cropped_mask = m_mask(boundingBox);
    cv::Mat cropped_result = m_result(boundingBox);

    m_filePathString = GetSaveFileNameWithDialog();
    if (m_filePathString.empty()) {
        AfxMessageBox(_T("未选择文件"), MB_OK | MB_ICONERROR);
        LOG_ERROR("未选择文件");
        return;
    }

    fs::path filePath(m_filePathString);
    std::string baseName = filePath.stem().string();
    fs::path maskFile = filePath.parent_path() / (baseName + "Mask" + filePath.extension().string());
    fs::path greyFile = filePath.parent_path() / (baseName + "Grey" + filePath.extension().string());

    cv::imwrite(maskFile.string(), cropped_mask);
    cv::imwrite(filePath.string(), cropped_result);

    cv::Mat gray_result;
    cv::cvtColor(cropped_result, gray_result, cv::COLOR_BGR2GRAY);
    cv::imwrite(greyFile.string(), gray_result);
}

void ExtractedImage::OnMouse(int event, int x, int y, int flags, void* param) {
    bool& drawing = *(bool*)((void**)param)[0];
    std::vector<cv::Point>& points = *(std::vector<cv::Point>*)((void**)param)[1];
    cv::Mat& src = *(cv::Mat*)((void**)param)[2];
    cv::Mat& mask = *(cv::Mat*)((void**)param)[3];
    cv::Mat& result = *(cv::Mat*)((void**)param)[4];

    if (event == cv::EVENT_LBUTTONDOWN) {
        drawing = true;
        points.clear();
        points.push_back(cv::Point(x, y));
    }
    else if (event == cv::EVENT_MOUSEMOVE && drawing) {
        points.push_back(cv::Point(x, y));
        cv::Mat temp = src.clone();
        for (size_t i = 1; i < points.size(); ++i) {
            cv::line(temp, points[i - 1], points[i], cv::Scalar(0, 255, 0), 2);
        }
        cv::imshow("Draw Contour", temp);
    }
    else if (event == cv::EVENT_LBUTTONUP) {
        drawing = false;
        if (points.empty()) {
            return;
        }
        mask = cv::Mat::zeros(src.size(), CV_8UC1);
        std::vector<std::vector<cv::Point>> contours{ points };
        cv::drawContours(mask, contours, -1, cv::Scalar(255), cv::FILLED);
        src.copyTo(result, mask);
        cv::imshow("Extracted Image", result);
    }
}

int ExtractedImage::Extract() {
    if (m_src.empty()) {
        AfxMessageBox(_T("图像文件错误"), MB_OK | MB_ICONERROR);
        LOG_ERROR("图像文件错误");
        return -1;
    }

    cv::namedWindow("Draw Contour");
    cv::imshow("Draw Contour", m_src);
    void* pVoid[] = { &m_drawing ,&m_points,&m_src,&m_mask,&m_result };
    cv::setMouseCallback("Draw Contour", OnMouse, pVoid);

    cv::waitKey(0);
    cv::destroyAllWindows();
    ProcessAndSaveImages();
    return 0;
}

std::string ExtractedImage::GetDesFileName() {
    return m_filePathString;
}
