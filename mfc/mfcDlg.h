// mfcDlg.h : 头文件
//

#pragma once
#include "afxwin.h"
#include "afxcmn.h"
#include "resource.h"
#include "afxdtctl.h"

//订单信息列表表头
#define OrderInfo_Column_ClOrderID 0
#define OrderInfo_Column_OrigClOrdID 1
#define OrderInfo_Column_CorrelationClOrdID 2
#define OrderInfo_Column_Status 3
#define OrderInfo_Column_OrderID 4
#define OrderInfo_Column_SecurityDesc 5
#define OrderInfo_Column_Side 6
#define OrderInfo_Column_OrderQty 7
#define OrderInfo_Column_MinQty 8
#define OrderInfo_Column_MaxShow 9
#define OrderInfo_Column_OrderType 10
#define OrderInfo_Column_Price 11
#define OrderInfo_Column_StopPx 12
#define OrderInfo_Column_TimeInForce 13
#define OrderInfo_Column_ExpireDate 14
#define OrderInfo_Column_Symbol 15

//行情信息列表表头
#define MktDtInfo_Column_Symbol 0
#define MktDtInfo_Column_Status 1
#define MktDtInfo_Column_SecurityID 2
#define MktDtInfo_Column_Asset 3
#define MktDtInfo_Column_SecurityType 4
#define MktDtInfo_Column_Last 5
#define MktDtInfo_Column_Open 6
#define MktDtInfo_Column_High 7
#define MktDtInfo_Column_Low 8

#define OrderBook_Column_BuyLevel 0
#define OrderBook_Column_BuyPrice 1
#define OrderBook_Column_BuyQuantity 2
#define OrderBook_Column_SellLevel 3
#define OrderBook_Column_SellPrice 4
#define OrderBook_Column_SellQuantity 5
// CmfcDlg 对话框
class CmfcDlg : public CDialog
{
// 构造
public:
	CmfcDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_CME_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedEnter();
	afx_msg void OnNMDblclkList2(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedButton3();
	afx_msg void OnBnClickedButton5();

	//双击订单信息界面，用于改单、撤单
	afx_msg void OnNMDblclkListOrderInfo(NMHDR *pNMHDR, LRESULT *pResult);

	// 订单信息
	CListCtrl m_lvOrderInfoList;
	// 行情信息
	CListCtrl m_lvMktDtInfoList;
	// 日志容器
	CListBox m_lbLogList;
	// 买卖盘
	CListCtrl m_lvOrderBookList;
	// 买卖盘当前显示的合约ID
	int m_OrderBookSecurityID;

	// 交易合约主键 对应MDP3.0的Symbol
	CComboBox m_SecurityDesc;
	// 对应MDP3.0的SecurityGroup
	CComboBox m_Symbol;
	// 合约类型
	CComboBox m_SecurityType;
	// 买卖方向
	CComboBox m_Side;
	// 委托类型 0:委托 1:撤单 4:补单 D:改单 G:询价
	CComboBox m_OrdType;
	// 委托价格
	CEdit m_Price;
	// 止损价
	CEdit m_StopPx;
	// 有效期类型
	CComboBox m_TimeInForce;
	// 委托数量
	CEdit m_OrderQty;
	// 最大显示数量
	CEdit m_MaxShow;
	//订单到期日
	CDateTimeCtrl m_ExpireDate;
	//订单最少成交量
	CEdit m_MinQty;
	afx_msg void OnBnClickedQuote();
};
