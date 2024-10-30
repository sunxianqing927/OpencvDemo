#include "pch.h"
#include "FetchFocusWnd.h"
#include<thread>
#include <future>


std::function<void(DWORD event, HWND hwnd)> FetchFocusWnd::FocusWndChangeCallback = nullptr;


void HandleWinEvent(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime) {
    if (event == EVENT_SYSTEM_FOREGROUND) {
        if (FetchFocusWnd::FocusWndChangeCallback) {
            FetchFocusWnd::FocusWndChangeCallback(event, hwnd);
        }
    }
}

FetchFocusWnd::FetchFocusWnd(UINT stopMsg):m_stopMsg(stopMsg) {
}

int FetchFocusWnd::Fetch() {
    m_currentThreadId = GetCurrentThreadId();
    HWINEVENTHOOK hWinEventHook = SetWinEventHook(
        EVENT_SYSTEM_FOREGROUND,
        EVENT_SYSTEM_FOREGROUND,
        NULL,
        HandleWinEvent,
        0,
        0,
        WINEVENT_OUTOFCONTEXT
    );

    if (!hWinEventHook) {
        return 0;
    }

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        if (msg.message == m_stopMsg) {
            break;
        }
    }
    UnhookWinEvent(hWinEventHook);
    return 0;
}

void FetchFocusWnd::Stop() {
    PostThreadMessage(m_currentThreadId, m_stopMsg, 0, 0);
}

void FetchFocusWnd::SetFocusWndChangeCallback(std::function<void(DWORD event, HWND hwnd)> focusWndChangeCallback) {
    FocusWndChangeCallback = focusWndChangeCallback;
}

