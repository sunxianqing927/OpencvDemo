#include "pch.h"
#include "ImageRecognizer.h"
#include <filesystem>
#include "Logger.h"
#include "Tool.h"

namespace fs = std::filesystem;

const std::string greyMarker = "Grey";
const std::string MaskMarker = "Mask";


cv::Mat ImageRecognizer::ReadImage(const std::string& filePathStr, const std::string& processType, bool bGrey) {
    fs::path filePath(filePathStr);
    std::string desFilePathStr = (filePath.parent_path() / (filePath.stem().string() + processType + filePath.extension().string())).string();
    return cv::imread(desFilePathStr, bGrey? cv::IMREAD_GRAYSCALE: cv::IMREAD_COLOR);
}

cv::Mat ImageRecognizer::ReadGreyImage(const std::string& filePathStr, bool bGreyName,bool tryDefault) {
    cv::Mat image = ReadImage(filePathStr, bGreyName? "": greyMarker, true);
    if (image.empty() && tryDefault) {
        // 如果灰度文件不存在，加载原图并转为灰度
        fs::path filePath(filePathStr);
        std::string baseName = filePath.stem().string();
        size_t pos = baseName.find(greyMarker);
        if (pos != std::string::npos) {
            std::string srcFilePathStr = (filePath.parent_path() / (baseName.replace(pos, greyMarker.length(), "") + filePath.extension().string())).string();
            image = cv::imread(srcFilePathStr, cv::IMREAD_GRAYSCALE);
            if (!image.empty()) {
                cv::imwrite(filePathStr, image);
            }
        }
    }
    return image;
}

ImageRecognizer::ImageRecognizer(const std::vector<std::string>& templateNames, cv::TemplateMatchModes templateMatchMode, bool bGrey, bool bAndOr, int successThreshold, int intervalTime, std::function<bool(cv::Mat&, cv::Mat&)> findCallBack)
    :m_bGrey(bGrey), m_bAndOr(bAndOr),m_templateMatchMode(templateMatchMode),m_successThreshold(successThreshold), m_intervalTime(intervalTime), m_FindCallBack(findCallBack) {
    m_templates.resize(templateNames.size());
    for (size_t i = 0; i < templateNames.size(); i++) {
        m_templates[i].first = bGrey ? ReadGreyImage(templateNames[i], false, true) : ReadImage(templateNames[i], "");;
        m_templates[i].second =ReadImage(templateNames[i], MaskMarker, true);
        if (m_templates[i].first.empty() || m_templates[i].second.empty()) {
            std::string msg="模板文件加载失败:" + templateNames[i];
            LOG_ERROR(msg);
            AfxMessageBox(StringToWStringGB2312(msg).c_str(), MB_OK | MB_ICONERROR);
            m_bValid=false;
        }
    }
}
bool ImageRecognizer::isValid() {
    return m_bValid;
}

bool ImageRecognizer::MonitorWindow(const HWND hwnd ) {
    if (!hwnd||!IsWindow(hwnd)) {
        std::string msg = "Window not found.";
        LOG_ERROR(msg);
        AfxMessageBox(StringToWStringGB2312(msg).c_str(), MB_OK | MB_ICONERROR);
        return false;
    }

    // 获取窗口标题
    const int length = GetWindowTextLength(hwnd) + 1;
    std::shared_ptr<char[]> title(new char[length]);
    GetWindowTextA(hwnd, title.get(), length);
    std::string windowTitle(title.get());
    while (!m_stop.load() && IsWindow(hwnd)) {
        try {
            auto start = std::chrono::high_resolution_clock::now();
            cv::Mat srcImage = CaptureScreen(hwnd, m_bGrey ? cv::COLOR_BGRA2GRAY : cv::COLOR_BGRA2BGR);
            if (!srcImage.empty()) {
                cv::Mat markedImage;
                bool bMatch = Match(srcImage, markedImage);
                // 根据成功标志输出结果
                if (bMatch) {
                    if (m_FindCallBack) {
                        m_FindCallBack(srcImage, markedImage);
                    }
                }
            }
           
            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double, std::milli> elapsed = end - start;
            if (elapsed.count() < m_intervalTime) {
                std::this_thread::sleep_for(std::chrono::milliseconds(m_intervalTime - static_cast<int>(elapsed.count())));
            }
        }
        catch (const std::exception& e) {
            LOG_ERROR(e.what());
        }
    }
    return 0;
}

bool ImageRecognizer::Match(const cv::Mat& srcImage, cv::Mat& markedImage) {
    markedImage = srcImage;
    bool success = m_bAndOr ? true : false;
    for (const auto& templatePair : m_templates) {
        const cv::Mat& templateImg = templatePair.first;  // 模板图像
        const cv::Mat& maskImg = templatePair.second;      // 掩码图像
        // 进行匹配
        cv::Mat result;
        if (maskImg.empty()) {
            cv::matchTemplate(markedImage, templateImg, result, m_templateMatchMode);
        }
        else {
            cv::matchTemplate(markedImage, templateImg, result, m_templateMatchMode, maskImg);
        }
        // 计算匹配度
        double maxVal;
        cv::minMaxLoc(result, nullptr, &maxVal);
        // 根据匹配方法处理匹配度
        double matchPercentage;
        if (m_templateMatchMode == cv::TM_CCOEFF_NORMED || m_templateMatchMode == cv::TM_CCORR_NORMED) {
            // 对于这两种方法，返回值在 [0, 1] 之间，值越大越好
            matchPercentage = maxVal * 100.0; // 转换为百分比
        }
        else if (m_templateMatchMode == cv::TM_SQDIFF_NORMED) {
            // 对于这种方法，返回值越小越好
            matchPercentage = (1.0 - maxVal) * 100.0; // 转换为百分比
        }
        else {
            LOG_ERROR("Unsupported match method!");
            break;
        }

        if (matchPercentage >= m_successThreshold) {
            // 找到最佳匹配的位置
            double minVal, maxVal;
            cv::Point minLoc, maxLoc;
            cv::minMaxLoc(result, &minVal, &maxVal, &minLoc, &maxLoc);
            // 设定匹配区域
            cv::Rect matchRect(maxLoc.x, maxLoc.y, templateImg.cols, templateImg.rows);
            // 在原图上绘制匹配框
            cv::rectangle(markedImage, matchRect, cv::Scalar(0, 255, 0), 2);
        }

        // 判断成功条件
        if (m_bAndOr&& matchPercentage < m_successThreshold) { // AND 操作
            success = false;
            break; // 提前退出
        }
        else if (!m_bAndOr&&matchPercentage >= m_successThreshold) { // OR 操作
            success = true;         // 找到一个成功匹配
            break; // 提前退出
        }
    }

    return success;
}

void ImageRecognizer::Stop() {
    m_stop.store(true);;
}

bool ImageRecognizer::VideoRecognizer(const std::string& videoPath, std::function<void(const std::wstring&)> UpdateProgress) {
    cv::VideoCapture cap(videoPath);
    if (!cap.isOpened()) {
        std::string msg = "Error opening video file:" + videoPath;
        LOG_ERROR(msg);
        AfxMessageBox(StringToWStringGB2312(msg).c_str(), MB_OK | MB_ICONERROR);
        return false;
    }

    double totalFrames = cap.get(cv::CAP_PROP_FRAME_COUNT); // 获取视频的总帧数
    double fps = cap.get(cv::CAP_PROP_FPS); // 获取帧率
    int frameInterval = static_cast<int>(fps * m_intervalTime/1000); // 计算帧间隔
    int frameCount = 0;


    // 计算总时间
    double totalTime = totalFrames / fps;
    int totalMinutes = static_cast<int>(totalTime / 60); // 总分钟
    int totalSeconds = static_cast<int>(totalTime) % 60; // 总秒

    // 循环直到提取所有帧
    while (!m_stop&&frameCount < totalFrames) {
        // 计算当前时间（秒）
        double currentTime = frameCount / fps;
        int currentMinutes = static_cast<int>(currentTime / 60); // 当前分钟
        int currentSeconds = static_cast<int>(currentTime) % 60; // 当前秒
        // 更新进度字符串
        std::wstring updateStr = L"当前进度: " + std::to_wstring(currentMinutes) + L"分 " + std::to_wstring(currentSeconds) + L"秒 / "
            + std::to_wstring(totalMinutes) + L"分 " + std::to_wstring(totalSeconds) + L"秒";
        UpdateProgress(updateStr);

        cap.set(cv::CAP_PROP_POS_FRAMES, frameCount); // 设置当前帧位置
        cv::Mat frame; // 创建一个Mat对象用于存储帧
        cap.read(frame); // 读取当前帧
        if (frame.empty()) { // 检查是否读取成功
            break; // 如果读取失败，跳出循环
        }

        cv::Mat markedImage;
        cv::Mat grayFrame;
        if (m_bGrey) {
            cv::cvtColor(frame, grayFrame, cv::COLOR_BGR2GRAY);
        }
        bool bMatch = Match(m_bGrey? grayFrame:frame, markedImage);
        // 根据成功标志输出结果
        if (bMatch) {
            if (m_FindCallBack) {
                m_FindCallBack(frame, markedImage);
            }
        }

        frameCount += frameInterval; // 更新帧计数器，跳到下一个帧
    }
    return true;
}

     