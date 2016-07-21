#pragma once


// CAlterDlg 对话框

class CAlterDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CAlterDlg)

public:
	CAlterDlg(ORDER& order, CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CAlterDlg();

// 对话框数据
	enum { IDD = IDD_ALTER_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	CString m_csOrigSecurityDesc;
	CString m_csOrigOrderQty;
	CString m_csOrigPrice;
	CString m_csOrigStopPx;
	afx_msg void OnBnClickedCancelOrder();

	//被改/撤订单信息需要传入
	ORDER& m_order;
	afx_msg void OnBnClickedReplaceOrder();
};
