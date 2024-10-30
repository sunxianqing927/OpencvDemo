#pragma once
#include<vector>
#include<string>
#include <opencv2/opencv.hpp>

class ImageRecognizer {
public:
    ImageRecognizer(const std::vector<std::string>& templateImageNames, cv::TemplateMatchModes templateMatchMode, bool bGrey, bool bAndOr, int successThreshold, int intervalTime, std::function<bool(cv::Mat&, cv::Mat&)> m_FindCallBack = nullptr);
    bool isValid();
    bool MonitorWindow(const HWND hwnd);
    bool VideoRecognizer(const std::string& videoPath, std::function<void(const std::wstring&)> UpdateProgress);
    void Stop();
    bool Match(const cv::Mat& src, cv::Mat& markedImage);

private:
    static cv::Mat ReadImage(const std::string& filePathStr, const std::string& processType="", bool bGrey= false);
    //tryDefault=true����Ҷ�ͼ���ְ���Grey����,����ʧ�ܣ�������ȥ��Grey,����ԭͼ��Ȼ��ת��λ�Ҷ�ͼ���������±���Ҷ�ͼ
    static cv::Mat ReadGreyImage(const std::string& filePathStr,bool bGreyName=true,bool tryDefault=false);

private:
    bool m_bValid=true;
    const bool m_bGrey;
    const bool m_bAndOr;
    std::atomic<bool> m_stop = false;
    const int m_successThreshold;
    const int m_intervalTime;
    const cv::TemplateMatchModes  m_templateMatchMode;
    std::function<bool(cv::Mat&, cv::Mat&)> m_FindCallBack;
    std::vector<std::pair<cv::Mat, cv::Mat>> m_templates;
};

