#include "pch.h"
#include "OpenCvDemoDlg.h"
#include <iostream>
#include <GdiPlus.h>
#include<Windows.h>
#include <filesystem>
#include <afxdialogex.h>

#include "OpenCvDemo.h"
#include "framework.h"
#include"ThreadPool.h"
#include"FetchFocusWnd.h"
#include "Tool.h"
#include "ExtractedImage.h"
#include "ImageRecognizer.h"
#include "Logger.h"

namespace fs = std::filesystem;
using namespace Gdiplus;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

const UINT WM_CUSTOM_MESSAGE_STOP = WM_USER + 1;
const UINT WM_UPDATE_ACTIVE_WND = WM_USER + 2;
const UINT WM_UPDATE_BTN_ENABLE = WM_USER + 3;

const CString CHECK_VIDEO_STR = L"开始检测视频";
const CString CHECK_WND_STR= L"开始检测窗口";

static const std::wstring greyInfo = L"处理速度：灰度图像的处理速度通常较快，因为计算量较小。"
L"匹配精度：在需要形状和结构特征的情况下，灰度图像的匹配精度可能保持良好，但在依赖颜色信息的应用中，可能会降低精度。";


static std::vector<std::tuple<std::wstring, std::wstring, cv::TemplateMatchModes>> normTemplateMethods = {
       {L"TM_CCORR_NORMED",
        L"特点：对噪声不敏感，但对亮度变化影响较大。",
        cv::TM_CCORR_NORMED},

    {L"TM_SQDIFF_NORMED",
        L"特点：对噪声敏感，但适合灰度范围大的图像，归一化减少亮度影响。",
        cv::TM_SQDIFF_NORMED},


    {L"TM_CCOEFF_NORMED",
        L"特点：适合相对均匀亮度的图像，排除平均亮度的影响。",
        cv::TM_CCOEFF_NORMED}
};


// COpenCvDemoDlg dialog

bool MonitorWindowFindCallback(cv::Mat& srcImage, cv::Mat& markedImage,HWND hwnd,const std::string& windowTitle) {
    ActivateAndSendKeys(hwnd, { VK_SPACE });     //给目标发送键盘指令
    cv::imshow("Window Result", markedImage);
    cv::waitKey(0);
    SaveImageWithTimestamp(windowTitle, srcImage);
    ActivateAndSendKeys(hwnd, { VK_SPACE });     //给目标发送键盘指令
    Sleep(1500);
    return true;
}

bool VideoRecognizerFindCallback(cv::Mat& srcImage, cv::Mat&markedImage,const std::string& videoPath){
    cv::imshow("Video Result", markedImage);
    cv::waitKey(0);
    SaveImageWithTimestamp(fs::path(videoPath).stem().string(), srcImage);
    return true;
}

COpenCvDemoDlg::COpenCvDemoDlg(CWnd* pParent /*=nullptr*/)
    : CDialogEx(IDD_OPENCVDEMO_DIALOG, pParent) {
    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void COpenCvDemoDlg::DoDataExchange(CDataExchange* pDX) {
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_BUTTON3, m_btSelect);
    DDX_Control(pDX, IDC_STATIC1, m_staticSelect);
    DDX_Control(pDX, IDC_COMBO1, m_cbTemplates);
    DDX_Control(pDX, IDC_BUTTON1, m_btAddScreenshot);
    DDX_Control(pDX, IDC_BUTTON7, m_btIAddmage);
    DDX_Control(pDX, IDC_BUTTON9, m_btIAddTemplate);
    DDX_Control(pDX, IDC_BUTTON6, m_btDeleteTemplate);
    DDX_Control(pDX, IDC_BUTTON8, m_btDeleteTemplateAll);
    DDX_Control(pDX, IDC_COMBO2, m_cbTemplateMatchMode);
    DDX_Control(pDX, IDC_CHECK1, m_ckGrey);
    DDX_Control(pDX, IDC_BUTTON2, m_btMatch);
    DDX_Control(pDX, IDC_CHECK2, m_ckMatchAll);
    DDX_Control(pDX, IDC_EDIT1, m_editThreshold);
    DDX_Control(pDX, IDC_BUTTON5, m_btSetVideoPath);
    DDX_Control(pDX, IDC_EDIT2, m_txtVideoPath);
    DDX_Control(pDX, IDC_EDIT3, m_editIntervalTime);
    DDX_Control(pDX, IDC_BUTTON4, m_btCheckVideo);
    DDX_Control(pDX, IDC_STATIC3, m_lbProgress);
}

// DDX/DDV support

BOOL COpenCvDemoDlg::PreTranslateMessage(MSG* pMsg) {
    m_ToolTip.RelayEvent(pMsg);
    return CDialogEx::PreTranslateMessage(pMsg);
}

BEGIN_MESSAGE_MAP(COpenCvDemoDlg, CDialogEx)
    ON_MESSAGE(WM_UPDATE_ACTIVE_WND, &COpenCvDemoDlg::OnBnClickedUpdateSelectWnd) // 映射自定义消息
    ON_MESSAGE(WM_UPDATE_BTN_ENABLE, &COpenCvDemoDlg::OnMsgUpdateControl) // 映射自定义消息
    ON_BN_CLICKED(IDC_BUTTON1, &COpenCvDemoDlg::OnBnClickedAddTempleFromScreen)
    ON_BN_CLICKED(IDC_BUTTON2, &COpenCvDemoDlg::OnBnClickedWndMatch)
    ON_BN_CLICKED(IDC_BUTTON3, &COpenCvDemoDlg::OnBnClickedSelectWnd)
    ON_BN_CLICKED(IDC_BUTTON7, &COpenCvDemoDlg::OnBnClickedAddTempleFromImage)
    ON_BN_CLICKED(IDC_BUTTON6, &COpenCvDemoDlg::OnBnClickedDeleteTemplate)
    ON_BN_CLICKED(IDC_BUTTON8, &COpenCvDemoDlg::OnBnClickedDeleteTemplateAll)
    ON_BN_CLICKED(IDC_BUTTON9, &COpenCvDemoDlg::OnBnClickedAddTemplate)
    ON_CBN_SELCHANGE(IDC_COMBO2, &COpenCvDemoDlg::OnCbnSelchangeCombo)
    ON_BN_CLICKED(IDC_BUTTON5, &COpenCvDemoDlg::OnBnClickedSetVedioPath)
    ON_BN_CLICKED(IDC_BUTTON4, &COpenCvDemoDlg::OnBnClickedCheckVedio)
    ON_EN_CHANGE(IDC_EDIT3, &COpenCvDemoDlg::OnEnChangeIntervalTime)
    ON_EN_CHANGE(IDC_EDIT1, &COpenCvDemoDlg::OnEnChangeThreshold)
END_MESSAGE_MAP()

BOOL COpenCvDemoDlg::OnInitDialog() {
    CDialogEx::OnInitDialog();
    m_hMainWnd = AfxGetMainWnd()->GetSafeHwnd();
    m_ToolTip.Create(this);
    m_ToolTip.Activate(TRUE);
    for (auto& item : normTemplateMethods) {
        m_cbTemplateMatchMode.AddString(std::get<0>(item).c_str());
    }
    m_ToolTip.AddTool(&m_cbTemplateMatchMode, std::get<1>(normTemplateMethods[0]).c_str());
    m_cbTemplateMatchMode.SetCurSel(0);

    m_ToolTip.AddTool(&m_ckGrey, greyInfo.c_str());
    m_ckGrey.SetCheck(BST_UNCHECKED);
    m_ckMatchAll.SetCheck(BST_UNCHECKED);

    CString text;
    text.Format(_T("%d"), m_threshold); // 将整数格式化为 CStri
    m_editThreshold.SetWindowText(text);

    text.Format(_T("%d"), m_intervalTime); // 将整数格式化为 CStri
    m_editIntervalTime.SetWindowText(text);

    std::function<void(DWORD event, HWND hwnd)> FocusWndChangeCallback = [hMainWnd=m_hMainWnd](DWORD event, HWND hwnd) {
        if (event == EVENT_SYSTEM_FOREGROUND && hwnd != hMainWnd && IsWindow(hMainWnd)) {
            CString newText;
            int length = ::GetWindowTextLength(hwnd) + 1;
            ::GetWindowText(hwnd, newText.GetBuffer(length), length);
            newText.ReleaseBuffer();
            ::SendMessage(hMainWnd, WM_UPDATE_ACTIVE_WND, (WPARAM)hwnd, (LPARAM)&newText);
        }
        };

    FetchFocusWnd::SetFocusWndChangeCallback(FocusWndChangeCallback);
    return TRUE;
}

void COpenCvDemoDlg::OnBnClickedAddTempleFromScreen() {
    m_btAddScreenshot.EnableWindow(FALSE);
    auto fExtractedImage = [hMainWnd = m_hMainWnd] {
        ExtractedImage extractedImage(LOAD_FROM_SCREEN);
        int res = extractedImage.Extract();
        ::PostMessage(hMainWnd, WM_UPDATE_BTN_ENABLE, IDC_BUTTON1, (LPARAM) new std::string(extractedImage.GetDesFileName()));
        return res;
        };

    ThreadPool<int>::getInstance().addTask(fExtractedImage);
}

void COpenCvDemoDlg::OnBnClickedWndMatch() {
    try {
        CString text;
        m_btMatch.GetWindowText(text);
        if (text == CHECK_WND_STR) {
            if (!m_ClientActiveWnd || !IsWindow(m_ClientActiveWnd)) {
                AfxMessageBox(_T("当前追踪的活动窗口不可用，请重新设置"), MB_OK | MB_ICONERROR);
                return;
            }
            m_staticSelect.GetWindowText(text);

            std::function<bool(cv::Mat&, cv::Mat&)> findCallBack = std::bind(MonitorWindowFindCallback, std::placeholders::_1, std::placeholders::_2, m_ClientActiveWnd, std::string(CT2A(text)));
            m_pWndRecognizer=CreateImageRecognizer(findCallBack);
            if (m_pWndRecognizer) {
                m_btMatch.SetWindowText(L"停止");
                ThreadPool<int>::getInstance().addTask([pImageRecognizer = m_pWndRecognizer, ClientActiveWnd = m_ClientActiveWnd, hMainWnd = m_hMainWnd] {
                    int res = pImageRecognizer->MonitorWindow(ClientActiveWnd);
                    ::PostMessage(hMainWnd, WM_UPDATE_BTN_ENABLE, IDC_BUTTON2, (LPARAM)&CHECK_WND_STR);
                    return res;
                    }
                );
            }
        }
        else {
            if (m_pWndRecognizer) {
                m_pWndRecognizer->Stop();
                m_pWndRecognizer.reset();
            }
        }
    }
    catch (const std::exception&e) {
        LOG_ERROR(e.what());
    }
    catch (...) {
        LOG_ERROR("未知错误");
    }

}

void COpenCvDemoDlg::OnBnClickedSelectWnd() {
    CString buttonText;
    m_btSelect.GetWindowText(buttonText);
    if (buttonText == L"设置") {
        m_btSelect.SetWindowText(L"确定");
        m_staticSelect.SetWindowText(L"");
        m_ClientActiveWnd = NULL;
        m_pFetchFocusWnd = std::make_shared<FetchFocusWnd>(WM_CUSTOM_MESSAGE_STOP);
        auto fFetchFocusWnd = [pFetchFocusWnd= m_pFetchFocusWnd, hMainWnd = m_hMainWnd]() {
            int res=pFetchFocusWnd->Fetch();
            ::PostMessage(hMainWnd, WM_UPDATE_BTN_ENABLE, IDC_BUTTON3, 0);
            return res;
            };

        ThreadPool<int>::getInstance().addTask(fFetchFocusWnd);
    }
    else if (buttonText == L"确定") {
        m_btSelect.SetWindowText(L"设置");
        m_btSelect.EnableWindow(FALSE);
        if (m_pFetchFocusWnd) {
            m_pFetchFocusWnd->Stop();
            m_pFetchFocusWnd.reset();
        }

    }
}

LRESULT COpenCvDemoDlg::OnBnClickedUpdateSelectWnd(WPARAM wParam, LPARAM lParam) {
    m_ClientActiveWnd = (HWND)wParam;
    CString* newText = reinterpret_cast<CString*>(lParam); // 新文本内容
    m_staticSelect.SetWindowText(*newText);
    return 0;
}

LRESULT COpenCvDemoDlg::OnMsgUpdateControl(WPARAM wParam, LPARAM lParam) {
    CWnd* pControl = GetDlgItem((int)wParam);
    if (pControl != nullptr) {
        pControl->EnableWindow(TRUE);
    }

    if ((IDC_BUTTON7 == wParam || IDC_BUTTON1 == wParam) && lParam != NULL) {
        std::string* pStr = (std::string*)lParam;
        if (pStr->size()) {
            m_cbTemplates.AddString(StringToWStringGB2312(*pStr).c_str());
            if (m_cbTemplates.GetCurSel() == CB_ERR) {
                m_cbTemplates.SetCurSel(0);
            }
        }
        delete pStr;
    }else if (IDC_BUTTON4== wParam) {
        CString* pStr = (CString*)lParam;
        m_btCheckVideo.SetWindowText(*pStr);
        m_pVideoRecognizer.reset();
    }
    else if (IDC_STATIC3 == wParam) {
        std::wstring* pStr = (std::wstring*)lParam;
        if (pStr) {
            m_lbProgress.SetWindowText(pStr->c_str());
            delete pStr;
        }
    }
    else if (IDC_BUTTON2 == wParam) {
        CString* pStr = (CString*)lParam;
        m_btMatch.SetWindowText(*pStr);
    }

    return 0;
}

std::shared_ptr<ImageRecognizer> COpenCvDemoDlg::CreateImageRecognizer(std::function<bool(cv::Mat&, cv::Mat&)> findCallBack) {
    int itemCount = m_cbTemplates.GetCount(); // 获取条目数
    if (itemCount == 0) {
        AfxMessageBox(_T("请配置模板图片"), MB_OK | MB_ICONERROR);
        return {};
    }

    CString text;
    std::vector<std::string> templateImageNames;
    for (int i = 0; i < itemCount; ++i) {
        m_cbTemplates.GetLBText(i, text); // 获取条目字符串
        templateImageNames.push_back(std::string(CT2A(text))); // 将 CString 转换为 std::string 并添加到 vector 中
    }
    m_editThreshold.GetWindowTextW(text);
    m_threshold = _ttoi(text); // 转换为整数

    m_editIntervalTime.GetWindowTextW(text);
    m_intervalTime = _ttoi(text); // 转换为整数

    bool bGrey = m_ckGrey.GetCheck() == BST_CHECKED;
    bool bAndOr = m_ckMatchAll.GetCheck() == BST_CHECKED;
    cv::TemplateMatchModes templateMatchMode = std::get<2>(normTemplateMethods[m_cbTemplateMatchMode.GetCurSel()]);
    return std::make_shared<ImageRecognizer>(templateImageNames, templateMatchMode, bGrey, bAndOr, m_threshold, m_intervalTime, findCallBack);
}

void COpenCvDemoDlg::OnBnClickedAddTempleFromImage() {
    m_btIAddmage.EnableWindow(FALSE);
    auto cmd = [this] {
        ExtractedImage extractedImage(LOAD_FROM_FILE);
        int res = extractedImage.Extract();
        ::PostMessage(m_hMainWnd, WM_UPDATE_BTN_ENABLE, IDC_BUTTON7, (LPARAM) new std::string(extractedImage.GetDesFileName()));
        return res;
        };
    ThreadPool<int>::getInstance().addTask(cmd);
}

void COpenCvDemoDlg::OnBnClickedDeleteTemplate() {
    int currentIndex = m_cbTemplates.GetCurSel();
    int leftCnt = m_cbTemplates.DeleteString(currentIndex);
    if (leftCnt <= 0) {
        m_cbTemplates.SetCurSel(-1);
    }
    else if (currentIndex < leftCnt) {
        m_cbTemplates.SetCurSel(currentIndex);
    }
    else if (currentIndex - 1 < leftCnt) {
        m_cbTemplates.SetCurSel(currentIndex - 1);
    }
}

void COpenCvDemoDlg::OnBnClickedDeleteTemplateAll() {
    m_cbTemplates.ResetContent();
}

void COpenCvDemoDlg::OnBnClickedAddTemplate() {
    std::wstring filePath = OpenImageFileDialogW();
    m_cbTemplates.AddString(filePath.c_str());
    if (m_cbTemplates.GetCurSel() == CB_ERR) {
        m_cbTemplates.SetCurSel(0);
    }
}

void COpenCvDemoDlg::OnCbnSelchangeCombo() {
    int idx = m_cbTemplateMatchMode.GetCurSel();
    m_ToolTip.AddTool(&m_cbTemplateMatchMode, std::get<1>(normTemplateMethods[idx]).c_str());
}


void COpenCvDemoDlg::OnBnClickedSetVedioPath() {
    m_txtVideoPath.SetWindowTextW(OpenVedioFileDialogW().c_str());
}


void COpenCvDemoDlg::OnBnClickedCheckVedio() {
    CString text;
    m_btCheckVideo.GetWindowText(text);
    if (text== CHECK_VIDEO_STR) {
        m_txtVideoPath.GetWindowText(text);
        std::string filePath(CT2A(text.GetString()));
        if (!fs::exists(filePath)) {
            AfxMessageBox(_T("当前文件路径无效："+ text), MB_OK | MB_ICONERROR);
            return;
        }
        std::function<bool(cv::Mat&, cv::Mat&)> findCallBack = std::bind(VideoRecognizerFindCallback, std::placeholders::_1, std::placeholders::_2, std::string(CT2A(text)));
        m_pVideoRecognizer = CreateImageRecognizer(findCallBack);
        if (m_pVideoRecognizer) {

            std::function<void(const std::wstring&)> UpdateProgressCallback = [hMainWnd = m_hMainWnd](const std::wstring& msg) {
                std::wstring* newString = new std::wstring(msg);
                ::PostMessage(hMainWnd, WM_UPDATE_BTN_ENABLE, IDC_STATIC3, (LPARAM)newString);
                };
            ThreadPool<int>::getInstance().addTask([pImageRecognizer = m_pVideoRecognizer, filePath, hMainWnd=m_hMainWnd, UpdateProgressCallback] {
                 int res=pImageRecognizer->VideoRecognizer(filePath, UpdateProgressCallback);
                 ::PostMessage(hMainWnd, WM_UPDATE_BTN_ENABLE, IDC_BUTTON4, (LPARAM)&CHECK_VIDEO_STR);
                 return   res;
                });
            
            m_btCheckVideo.SetWindowText(L"停止");
        }
    }
    else {
        if (m_pVideoRecognizer) {
            m_pVideoRecognizer->Stop();
            m_pVideoRecognizer.reset();
        }
    }
}

void COpenCvDemoDlg::OnEnChangeIntervalTime() {
    CString text;
    m_editThreshold.GetWindowText(text);
    int value = _ttoi(text); // 转换为整数
    if (value <= 100 && value >= 0) {
        m_threshold = value;
    }
    else {
        text.Format(_T("%d"), m_threshold); // 将整数格式化为 CStri
        m_editThreshold.SetWindowText(text);
    }
}

void COpenCvDemoDlg::OnEnChangeThreshold() {
    CString text;
    m_editThreshold.GetWindowText(text);
    int value = _ttoi(text); // 转换为整数
    if (value <= 100 && value >= 0) {
        m_threshold = value;
    }
    else {
        text.Format(_T("%d"), m_threshold); // 将整数格式化为 CStri
        m_editThreshold.SetWindowText(text);
    }
}
