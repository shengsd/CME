// mfcDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "cme.h"
#include "cmeDlg.h"
#include "AlterDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//辅助线程的控制函数
//交易开关
UINT Trade( LPVOID pParam )
{
	static BOOL bTrade= FALSE;
	CcmeDlg* pDlg = (CcmeDlg* )pParam;
	pDlg->GetDlgItem(IDC_BUTTON3)->EnableWindow(FALSE);
	if (bTrade)//stop
	{
		g_worker.StopTrade();
		bTrade = FALSE;
		pDlg->GetDlgItem(IDC_BUTTON3)->SetWindowTextA(_T("Trade ON"));
		pDlg->GetDlgItem(IDC_BUTTON3)->EnableWindow(TRUE);
		return 0;
	}
	else//start
	{
		if (g_worker.StartTrade())
		{
			pDlg->GetDlgItem(IDC_BUTTON3)->SetWindowTextA(_T("Trade OFF"));
			pDlg->GetDlgItem(IDC_BUTTON3)->EnableWindow(TRUE);
			bTrade = TRUE;
			return 0;
		}
		else
		{
			pDlg->GetDlgItem(IDC_BUTTON3)->EnableWindow(TRUE);
			return 1;
		}
	}
}
//行情开关
UINT Quote( LPVOID pParam )
{
	static BOOL bQuote= FALSE;
	CcmeDlg* pDlg = (CcmeDlg* )pParam;
	pDlg->GetDlgItem(IDC_BUTTON5)->EnableWindow(FALSE);
	if (bQuote)//stop
	{
		if (!g_worker.stopQuote())
		{
			pDlg->GetDlgItem(IDC_BUTTON5)->SetWindowTextA(_T("Quote ON"));
			pDlg->GetDlgItem(IDC_BUTTON5)->EnableWindow(TRUE);
			bQuote = FALSE;
			return 0;
		}
		else
		{
			pDlg->GetDlgItem(IDC_BUTTON5)->EnableWindow(TRUE);
			return -1;
		}
	}
	else//start
	{
		if (!g_worker.startQuote())
		{
			pDlg->GetDlgItem(IDC_BUTTON5)->SetWindowTextA(_T("Quote OFF"));
			pDlg->GetDlgItem(IDC_BUTTON5)->EnableWindow(TRUE);
			bQuote = TRUE;
			return 0;
		}
		else
		{
			pDlg->GetDlgItem(IDC_BUTTON5)->EnableWindow(TRUE);
			return -1;
		}
	}
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

CcmeDlg::CcmeDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CcmeDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_OrderBookSecurityID = 0;
}

void CcmeDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST3, m_lvOrderInfoList);
	DDX_Control(pDX, IDC_COMBO1, m_SecurityDesc);
	DDX_Control(pDX, IDC_COMBO2, m_Symbol);
	DDX_Control(pDX, IDC_COMBO3, m_SecurityType);
	DDX_Control(pDX, IDC_COMBO4, m_Side);
	DDX_Control(pDX, IDC_COMBO5, m_OrdType);
	DDX_Control(pDX, IDC_COMBO6, m_TimeInForce);
	DDX_Control(pDX, IDC_EDIT1, m_Price);
	DDX_Control(pDX, IDC_EDIT2, m_StopPx);
	DDX_Control(pDX, IDC_LIST2, m_lvMktDtInfoList);
	DDX_Control(pDX, IDC_LIST4, m_lbLogList);
	DDX_Control(pDX, IDC_LIST1, m_lvOrderBookList);
	DDX_Control(pDX, IDC_EDIT3, m_OrderQty);
	DDX_Control(pDX, IDC_EDIT4, m_MaxShow);
	DDX_Control(pDX, IDC_DATETIMEPICKER1, m_ExpireDate);
	DDX_Control(pDX, IDC_EDIT5, m_MinQty);
}

BEGIN_MESSAGE_MAP(CcmeDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_BUTTON_ENTER, &CcmeDlg::OnBnClickedEnter)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST2, &CcmeDlg::OnNMDblclkList2)
	ON_BN_CLICKED(IDC_BUTTON3, &CcmeDlg::OnBnClickedTrade)
	ON_BN_CLICKED(IDC_BUTTON5, &CcmeDlg::OnBnClickedQuote)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST3, &CcmeDlg::OnNMDblclkListOrderInfo)
	ON_BN_CLICKED(IDC_BUTTON_CLEAR, &CcmeDlg::OnBnClickedRFQ)
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

BOOL CcmeDlg::OnInitDialog()
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
	SetIcon(m_hIcon, FALSE);			// 设置小图标

	// TODO: 在此添加额外的初始化代码

	g_worker.setDlg(this);
	
	//初始化订单信息ListControl
	m_lvOrderInfoList.SetExtendedStyle(m_lvOrderInfoList.GetExtendedStyle() | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	m_lvOrderInfoList.InsertColumn(OrderInfo_Column_ClOrderID, _T("ClOrderID"), LVCFMT_LEFT, 72);//本地单号，每条订单（委托指令）对应一个本地单号
	m_lvOrderInfoList.InsertColumn(OrderInfo_Column_OrigClOrdID, _T("OrigClOrdID"), LVCFMT_LEFT, 80);//订单链的上一个本地单号，改单，撤单对应的订单的本地单号
	m_lvOrderInfoList.InsertColumn(OrderInfo_Column_CorrelationClOrdID, _T("CorrelationClOrdID"), LVCFMT_LEFT, 90);//订单链最开始的本地单号，改单，撤单对应的下单的本地单号
	m_lvOrderInfoList.InsertColumn(OrderInfo_Column_Status, _T("Status"), LVCFMT_LEFT, 56);//订单状态
	m_lvOrderInfoList.InsertColumn(OrderInfo_Column_OrderID, _T("OrderID"), LVCFMT_LEFT, 72);//主场单号
	m_lvOrderInfoList.InsertColumn(OrderInfo_Column_SecurityDesc, _T("SecurityDesc"), LVCFMT_LEFT, 88);//合约名
	m_lvOrderInfoList.InsertColumn(OrderInfo_Column_Side, _T("Side"), LVCFMT_LEFT, 40);//买卖方向
	m_lvOrderInfoList.InsertColumn(OrderInfo_Column_OrderQty, _T("OrderQty"), LVCFMT_LEFT, 64);//订单数量
	m_lvOrderInfoList.InsertColumn(OrderInfo_Column_MinQty, _T("MinQty"), LVCFMT_LEFT, 56);//最少成交量
	m_lvOrderInfoList.InsertColumn(OrderInfo_Column_MaxShow, _T("MaxShow"), LVCFMT_LEFT, 64);//买卖盘最大显示数量
	m_lvOrderInfoList.InsertColumn(OrderInfo_Column_OrderType, _T("OrderType"), LVCFMT_LEFT, 72);//订单类型
	m_lvOrderInfoList.InsertColumn(OrderInfo_Column_Price, _T("Price"), LVCFMT_LEFT, 56);//价格
	m_lvOrderInfoList.InsertColumn(OrderInfo_Column_StopPx, _T("StopPx"), LVCFMT_LEFT, 56);//止损价
	m_lvOrderInfoList.InsertColumn(OrderInfo_Column_TimeInForce, _T("TimeInForce"), LVCFMT_LEFT, 80);//有效期
	m_lvOrderInfoList.InsertColumn(OrderInfo_Column_ExpireDate, _T("ExpireDate"), LVCFMT_LEFT, 72);//指定到期日
	//m_lvOrderInfoList.InsertColumn(OrderInfo_Column_Symbol, _T("Symbol"), LVCFMT_LEFT, 72);

	/*/Test
	m_lvOrderInfoList.InsertItem(0, _T("3"));//Column_ClOrderID
	m_lvOrderInfoList.SetItemText(0, OrderInfo_Column_OrigClOrdID, _T("2"));
	m_lvOrderInfoList.SetItemText(0, OrderInfo_Column_CorrelationClOrdID, _T("1"));
	m_lvOrderInfoList.SetItemText(0, OrderInfo_Column_Status, _T("New"));
	m_lvOrderInfoList.SetItemText(0, OrderInfo_Column_OrderID, _T("OrderId"));
	m_lvOrderInfoList.SetItemText(0, OrderInfo_Column_OrderQty, _T("OrderQty"));
	m_lvOrderInfoList.SetItemText(0, OrderInfo_Column_Price, _T("Price"));
	m_lvOrderInfoList.SetItemText(0, OrderInfo_Column_StopPx, _T("StopPx"));
	/*/

	//初始化行情信息ListControl
	m_lvMktDtInfoList.SetExtendedStyle(m_lvMktDtInfoList.GetExtendedStyle() | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	m_lvMktDtInfoList.InsertColumn(MktDtInfo_Column_Symbol, _T("Symbol"), LVCFMT_LEFT, 80);
	m_lvMktDtInfoList.InsertColumn(MktDtInfo_Column_Status, _T("Status"), LVCFMT_LEFT, 80);
	m_lvMktDtInfoList.InsertColumn(MktDtInfo_Column_SecurityID, _T("SecurityID"), LVCFMT_LEFT, 80);
	m_lvMktDtInfoList.InsertColumn(MktDtInfo_Column_Asset, _T("Asset"), LVCFMT_LEFT, 50);
	m_lvMktDtInfoList.InsertColumn(MktDtInfo_Column_SecurityType, _T("SecurityType"), LVCFMT_LEFT, 80);
	m_lvMktDtInfoList.InsertColumn(MktDtInfo_Column_Last, _T("Last"), LVCFMT_LEFT, 50);
	m_lvMktDtInfoList.InsertColumn(MktDtInfo_Column_Open, _T("Open"), LVCFMT_LEFT, 50);
	m_lvMktDtInfoList.InsertColumn(MktDtInfo_Column_High, _T("High"), LVCFMT_LEFT, 50);
	m_lvMktDtInfoList.InsertColumn(MktDtInfo_Column_Low, _T("Low"), LVCFMT_LEFT, 50);


	//初始化买卖盘ListControl
	m_lvOrderBookList.SetExtendedStyle(m_lvOrderBookList.GetExtendedStyle() | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	m_lvOrderBookList.InsertColumn(OrderBook_Column_BuyLevel, _T("Level"), LVCFMT_LEFT, 40);
	m_lvOrderBookList.InsertColumn(OrderBook_Column_BuyPrice, _T("BuyPrice"), LVCFMT_LEFT, 100);
	m_lvOrderBookList.InsertColumn(OrderBook_Column_BuyQuantity, _T("BuyQuantity"), LVCFMT_LEFT, 60);
	m_lvOrderBookList.InsertColumn(OrderBook_Column_SellLevel, _T("Level"), LVCFMT_LEFT, 40);
	m_lvOrderBookList.InsertColumn(OrderBook_Column_SellPrice, _T("SellPrice"), LVCFMT_LEFT, 100);
	m_lvOrderBookList.InsertColumn(OrderBook_Column_SellQuantity, _T("SellQuantity"), LVCFMT_LEFT, 60);
	for (int i = 0; i < 10; i++)
	{
		CString s;
		s.Format("%d", i+1);
		m_lvOrderBookList.InsertItem(i, s);//OrderBook_Column_BuyLevel
		m_lvOrderBookList.SetItemText(i, OrderBook_Column_SellLevel, s);
	}

	//初始化日志框
	m_lbLogList.SetHorizontalExtent(1024);
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
	
	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CcmeDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CcmeDlg::OnPaint()
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
HCURSOR CcmeDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CcmeDlg::OnBnClickedEnter()
{
	// TODO: 在此添加控件通知处理程序代码
	ORDER order = {0};

	order.OrdType[0] = _T('D');
	CString csSecurityDesc;
	 m_SecurityDesc.GetWindowText(csSecurityDesc);
	if (csSecurityDesc.IsEmpty())
	{
		MessageBox(_T("SecurityDesc"), _T("Warning"));
		return;
	}
	_tcscpy_s(order.SecurityDesc, _countof(order.SecurityDesc), csSecurityDesc);
	
	CString csSymbol;
	m_Symbol.GetWindowText(csSymbol);
	if (csSymbol.IsEmpty())
	{
		MessageBox(_T("Symbol"), _T("Warning"));
		return;
	}
	_tcscpy_s(order.Symbol, _countof(order.Symbol), csSymbol);

	CString csSecurityType;
	m_SecurityType.GetWindowText(csSecurityType);
	if (csSecurityType.IsEmpty())
	{
		MessageBox(_T("SecurityType"), _T("Warning"));
		return;
	}
	_tcscpy_s(order.SecurityType, _countof(order.SecurityType), csSecurityType);

	CString csSide;
	m_Side.GetWindowText(csSide);
	if (csSide.IsEmpty())
	{
		MessageBox(_T("Side"), _T("Warning"));
		return;
	}
	order.Side[0] = csSide.GetAt(0);

	CString csOrderQty;
	m_OrderQty.GetWindowText(csOrderQty);
	if (csOrderQty.IsEmpty())
	{
		MessageBox(_T("OrderQty"), _T("Warning"));
		return;
	}
	_tcscpy_s(order.OrderQty, _countof(order.OrderQty), csOrderQty);

	CString csOrdType;
	m_OrdType.GetWindowText(csOrdType);
	if (csOrdType.IsEmpty())
	{
		MessageBox(_T("OrderType"), _T("Warning"));
		return;
	}
	order.OrdType[0] = csOrdType.GetAt(0);

	switch (order.OrdType[0])
	{
	case _T('1')://Market order (with protection) 
		break;
	case _T('2')://Limit order 
		{
			CString csPrice;
			m_Price.GetWindowText(csPrice);
			if (csPrice.IsEmpty())
			{
				MessageBox(_T("price"), _T("Warning"));
				return;
			}
			_tcscpy_s(order.Price, _countof(order.Price), csPrice);
			break;
		}
	case _T('3')://Stop order (with protection) 
		{
			CString csStopPx;
			m_StopPx.GetWindowText(csStopPx);
			if (csStopPx.IsEmpty())
			{
				MessageBox(_T("StopPx"), _T("Warning"));
				return;
			}
			_tcscpy_s(order.StopPx, _countof(order.StopPx), csStopPx);
			break;
		}
	case _T('4')://Stop-Limit order
		{
			CString csPrice;
			m_Price.GetWindowText(csPrice);
			if (csPrice.IsEmpty())
			{
				MessageBox(_T("Price"), _T("Warning"));
				return;
			}
			_tcscpy_s(order.Price, _countof(order.Price), csPrice);
			CString csStopPx;
			m_StopPx.GetWindowText(csStopPx);
			if (csStopPx.IsEmpty())
			{
				MessageBox(_T("StopPx"), _T("Warning"));
				return;
			}
			_tcscpy_s(order.StopPx, _countof(order.StopPx), csStopPx);
			break;
		}
	case _T('K')://Market-Limit order
		break;
	default:
		break;
	}

	//非必要字段，约定0表示不发送
	CString csMinQty;
	m_MinQty.GetWindowText(csMinQty);
	if (csMinQty.IsEmpty())//前台为空，就表示不发送该字段
	{
		order.MinQty[0] = '0';
	}
	_tcscpy_s(order.MinQty, _countof(order.MinQty), csMinQty);

	//非必要字段，约定0表示不发送
	CString csMaxShow;
	m_MaxShow.GetWindowText(csMaxShow);
	if (csMaxShow.IsEmpty())//前台为空，就表示不发送该字段
	{
		order.MaxShow[0] = '0';
	}
	_tcscpy_s(order.MaxShow, _countof(order.MaxShow), csMaxShow);

	//非必要字段，第一个字符为'-'表示不发送
	CString csTimeInforce;
	m_TimeInForce.GetWindowText(csTimeInforce);
	order.TimeInForce[0] = csTimeInforce.GetAt(0);
	switch (order.TimeInForce[0])
	{
	case _T('0')://Day
		break;
	case _T('1')://Good Till Cancel (GTC)
		break;
	case _T('3')://Fill and Kill
		break;
	case _T('6')://Good Till Date
		{
			SYSTEMTIME st;
			DWORD dwResult = m_ExpireDate.GetTime(&st);
			if (dwResult == GDT_VALID)
			{
				CString csExpireDate;
				csExpireDate.Format(_T("%04d%02d%02d"), st.wYear, st.wMonth, st.wDay);
				_tcscpy_s(order.ExpireDate, _countof(order.ExpireDate), csExpireDate);
			}
			else
			{
				MessageBox(_T("ExpireDate"), _T("Warning"));
				return;
			}
			break;
		}
	default:
		break;
	}

	g_worker.EnterOrder(order);
}


void CcmeDlg::OnNMDblclkList2(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;

 	int nIndex = pNMItemActivate->iItem;
 	if (-1 != nIndex)
 	{
 		//m_lvOrderBook.SetItemText(0, 0, "set");
		m_OrderBookSecurityID = (int )m_lvMktDtInfoList.GetItemData(nIndex);
		g_worker.updateOrderBook(m_OrderBookSecurityID);
 	}
}

void CcmeDlg::OnBnClickedTrade()
{
	// TODO: 在此添加控件通知处理程序代码
	AfxBeginThread(Trade, this);
}

void CcmeDlg::OnBnClickedQuote()
{
	// TODO: 在此添加控件通知处理程序代码
	//AfxBeginThread(Quote, this);
	static BOOL bQuote= FALSE;
	this->GetDlgItem(IDC_BUTTON5)->EnableWindow(FALSE);
	if (bQuote)//stop
	{
		if (!g_worker.stopQuote())
		{
			this->GetDlgItem(IDC_BUTTON5)->SetWindowTextA(_T("Quote ON"));
			this->GetDlgItem(IDC_BUTTON5)->EnableWindow(TRUE);
			bQuote = FALSE;
		}
		else
		{
			this->GetDlgItem(IDC_BUTTON5)->EnableWindow(TRUE);
		}
	}
	else//start
	{
		if (!g_worker.startQuote())
		{
			this->GetDlgItem(IDC_BUTTON5)->SetWindowTextA(_T("Quote OFF"));
			this->GetDlgItem(IDC_BUTTON5)->EnableWindow(TRUE);
			bQuote = TRUE;
		}
		else
		{
			this->GetDlgItem(IDC_BUTTON5)->EnableWindow(TRUE);
		}
	}
}


void CcmeDlg::OnNMDblclkListOrderInfo(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;

	//点击了第iItem行
	int iItem = pNMItemActivate->iItem;
	if (iItem == -1)
		return;

	ORDER order = {0};

	if (FALSE == g_worker.GetOrderByClOrderID(m_lvOrderInfoList.GetItemText(iItem, 0), order))
	{
		MessageBox( _T("Can't find the order!"), _T("Error"), MB_ICONERROR | MB_OK );
		return;
	}
	//改、撤单不限制
	/*
	//已报或已改状态下 可以改、撤单
	if ( _T('0') != order.OrdStatus[0] && _T('0') != order.OrdStatus[5] )
	{
		MessageBox( _T("Can't cancel or repalce this order, wrong OrdStatus!"), _T("Error"), MB_ICONERROR | MB_OK );
		return;
	}
	*/
	CAlterDlg alterDlg(order);

	//待改/撤参数显示到窗口
	alterDlg.m_csOrigSecurityDesc = m_lvOrderInfoList.GetItemText(iItem, OrderInfo_Column_SecurityDesc);
	alterDlg.m_csOrigOrderQty = m_lvOrderInfoList.GetItemText(iItem, OrderInfo_Column_OrderQty);
	alterDlg.m_csOrigPrice = m_lvOrderInfoList.GetItemText(iItem, OrderInfo_Column_Price);
	alterDlg.m_csOrigStopPx = m_lvOrderInfoList.GetItemText(iItem, OrderInfo_Column_StopPx);

	//订单参数传给窗口
	INT_PTR nRet = alterDlg.DoModal();
	switch (nRet)
	{
	case -1: 
		AfxMessageBox(_T("Dialog box could not be created!"));
		break;
	case IDABORT:
		// Do something
		break;
	case IDOK:
		// Do something
		break;
	case IDCANCEL:
		// Do something
		break;
	default:
		// Do something
		break;
	};
}

//询价
void CcmeDlg::OnBnClickedRFQ()
{
	// TODO: 在此添加控件通知处理程序代码
	ORDER order = {0};

	CString csSymbol;
	m_Symbol.GetWindowText(csSymbol);
	if (csSymbol.IsEmpty())
	{
		MessageBox(_T("Symbol"), _T("Warning"));
		return;
	}
	_tcscpy_s(order.Symbol, _countof(order.Symbol), csSymbol);

	CString csOrderQty;
	m_OrderQty.GetWindowText(csOrderQty);
	_tcscpy_s(order.OrderQty, _countof(order.OrderQty), csOrderQty);

	CString csSide;
	m_Side.GetWindowText(csSide);
	order.Side[0] = csSide.GetAt(0);

	CString csSecurityDesc;
	m_SecurityDesc.GetWindowText(csSecurityDesc);
	if (csSecurityDesc.IsEmpty())
	{
		MessageBox(_T("SecurityDesc"), _T("Warning"));
		return;
	}
	_tcscpy_s(order.SecurityDesc, _countof(order.SecurityDesc), csSecurityDesc);

	CString csSecurityType;
	m_SecurityType.GetWindowText(csSecurityType);
	if (csSecurityType.IsEmpty())
	{
		MessageBox(_T("SecurityType"), _T("Warning"));
		return;
	}
	_tcscpy_s(order.SecurityType, _countof(order.SecurityType), csSecurityType);

	g_worker.RequestForQuote(order);
}
