// mfcDlg.h : ͷ�ļ�
//

#pragma once
#include "afxwin.h"
#include "afxcmn.h"
#include "resource.h"


// CmfcDlg �Ի���
class CmfcDlg : public CDialog
{
// ����
public:
	CmfcDlg(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
	enum { IDD = IDD_MFC_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��


// ʵ��
protected:
	HICON m_hIcon;

	// ���ɵ���Ϣӳ�亯��
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
	// ��������
	CComboBox m_cbBS;
	afx_msg void OnBnClickedButton2();
	// ί������ 0:ί�� 1:���� 4:���� D:�ĵ� G:ѯ��
	CComboBox m_cbEntrustType;
	// ί�м۸�
	CEdit m_editPrice;
	// ί������
	CEdit m_editQuantity;
	// ������Ϣ
	CListCtrl m_lvQuote;
	// ��־����
	CListBox m_lbLog;
	// //������
	int m_securityID;
	CListCtrl m_lvOrderBook;
	afx_msg void OnNMDblclkList2(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedButton3();

	afx_msg void OnBnClickedButton4();
	afx_msg void OnBnClickedButton5();
	afx_msg void OnBnClickedButton6();
};
