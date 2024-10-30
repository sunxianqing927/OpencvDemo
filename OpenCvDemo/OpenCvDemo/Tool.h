#pragma once
#include <string>
#include <opencv2/opencv.hpp>

std::string WStringToStringGB2312(const std::wstring& wstr);
std::wstring StringToWStringGB2312(const std::string& str);

bool EnsureDirectoryExistForFile(const std::string& filePath);

std::string OpenImageFileDialogA();
std::string OpenVedioFileDialogA();

std::wstring OpenImageFileDialogW();
std::wstring OpenVedioFileDialogW();

std::string GetSaveFileNameWithDialog();

void SendKey(int key, bool keyDown);
bool ActivateAndSendKeys(HWND hwnd, const std::vector<int>& keys);

cv::Mat CaptureScreen(int outputFormat = cv::IMREAD_COLOR);
cv::Mat CaptureScreen(HWND hwnd, int outputFormat = cv::IMREAD_COLOR);

std::string GetExeDirectory();


void SaveImageWithTimestamp(std::string windowTitle, const cv::Mat& image);