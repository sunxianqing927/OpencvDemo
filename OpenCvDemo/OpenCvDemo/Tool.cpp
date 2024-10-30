#include "pch.h"
#include "Tool.h"
#include <map>
#include <iostream>
#include <filesystem>
#include "Logger.h"

namespace fs = std::filesystem;

std::string WStringToStringGB2312(const std::wstring& wstr) {
    // �����ַ�ת��Ϊ GB2312 �����խ�ַ�
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
    // ���·��û����չ�������������ļ���
    return fs::exists(path) || fs::create_directories(path);
}

std::wstring OpenFileDialog(const COMDLG_FILTERSPEC* pImageTypes, int len) {
    std::wstring filePath;
    // ��ʼ�� COM ��
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (SUCCEEDED(hr)) {
        IFileOpenDialog* pFileOpen = nullptr;
        // �����ļ��򿪶Ի���
        hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));
        if (SUCCEEDED(hr)) {
            pFileOpen->SetFileTypes(len, pImageTypes);
            pFileOpen->SetFileTypeIndex(1); // Ĭ��ѡ���һ���ļ�����
            // ��ʾ���ļ��Ի���
            hr = pFileOpen->Show(NULL);
            // ��ȡ�û�ѡ����ļ�·��
            if (SUCCEEDED(hr)) {
                IShellItem* pItem = nullptr;
                hr = pFileOpen->GetResult(&pItem);

                if (SUCCEEDED(hr)) {
                    PWSTR pszFilePath = nullptr;
                    hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);

                    // ���ļ�·�����浽 std::wstring
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


    ofn.lpstrTitle = L"�����ļ�";
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
        // ���¼�
        SendInput(1, &input, sizeof(INPUT));
    }
    else {
        // ̧���
        input.ki.dwFlags = KEYEVENTF_KEYUP;
        SendInput(1, &input, sizeof(INPUT));
    }
}

bool ActivateAndSendKeys(HWND hwnd, const std::vector<int>& keys) {
    if (hwnd == NULL) {
        LOG_ERROR("Invalid window handle!");
        return false;
    }

    // ����Ŀ�괰��
    if (!SetForegroundWindow(hwnd)) {
        LOG_ERROR("Failed to set foreground window!");
        return false;
    }
    // ȷ�������Ѽ���
    Sleep(100); // �ȴ�һ�£���ȷ�����ڼ���
    // ����������ϼ�
    for (int key : keys) {
        SendKey(key, true);
    }

    // ������ϼ���̧�����м�
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
        // ��ȡ���ڳߴ�
        RECT windowRect;
        if (!GetWindowRect(hwnd, &windowRect)) {
            LOG_ERROR("Failed to get window rect.");
            return {};
        }
        // �������Ļ��������
        LONG left = std::max<LONG>(windowRect.left, 0);
        LONG top = std::max<LONG>(windowRect.top, 0);
        LONG right = std::min<LONG>(windowRect.right, GetSystemMetrics(SM_CXSCREEN));
        LONG bottom = std::min<LONG>(windowRect.bottom, GetSystemMetrics(SM_CYSCREEN));

        // ȷ����Ⱥ͸߶�Ϊ��
        if (right > left && bottom > top) {
            cv::Rect intersectRect(left, top, right - left, bottom - top);
            // �ü�ͼ��
            return fullImage(intersectRect);
        }
        else {
            // ����һ���յ� Mat ���ߴ����������������
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
    GetModuleFileNameA(NULL, path, MAX_PATH); // ��ȡ��ǰ���������·��
    std::string fullPath(path);

    // �������һ����б�ܵ�λ��
    size_t pos = fullPath.find_last_of("\\/");
    if (pos != std::string::npos) {
        return fullPath.substr(0, pos+1); // ����Ŀ¼����
    }
    return ""; // ���û���ҵ������ؿ��ַ���
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

    // ��ȡ��ǰʱ�䲢��ʽ��
    auto now = std::chrono::system_clock::now();
    auto now_c = std::chrono::system_clock::to_time_t(now);
    std::tm localTime;
    localtime_s(&localTime, &now_c);  // ʹ�ð�ȫ�汾�� localtime

    auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    int tenthsOfSeconds = std::min(static_cast<int>(milliseconds.count() / 100), 9); // ������ 0-9 ֮��

    std::ostringstream oss;
    oss << std::put_time(&localTime, "%Y_%m_%d_%H-%M-%S") << "_" << tenthsOfSeconds; // ʮ��֮һ��
    std::string timestamp = oss.str();

    std::string filename = dir+windowTitle + "_" + timestamp + ".jpg";
    // ����ͼ��
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
        // ����һ������Ļ�ߴ���ͬ��λͼ
        HDC hScreenDC = GetDC(NULL);
        HDC hMemoryDC = CreateCompatibleDC(hScreenDC);
        HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
        if (!hBitmap) {
            LOG_ERROR("Failed to create bitmap.");
            return {};
        }

        SelectObject(hMemoryDC, hBitmap);
        BitBlt(hMemoryDC, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), hScreenDC, 0, 0, SRCCOPY);

        // �ü�ͼ��
        cv::Mat fullImage(GetSystemMetrics(SM_CYSCREEN), GetSystemMetrics(SM_CXSCREEN), CV_8UC4);
        GetBitmapBits(hBitmap, LONG(fullImage.total() * fullImage.elemSize()), fullImage.data);

        cv::Mat image;
        cv::cvtColor(fullImage, image, outputFormat);

        DeleteObject(hBitmap);
        DeleteDC(hMemoryDC);
        ReleaseDC(NULL, hScreenDC);
        return image;  // ���ؽ�ͼ��ͼ��
    }
    catch (const std::exception& e) {
        LOG_ERROR(e.what());
    }

    return {};
}
