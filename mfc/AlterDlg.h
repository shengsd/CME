#pragma once


// CAlterDlg �Ի���

class CAlterDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CAlterDlg)

public:
	CAlterDlg(ORDER& order, CWnd* pParent = NULL);   // ��׼���캯��
	virtual ~CAlterDlg();

// �Ի�������
	enum { IDD = IDD_ALTER_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

	DECLARE_MESSAGE_MAP()
public:
	CString m_csOrigSecurityDesc;
	CString m_csOrigOrderQty;
	CString m_csOrigPrice;
	CString m_csOrigStopPx;
	afx_msg void OnBnClickedCancelOrder();

	//����/��������Ϣ��Ҫ����
	ORDER& m_order;
	afx_msg void OnBnClickedReplaceOrder();
};
