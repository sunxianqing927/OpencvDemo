
// OpenCvDemo.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'pch.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols


// COpenCvDemoApp:
// See OpenCvDemo.cpp for the implementation of this class
//

class COpenCvDemoApp : public CWinApp
{
public:
	COpenCvDemoApp();

// Overrides
public:
	virtual BOOL InitInstance();

// Implementation

	DECLARE_MESSAGE_MAP()
};

extern COpenCvDemoApp theApp;
