
// OpenCvDemoDlg.h : header file
//

#pragma once

#include<memory>
#include<functional>

class ImageRecognizer;
class FetchFocusWnd;
namespace cv {
    class Mat;
}


// COpenCvDemoDlg dialog
class COpenCvDemoDlg : public CDialogEx {
    // Construction
public:
    COpenCvDemoDlg(CWnd* pParent = nullptr);	// standard constructor

    // Dialog Data
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_OPENCVDEMO_DIALOG };
#endif

protected:
    virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
    virtual BOOL PreTranslateMessage(MSG* pMsg);
    // Implementation
protected:
    HICON m_hIcon;
    // Generated message map functions
    virtual BOOL OnInitDialog();
    DECLARE_MESSAGE_MAP()

public:
    afx_msg void OnBnClickedAddTempleFromScreen();
    afx_msg void OnBnClickedAddTempleFromImage();
    afx_msg void OnBnClickedWndMatch();
    afx_msg void OnBnClickedSelectWnd();
    afx_msg void OnBnClickedDeleteTemplate();
    afx_msg void OnBnClickedDeleteTemplateAll();
    afx_msg void OnBnClickedAddTemplate();
    afx_msg void OnCbnSelchangeCombo();
    afx_msg void OnBnClickedSetVedioPath();
    afx_msg void OnBnClickedCheckVedio();
    afx_msg void OnEnChangeIntervalTime();
    afx_msg void OnEnChangeThreshold();
    afx_msg LRESULT OnBnClickedUpdateSelectWnd(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnMsgUpdateControl(WPARAM wParam, LPARAM lParam);
private:
    std::shared_ptr<ImageRecognizer>  CreateImageRecognizer(std::function<bool(cv::Mat&, cv::Mat&)> findCallBack);
    int m_threshold = 90;
    int m_intervalTime = 500;
    HWND m_hMainWnd = NULL;
    HWND m_ClientActiveWnd = NULL;
    std::shared_ptr<ImageRecognizer>  m_pWndRecognizer;
    std::shared_ptr<ImageRecognizer>  m_pVideoRecognizer;
    std::shared_ptr<FetchFocusWnd> m_pFetchFocusWnd;

    CToolTipCtrl m_ToolTip;
    CButton m_btSelect;
    CStatic m_staticSelect;
    CComboBox m_cbTemplates;
    CButton m_btAddScreenshot;
    CButton m_btIAddmage;
    CButton m_btIAddTemplate;
    CButton m_btDeleteTemplate;
    CButton m_btDeleteTemplateAll;
    CComboBox m_cbTemplateMatchMode;
    CButton m_ckGrey;
    CButton m_btMatch;
    CButton m_ckMatchAll;
    CEdit m_editThreshold;
    CButton m_btSetVideoPath;
    CEdit m_txtVideoPath;
    CEdit m_editIntervalTime;
    CButton m_btCheckVideo;
    CStatic m_lbProgress;
};
