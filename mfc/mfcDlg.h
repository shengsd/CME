// mfcDlg.h : 头文件
//

#pragma once
#include "afxwin.h"
#include "afxcmn.h"
#include "resource.h"


// CmfcDlg 对话框
class CmfcDlg : public CDialog
{
// 构造
public:
	CmfcDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_MFC_DIALOG };

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
	CListCtrl m_lvDealInfo;
	CComboBox m_cbExchange;
	CComboBox m_cbCommodity;
	CComboBox m_cbContractCode;
	afx_msg void OnBnClickedButton1();
	// 买卖方向
	CComboBox m_cbBS;
	afx_msg void OnBnClickedButton2();
	// 委托类型 0:委托 1:撤单 4:补单 D:改单 G:询价
	CComboBox m_cbEntrustType;
	// 委托价格
	CEdit m_editPrice;
	// 委托数量
	CEdit m_editQuantity;
	// 行情信息
	CListCtrl m_lvQuote;
	// 日志容器
	CListBox m_lbLog;
	// //买卖盘
	int m_securityID;
	CListCtrl m_lvOrderBook;
	afx_msg void OnNMDblclkList2(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedButton3();

	afx_msg void OnBnClickedButton4();
	afx_msg void OnBnClickedButton5();
	afx_msg void OnBnClickedButton6();
};
