// mfc.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������
#include "Worker.h"


// CmfcApp:
// �йش����ʵ�֣������ mfc.cpp
//

class CcmeApp : public CWinApp
{
public:
	CcmeApp();

// ��д
	public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern CcmeApp theApp;
extern Worker g_worker;

