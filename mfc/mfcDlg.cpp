// mfcDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "mfc.h"
#include "mfcDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

UINT StartTrade( LPVOID pParam )
{
	return g_worker.startTrade();
}

UINT StopTrade( LPVOID pParam )
{
	return g_worker.stopTrade();
}

UINT StartQuote( LPVOID pParam )
{
	return g_worker.startQuote();
}

UINT StopQuote( LPVOID pParam )
{
	return g_worker.stopQuote();
}


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// 对话框数据
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()


// CmfcDlg 对话框

CmfcDlg::CmfcDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CmfcDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_securityID = 0;
}

void CmfcDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST3, m_lvDealInfo);
	DDX_Control(pDX, IDC_COMBO1, m_cbExchange);
	DDX_Control(pDX, IDC_COMBO2, m_cbCommodity);
	DDX_Control(pDX, IDC_COMBO3, m_cbContractCode);
	DDX_Control(pDX, IDC_COMBO4, m_cbBS);
	DDX_Control(pDX, IDC_COMBO5, m_cbEntrustType);
	DDX_Control(pDX, IDC_EDIT1, m_editPrice);
	DDX_Control(pDX, IDC_EDIT2, m_editQuantity);
	DDX_Control(pDX, IDC_LIST2, m_lvQuote);
	DDX_Control(pDX, IDC_LIST4, m_lbLog);
	DDX_Control(pDX, IDC_LIST1, m_lvOrderBook);
}

BEGIN_MESSAGE_MAP(CmfcDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_BUTTON1, &CmfcDlg::OnBnClickedButton1)
	ON_BN_CLICKED(IDC_BUTTON2, &CmfcDlg::OnBnClickedButton2)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST2, &CmfcDlg::OnNMDblclkList2)
	ON_BN_CLICKED(IDC_BUTTON3, &CmfcDlg::OnBnClickedButton3)
	ON_BN_CLICKED(IDC_BUTTON4, &CmfcDlg::OnBnClickedButton4)
	ON_BN_CLICKED(IDC_BUTTON5, &CmfcDlg::OnBnClickedButton5)
	ON_BN_CLICKED(IDC_BUTTON6, &CmfcDlg::OnBnClickedButton6)
END_MESSAGE_MAP()

/*
DWORD WINAPI StartWorkProc(void *)
{
	g_worker.start();
	return 0;
}
DWORD WINAPI StopWorkProc(void *)
{
	g_worker.stop();
	return 0;
}
*/

// CmfcDlg 消息处理程序

BOOL CmfcDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码

	g_worker.setDlg(this);
	
	//初始化成交信息ListControl
	m_lvDealInfo.SetExtendedStyle(m_lvDealInfo.GetExtendedStyle() | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	m_lvDealInfo.InsertColumn(0, _T("Exch"), LVCFMT_LEFT, 60);
	m_lvDealInfo.InsertColumn(1, _T("Comm"), LVCFMT_LEFT, 60);
	m_lvDealInfo.InsertColumn(2, _T("Code"), LVCFMT_LEFT, 60);
	m_lvDealInfo.InsertColumn(3, _T("BS"), LVCFMT_LEFT, 60);
	m_lvDealInfo.InsertColumn(4, _T("Price"), LVCFMT_LEFT, 60);
	m_lvDealInfo.InsertColumn(5, _T("Amount"), LVCFMT_LEFT, 60);
	m_lvDealInfo.InsertColumn(6, _T("EntrustType"), LVCFMT_LEFT, 80);
	m_lvDealInfo.InsertColumn(7, _T("Status"), LVCFMT_LEFT, 70);

	//初始化行情信息ListControl
	m_lvQuote.SetExtendedStyle(m_lvQuote.GetExtendedStyle() | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	m_lvQuote.InsertColumn(0, _T("SecurityID"), LVCFMT_LEFT, 80);
	m_lvQuote.InsertColumn(1, _T("MaketStatus"), LVCFMT_LEFT, 80);
	m_lvQuote.InsertColumn(2, _T("Last"), LVCFMT_LEFT, 50);
	m_lvQuote.InsertColumn(3, _T("Open"), LVCFMT_LEFT, 50);
	m_lvQuote.InsertColumn(4, _T("High"), LVCFMT_LEFT, 50);
	m_lvQuote.InsertColumn(5, _T("Close"), LVCFMT_LEFT, 50);

	//初始化买卖盘ListControl
	m_lvOrderBook.SetExtendedStyle(m_lvOrderBook.GetExtendedStyle() | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	m_lvOrderBook.InsertColumn(0, _T("Level"), LVCFMT_LEFT, 40);
	m_lvOrderBook.InsertColumn(1, _T("BuyPrice"), LVCFMT_LEFT, 100);
	m_lvOrderBook.InsertColumn(2, _T("BuyQuantity"), LVCFMT_LEFT, 60);
	m_lvOrderBook.InsertColumn(3, _T("Level"), LVCFMT_LEFT, 40);
	m_lvOrderBook.InsertColumn(4, _T("SellPrice"), LVCFMT_LEFT, 100);
	m_lvOrderBook.InsertColumn(5, _T("SellQuantity"), LVCFMT_LEFT, 60);
	for (int i = 0; i < 10; i++)
	{
		CString s;
		s.Format("%d", i+1);
		m_lvOrderBook.InsertItem(i, s);
		m_lvOrderBook.SetItemText(i, 3, s);
	}

	//test
	
/*
	m_lvQuote.InsertItem(1, "1,0"); 
	m_lvQuote.SetItemText(1, 1, "1,1");
	m_lvQuote.SetItemData(1, 222222);
	
	CString s;
	for (int i = 0; i < 10; i++)
	{
		s.Format("%d", i+100000);
		m_lvQuote.InsertItem(i, s);	 m_lvQuote.SetItemData(i, i*11);
	}

	if ( 55 == m_lvQuote.GetItemData(5) )
	{
		m_lvQuote.InsertItem(10, m_lvQuote.GetItemText(5, 0));//500
	}
	
	
	m_lvQuote.SetItemData(1, 1);
	CString s;
	s.Format("%d", m_lvQuote.GetItemCount());
	m_lvQuote.SetItemText(0, 2, s);

	int i;
	
	LVITEM lvItem;
	for (i = 0; i<m_lvQuote.GetItemCount(); i++)
	{
		memset(&lvItem, 0, sizeof(lvItem));
		lvItem.iItem = i;
		lvItem.iSubItem = 0;
		lvItem.mask = LVIF_TEXT;
		m_lvQuote.GetItem(&lvItem);

		CString s = m_lvQuote.GetItemText(i,0);
		DWORD dw = m_lvQuote.GetItemData(i);
		//if (s.Compare("0,0") == 0)
		if (dw == 1)
		{
			m_lvQuote.SetItemText(i, 3, s);
		}
	}
	*/

	//初始化日志框
	m_lbLog.SetHorizontalExtent(1024);
	
	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CmfcDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CmfcDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CmfcDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CmfcDlg::OnBnClickedButton1()
{
	// TODO: 在此添加控件通知处理程序代码
	CString strExchange, strCommodity, strContractCode;
	CString strBS, strPrice, strQuantity, strType;
	m_cbExchange.GetWindowText(strExchange);
	m_cbCommodity.GetWindowText(strCommodity);
	m_cbContractCode.GetWindowText(strContractCode);
	m_cbBS.GetWindowText(strBS);
	m_editPrice.GetWindowText(strPrice);
	m_editQuantity.GetWindowText(strQuantity);
	if (strExchange.IsEmpty())
	{
		MessageBox(_T("交易所没有赋值"), _T("警告"));
		return;
	}
	if (strCommodity.IsEmpty())
	{
		MessageBox(_T("商品没有赋值"), _T("警告"));
		return;
	}
	if (strContractCode.IsEmpty())
	{
		MessageBox(_T("合约代码没有赋值"), _T("警告"));
		return;
	}

	m_lvDealInfo.InsertItem(0, strExchange);
	m_lvDealInfo.SetItemText(0, 1, strCommodity);
	m_lvDealInfo.SetItemText(0, 2, strContractCode);
}

void CmfcDlg::OnBnClickedButton2()
{
	// TODO: 在此添加控件通知处理程序代码
	m_cbExchange.SetWindowText(_T(""));
	m_cbCommodity.SetWindowText(_T(""));
	m_cbContractCode.SetWindowText(_T(""));
	m_cbBS.SetWindowText(_T(""));
	m_cbEntrustType.SetWindowText(_T(""));
	m_editPrice.SetWindowText(_T(""));
	m_editQuantity.SetWindowText(_T(""));
}

void CmfcDlg::OnNMDblclkList2(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;

 	int nIndex = pNMItemActivate->iItem;
 	if (-1 != nIndex)
 	{
 		//m_lvOrderBook.SetItemText(0, 0, "set");
		m_securityID = (int )m_lvQuote.GetItemData(nIndex);
		g_worker.updateOrderBook(m_securityID);
 	}
}

void CmfcDlg::OnBnClickedButton3()
{
	// TODO: 在此添加控件通知处理程序代码
	AfxBeginThread(StartTrade, NULL);
}

void CmfcDlg::OnBnClickedButton4()
{
	// TODO: 在此添加控件通知处理程序代码
	AfxBeginThread(StopTrade, NULL);
}

void CmfcDlg::OnBnClickedButton5()
{
	// TODO: 在此添加控件通知处理程序代码
	//AfxBeginThread(StartQuote, NULL);
	g_worker.startQuote();
}

void CmfcDlg::OnBnClickedButton6()
{
	// TODO: 在此添加控件通知处理程序代码
	//AfxBeginThread(StopQuote, NULL);
	g_worker.stopQuote();
}
