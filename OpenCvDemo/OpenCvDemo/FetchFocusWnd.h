#pragma once
#include<Windows.h>
#include<functional>

class FetchFocusWnd
{
    friend void HandleWinEvent(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime);
public:
    FetchFocusWnd(UINT stopMsg);
    int Fetch();
    void Stop();
    static void SetFocusWndChangeCallback(std::function<void(DWORD event, HWND hwnd)> focusWndChangeCallback);
private:
    static std::function<void(DWORD event, HWND hwnd)> FocusWndChangeCallback;
    UINT m_stopMsg =0;
    DWORD m_currentThreadId=0;
};

