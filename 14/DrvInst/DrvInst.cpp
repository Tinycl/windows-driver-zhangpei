// DrvInst.cpp : ����Ӧ�ó��������Ϊ��
//

#include "stdafx.h"
#include "DrvInst.h"
#include "DrvInstSheetEx.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CDrvInstApp

BEGIN_MESSAGE_MAP(CDrvInstApp, CWinApp)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()


// CDrvInstApp ����

CDrvInstApp::CDrvInstApp()
{
	// TODO: �ڴ˴���ӹ�����룬
	// ��������Ҫ�ĳ�ʼ�������� InitInstance ��
}


// Ψһ��һ�� CDrvInstApp ����

CDrvInstApp theApp;


// CDrvInstApp ��ʼ��

BOOL CDrvInstApp::InitInstance()
{
	// ���һ�������� Windows XP �ϵ�Ӧ�ó����嵥ָ��Ҫ
	// ʹ�� ComCtl32.dll �汾 6 ����߰汾�����ÿ��ӻ���ʽ��
	//����Ҫ InitCommonControlsEx()�����򣬽��޷��������ڡ�
	CWnd* pWndPrev = CWnd::FindWindow(NULL, "DrvInst ������װС����");
	if (NULL != pWndPrev)
	{
		// If so, does it have any popups?
		CWnd *pWndChild = pWndPrev->GetLastActivePopup();

		// If iconic, restore the main window
		if (pWndPrev->IsIconic())
			pWndPrev->ShowWindow(SW_RESTORE);

		// Bring the main window or its popup to the foreground
		pWndChild->SetForegroundWindow();

		// and you are done activating the other application
		return FALSE;
	}

	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// ��������Ϊ��������Ҫ��Ӧ�ó�����ʹ�õ�
	// �����ؼ��ࡣ
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinApp::InitInstance();

	AfxEnableControlContainer();
	AfxInitRichEdit2();

	// ��׼��ʼ��
	// ���δʹ����Щ���ܲ�ϣ����С
	// ���տ�ִ���ļ��Ĵ�С����Ӧ�Ƴ�����
	// ����Ҫ���ض���ʼ������
	// �������ڴ洢���õ�ע�����
	// TODO: Ӧ�ʵ��޸ĸ��ַ�����
	// �����޸�Ϊ��˾����֯��
	SetRegistryKey(_T("Ӧ�ó��������ɵı���Ӧ�ó���"));

	CBitmap bmpWatermark;
	CBitmap bmpHeader;
	bmpWatermark.LoadBitmap(IDB_BACK);
	bmpHeader.LoadBitmap(IDB_HEAD);

	CDrvInstSheet dlg("DrvInst", NULL, 0, bmpWatermark, NULL, bmpHeader);

	m_pMainWnd = &dlg;
	INT_PTR nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
		// TODO: �ڴ˷��ô����ʱ��
		//  ��ȷ�������رնԻ���Ĵ���
	}
	else if (nResponse == IDCANCEL)
	{
		// TODO: �ڴ˷��ô����ʱ��
		//  ��ȡ�������رնԻ���Ĵ���
	}

	// ���ڶԻ����ѹرգ����Խ����� FALSE �Ա��˳�Ӧ�ó���
	//  ����������Ӧ�ó������Ϣ�á�
	return FALSE;
}
