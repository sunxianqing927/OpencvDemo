#include "pch.h"
#include "Tool.h"
#include <map>
#include <iostream>
#include <filesystem>
#include "Logger.h"

namespace fs = std::filesystem;

std::string WStringToStringGB2312(const std::wstring& wstr) {
    // 将宽字符转换为 GB2312 编码的窄字符
    int size_needed = WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string str(size_needed, 0);
    WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), (int)wstr.size(), &str[0], size_needed, NULL, NULL);
    return str;
}

std::wstring StringToWStringGB2312(const std::string& str) {
    int wchars_num = MultiByteToWideChar(936, 0, str.c_str(), -1, NULL, 0);
    std::wstring wstr(wchars_num, L'\0');
    MultiByteToWideChar(936, 0, str.c_str(), -1, &wstr[0], wchars_num);
    return wstr;
}

bool EnsureDirectoryExistForFile(const std::string& filePath) {
    fs::path path(filePath);
    if (path.has_extension()) {
        fs::path directory = path.parent_path();
        if (directory.empty() || fs::exists(directory)) {
            return true;
        }
        return fs::create_directories(directory);
    }
    // 如果路径没有扩展名，假设它是文件夹
    return fs::exists(path) || fs::create_directories(path);
}

std::wstring OpenFileDialog(const COMDLG_FILTERSPEC* pImageTypes, int len) {
    std::wstring filePath;
    // 初始化 COM 库
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (SUCCEEDED(hr)) {
        IFileOpenDialog* pFileOpen = nullptr;
        // 创建文件打开对话框
        hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));
        if (SUCCEEDED(hr)) {
            pFileOpen->SetFileTypes(len, pImageTypes);
            pFileOpen->SetFileTypeIndex(1); // 默认选择第一种文件类型
            // 显示打开文件对话框
            hr = pFileOpen->Show(NULL);
            // 获取用户选择的文件路径
            if (SUCCEEDED(hr)) {
                IShellItem* pItem = nullptr;
                hr = pFileOpen->GetResult(&pItem);

                if (SUCCEEDED(hr)) {
                    PWSTR pszFilePath = nullptr;
                    hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);

                    // 将文件路径保存到 std::wstring
                    if (SUCCEEDED(hr)) {
                        filePath = pszFilePath;
                        CoTaskMemFree(pszFilePath);
                    }
                    pItem->Release();
                }
            }
            pFileOpen->Release();
        }
        CoUninitialize();
    }
    return filePath;

}

std::string OpenImageFileDialogA() {
    return WStringToStringGB2312(OpenImageFileDialogW());
}

std::string OpenVedioFileDialogA() {
    return WStringToStringGB2312(OpenVedioFileDialogW());
}

std::wstring OpenImageFileDialogW() {
    const COMDLG_FILTERSPEC imageTypes[] = {
{ L"Image Files", L"*.jpg;*.jpeg;*.png;*.bmp;*.gif" },
{ L"All Files", L"*.*" }
    };
    return OpenFileDialog(imageTypes, ARRAYSIZE(imageTypes));
}

std::wstring OpenVedioFileDialogW() {
    const COMDLG_FILTERSPEC videoTypes[] = {
{ L"Video Files", L"*.mp4;*.avi;*.mov;*.mkv;*.wmv;*.flv" },
{ L"All Files", L"*.*" }
    };
    return OpenFileDialog(videoTypes, ARRAYSIZE(videoTypes));
}

std::string GetSaveFileNameWithDialog() {
    fs::path defaultDir = fs::current_path();
    if (!fs::exists(defaultDir)) {
        fs::create_directory(defaultDir);
    }

    OPENFILENAMEW ofn;
    static std::map<std::wstring, std::wstring> fileTypeMap = {
        {L"JPEG Files (*.jpg)", L".jpg"},
        {L"JPEG Files (*.jpeg)", L".jpeg"},
        {L"PNG Files (*.png)", L".png"},
        {L"BMP Files (*.bmp)", L".bmp"},
        {L"GIF Files (*.gif)", L".gif"},
        {L"TIF Files (*.tif)", L".tif"},
        {L"TIF Files (*.tiff)", L".tiff"},
        {L"PPM Files (*.ppm)", L".ppm"},
        {L"WEBP Files (*.webp)", L".webp"}
    };

    wchar_t szFile[MAX_PATH] = {};
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter =
        L"JPEG Files (*.jpg)\0*.jpg\0"
        L"JPEG Files (*.jpeg)\0*.jpeg\0"
        L"PNG Files (*.png)\0*.png\0"
        L"BMP Files (*.bmp)\0*.bmp\0"
        L"GIF Files (*.gif)\0*.gif\0"
        L"TIF Files (*.tif)\0*.tif\0"
        L"TIF Files (*.tiff)\0*.tiff\0"
        L"PPM Files (*.ppm)\0*.ppm\0"
        L"WEBP Files (*.webp)\0*.webp\0";


    ofn.lpstrTitle = L"保存文件";
    ofn.lpstrInitialDir = defaultDir.c_str();
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
    if (GetSaveFileNameW(&ofn)) {
        return WStringToStringGB2312(std::wstring(ofn.lpstrFile) + fileTypeMap[ofn.lpstrFilter]);
    }
    return "";
}

void SendKey(int key, bool keyDown) {
    INPUT input = { 0 };
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = key;
    if (keyDown) {
        // 按下键
        SendInput(1, &input, sizeof(INPUT));
    }
    else {
        // 抬起键
        input.ki.dwFlags = KEYEVENTF_KEYUP;
        SendInput(1, &input, sizeof(INPUT));
    }
}

bool ActivateAndSendKeys(HWND hwnd, const std::vector<int>& keys) {
    if (hwnd == NULL) {
        LOG_ERROR("Invalid window handle!");
        return false;
    }

    // 激活目标窗口
    if (!SetForegroundWindow(hwnd)) {
        LOG_ERROR("Failed to set foreground window!");
        return false;
    }
    // 确保窗口已激活
    Sleep(100); // 等待一下，以确保窗口激活
    // 按下所有组合键
    for (int key : keys) {
        SendKey(key, true);
    }

    // 发送组合键后，抬起所有键
    for (int key : keys) {
        SendKey(key, false);
    }

    return true;
}

cv::Mat CaptureScreen(HWND hwnd, int outputFormat) {
    try {
        cv::Mat fullImage = CaptureScreen(outputFormat);
        if (fullImage.empty()) {
            return {};
        }
        // 获取窗口尺寸
        RECT windowRect;
        if (!GetWindowRect(hwnd, &windowRect)) {
            LOG_ERROR("Failed to get window rect.");
            return {};
        }
        // 计算和屏幕交集区域
        LONG left = std::max<LONG>(windowRect.left, 0);
        LONG top = std::max<LONG>(windowRect.top, 0);
        LONG right = std::min<LONG>(windowRect.right, GetSystemMetrics(SM_CXSCREEN));
        LONG bottom = std::min<LONG>(windowRect.bottom, GetSystemMetrics(SM_CYSCREEN));

        // 确保宽度和高度为正
        if (right > left && bottom > top) {
            cv::Rect intersectRect(left, top, right - left, bottom - top);
            // 裁剪图像
            return fullImage(intersectRect);
        }
        else {
            // 返回一个空的 Mat 或者处理不符合条件的情况
            return cv::Mat();
        }
    }
    catch (const std::exception& e) {
        LOG_ERROR(e.what());
    }

    return {};
}

std::string GetCurrentExecutableDirectoryImpl() {
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH); // 获取当前程序的完整路径
    std::string fullPath(path);

    // 查找最后一个反斜杠的位置
    size_t pos = fullPath.find_last_of("\\/");
    if (pos != std::string::npos) {
        return fullPath.substr(0, pos+1); // 返回目录部分
    }
    return ""; // 如果没有找到，返回空字符串
}
std::string GetExeDirectory() {
    static std::string exePath = GetCurrentExecutableDirectoryImpl();
    return exePath;
}

void SaveImageWithTimestamp(std::string windowTitle, const cv::Mat& image) {
    std::string dir = GetExeDirectory() + "Image/" + windowTitle+"/";
    if (!fs::exists(dir)) {
        fs::create_directories(dir);
    }

    // 获取当前时间并格式化
    auto now = std::chrono::system_clock::now();
    auto now_c = std::chrono::system_clock::to_time_t(now);
    std::tm localTime;
    localtime_s(&localTime, &now_c);  // 使用安全版本的 localtime

    auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    int tenthsOfSeconds = std::min(static_cast<int>(milliseconds.count() / 100), 9); // 限制在 0-9 之间

    std::ostringstream oss;
    oss << std::put_time(&localTime, "%Y_%m_%d_%H-%M-%S") << "_" << tenthsOfSeconds; // 十分之一秒
    std::string timestamp = oss.str();

    std::string filename = dir+windowTitle + "_" + timestamp + ".jpg";
    // 保存图像
    if (!cv::imwrite(filename, image)) {
        std::cerr << "Failed to save image: " << filename << std::endl;
    }
    else {
        std::cout << "Image saved as: " << filename << std::endl;
        LOG_ERROR("Image saved as: " + filename);
    }
}

cv::Mat CaptureScreen(int outputFormat) {
    try {
        // 创建一个与屏幕尺寸相同的位图
        HDC hScreenDC = GetDC(NULL);
        HDC hMemoryDC = CreateCompatibleDC(hScreenDC);
        HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
        if (!hBitmap) {
            LOG_ERROR("Failed to create bitmap.");
            return {};
        }

        SelectObject(hMemoryDC, hBitmap);
        BitBlt(hMemoryDC, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), hScreenDC, 0, 0, SRCCOPY);

        // 裁剪图像
        cv::Mat fullImage(GetSystemMetrics(SM_CYSCREEN), GetSystemMetrics(SM_CXSCREEN), CV_8UC4);
        GetBitmapBits(hBitmap, LONG(fullImage.total() * fullImage.elemSize()), fullImage.data);

        cv::Mat image;
        cv::cvtColor(fullImage, image, outputFormat);

        DeleteObject(hBitmap);
        DeleteDC(hMemoryDC);
        ReleaseDC(NULL, hScreenDC);
        return image;  // 返回截图的图像
    }
    catch (const std::exception& e) {
        LOG_ERROR(e.what());
    }

    return {};
}
