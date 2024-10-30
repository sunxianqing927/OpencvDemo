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
        // ����Ҷ��ļ������ڣ�����ԭͼ��תΪ�Ҷ�
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
            std::string msg="ģ���ļ�����ʧ��:" + templateNames[i];
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

    // ��ȡ���ڱ���
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
                // ���ݳɹ���־������
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
        const cv::Mat& templateImg = templatePair.first;  // ģ��ͼ��
        const cv::Mat& maskImg = templatePair.second;      // ����ͼ��
        // ����ƥ��
        cv::Mat result;
        if (maskImg.empty()) {
            cv::matchTemplate(markedImage, templateImg, result, m_templateMatchMode);
        }
        else {
            cv::matchTemplate(markedImage, templateImg, result, m_templateMatchMode, maskImg);
        }
        // ����ƥ���
        double maxVal;
        cv::minMaxLoc(result, nullptr, &maxVal);
        // ����ƥ�䷽������ƥ���
        double matchPercentage;
        if (m_templateMatchMode == cv::TM_CCOEFF_NORMED || m_templateMatchMode == cv::TM_CCORR_NORMED) {
            // ���������ַ���������ֵ�� [0, 1] ֮�䣬ֵԽ��Խ��
            matchPercentage = maxVal * 100.0; // ת��Ϊ�ٷֱ�
        }
        else if (m_templateMatchMode == cv::TM_SQDIFF_NORMED) {
            // �������ַ���������ֵԽСԽ��
            matchPercentage = (1.0 - maxVal) * 100.0; // ת��Ϊ�ٷֱ�
        }
        else {
            LOG_ERROR("Unsupported match method!");
            break;
        }

        if (matchPercentage >= m_successThreshold) {
            // �ҵ����ƥ���λ��
            double minVal, maxVal;
            cv::Point minLoc, maxLoc;
            cv::minMaxLoc(result, &minVal, &maxVal, &minLoc, &maxLoc);
            // �趨ƥ������
            cv::Rect matchRect(maxLoc.x, maxLoc.y, templateImg.cols, templateImg.rows);
            // ��ԭͼ�ϻ���ƥ���
            cv::rectangle(markedImage, matchRect, cv::Scalar(0, 255, 0), 2);
        }

        // �жϳɹ�����
        if (m_bAndOr&& matchPercentage < m_successThreshold) { // AND ����
            success = false;
            break; // ��ǰ�˳�
        }
        else if (!m_bAndOr&&matchPercentage >= m_successThreshold) { // OR ����
            success = true;         // �ҵ�һ���ɹ�ƥ��
            break; // ��ǰ�˳�
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

    double totalFrames = cap.get(cv::CAP_PROP_FRAME_COUNT); // ��ȡ��Ƶ����֡��
    double fps = cap.get(cv::CAP_PROP_FPS); // ��ȡ֡��
    int frameInterval = static_cast<int>(fps * m_intervalTime/1000); // ����֡���
    int frameCount = 0;


    // ������ʱ��
    double totalTime = totalFrames / fps;
    int totalMinutes = static_cast<int>(totalTime / 60); // �ܷ���
    int totalSeconds = static_cast<int>(totalTime) % 60; // ����

    // ѭ��ֱ����ȡ����֡
    while (!m_stop&&frameCount < totalFrames) {
        // ���㵱ǰʱ�䣨�룩
        double currentTime = frameCount / fps;
        int currentMinutes = static_cast<int>(currentTime / 60); // ��ǰ����
        int currentSeconds = static_cast<int>(currentTime) % 60; // ��ǰ��
        // ���½����ַ���
        std::wstring updateStr = L"��ǰ����: " + std::to_wstring(currentMinutes) + L"�� " + std::to_wstring(currentSeconds) + L"�� / "
            + std::to_wstring(totalMinutes) + L"�� " + std::to_wstring(totalSeconds) + L"��";
        UpdateProgress(updateStr);

        cap.set(cv::CAP_PROP_POS_FRAMES, frameCount); // ���õ�ǰ֡λ��
        cv::Mat frame; // ����һ��Mat�������ڴ洢֡
        cap.read(frame); // ��ȡ��ǰ֡
        if (frame.empty()) { // ����Ƿ��ȡ�ɹ�
            break; // �����ȡʧ�ܣ�����ѭ��
        }

        cv::Mat markedImage;
        cv::Mat grayFrame;
        if (m_bGrey) {
            cv::cvtColor(frame, grayFrame, cv::COLOR_BGR2GRAY);
        }
        bool bMatch = Match(m_bGrey? grayFrame:frame, markedImage);
        // ���ݳɹ���־������
        if (bMatch) {
            if (m_FindCallBack) {
                m_FindCallBack(frame, markedImage);
            }
        }

        frameCount += frameInterval; // ����֡��������������һ��֡
    }
    return true;
}

     