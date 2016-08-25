// mfcDlg.h : ͷ�ļ�
//

#pragma once
#include "afxwin.h"
#include "afxcmn.h"
#include "resource.h"
#include "afxdtctl.h"

//������Ϣ�б��ͷ
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

//������Ϣ�б��ͷ
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
// CmfcDlg �Ի���
class CmfcDlg : public CDialog
{
// ����
public:
	CmfcDlg(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
	enum { IDD = IDD_CME_DIALOG };

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
	afx_msg void OnBnClickedEnter();
	afx_msg void OnNMDblclkList2(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedButton3();
	afx_msg void OnBnClickedButton5();

	//˫��������Ϣ���棬���ڸĵ�������
	afx_msg void OnNMDblclkListOrderInfo(NMHDR *pNMHDR, LRESULT *pResult);

	// ������Ϣ
	CListCtrl m_lvOrderInfoList;
	// ������Ϣ
	CListCtrl m_lvMktDtInfoList;
	// ��־����
	CListBox m_lbLogList;
	// ������
	CListCtrl m_lvOrderBookList;
	// �����̵�ǰ��ʾ�ĺ�ԼID
	int m_OrderBookSecurityID;

	// ���׺�Լ���� ��ӦMDP3.0��Symbol
	CComboBox m_SecurityDesc;
	// ��ӦMDP3.0��SecurityGroup
	CComboBox m_Symbol;
	// ��Լ����
	CComboBox m_SecurityType;
	// ��������
	CComboBox m_Side;
	// ί������ 0:ί�� 1:���� 4:���� D:�ĵ� G:ѯ��
	CComboBox m_OrdType;
	// ί�м۸�
	CEdit m_Price;
	// ֹ���
	CEdit m_StopPx;
	// ��Ч������
	CComboBox m_TimeInForce;
	// ί������
	CEdit m_OrderQty;
	// �����ʾ����
	CEdit m_MaxShow;
	//����������
	CDateTimeCtrl m_ExpireDate;
	//�������ٳɽ���
	CEdit m_MinQty;
	afx_msg void OnBnClickedQuote();
};
