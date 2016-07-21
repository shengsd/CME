// AlterDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "mfc.h"
#include "AlterDlg.h"
#include "afxdialogex.h"


// CAlterDlg 对话框

IMPLEMENT_DYNAMIC(CAlterDlg, CDialogEx)

CAlterDlg::CAlterDlg(ORDER& order, CWnd* pParent /*=NULL*/)
	: CDialogEx(CAlterDlg::IDD, pParent)
	, m_order(order)
	, m_csOrigSecurityDesc(_T(""))
	, m_csOrigOrderQty(_T(""))
	, m_csOrigPrice(_T(""))
	, m_csOrigStopPx(_T(""))
{
}

CAlterDlg::~CAlterDlg()
{
}

void CAlterDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT_SecurityDesc, m_csOrigSecurityDesc);
	DDX_Text(pDX, IDC_EDIT_Orig_OrderQty, m_csOrigOrderQty);
	DDX_Text(pDX, IDC_EDIT_Orig_Price, m_csOrigPrice);
	DDX_Text(pDX, IDC_EDIT_Orig_StopPrice, m_csOrigStopPx);
}


BEGIN_MESSAGE_MAP(CAlterDlg, CDialogEx)
	ON_BN_CLICKED(IDCANCEL, &CAlterDlg::OnBnClickedCancelOrder)
	ON_BN_CLICKED(IDOK, &CAlterDlg::OnBnClickedReplaceOrder)
END_MESSAGE_MAP()


// CAlterDlg 消息处理程序


void CAlterDlg::OnBnClickedCancelOrder()
{
	g_worker.CancelOrder(m_order);
	CDialogEx::OnCancel();
}


void CAlterDlg::OnBnClickedReplaceOrder()
{
	strcpy_s(m_order.OrderQty, m_csOrigOrderQty);
	strcpy_s(m_order.Price, m_csOrigPrice);
	strcpy_s(m_order.StopPx, m_csOrigStopPx);
	g_worker.ReplaceOrder(m_order);	
	CDialogEx::OnOK();
}
