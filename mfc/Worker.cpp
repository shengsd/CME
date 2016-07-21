#include "StdAfx.h"
#include "Worker.h"
#include "mfc.h"

Worker::Worker(void)
{
	m_pTradeSession = NULL;
	m_hEventReadyToTrade = CreateEvent(NULL, TRUE, FALSE, NULL);
	InitializeCriticalSection(&m_csOrder);
}

Worker::~Worker(void)
{
	if (m_hEventReadyToTrade)
		CloseHandle(m_hEventReadyToTrade);
	DeleteCriticalSection(&m_csOrder);
}

void Worker::WriteLog(int nLevel, const TCHAR *szFormat, ...)
{
	static const TCHAR *szLevel[] = {"INFO", "DEBUG", "WARNING", "ERROR", "FATAL"};

	va_list va;
	va_start (va, szFormat);

	SYSTEMTIME st;
	GetLocalTime(&st);
	LARGE_INTEGER PerformanceCount;
	LARGE_INTEGER Frequency;
	QueryPerformanceFrequency(&Frequency);
	QueryPerformanceCounter(&PerformanceCount);
	PerformanceCount.QuadPart *= 1000 * 1000;

	TCHAR msg[256] = {0};
	vsprintf(msg, szFormat, va);

	CString str;
	str.Format("%04d%02d%02d %02d:%02d:%02d:%03d[%I64d] - %s: %s", st.wYear, 
		st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds,
		PerformanceCount.QuadPart / Frequency.QuadPart, szLevel[nLevel], msg);
	//	str.Format("%s:%s", szLevel[nLevel], msg);

	m_pDlg->m_lbLogList.AddString(str);
	m_pDlg->m_lbLogList.SetCurSel(m_pDlg->m_lbLogList.GetCount()-1);
}

BOOL Worker::StartTrade()
{
	char cfgPath[128];
	GetCurrentDirectory(128, cfgPath);
	sprintf_s(cfgPath, "%s\\FIX_CME.ini", cfgPath);
	//启动HSfixEngine
	if(EngnInit((char*)cfgPath, this) != 0)
	{
		//sprintf_s(m_szLastErrorInfo, sizeof(m_szLastErrorInfo), "%s", cfgPath);
		WriteLog(LOG_ERROR, _T("%s"), cfgPath);
		return FALSE;
	}

	DWORD dwWaitResult;
	dwWaitResult = WaitForSingleObject(m_hEventReadyToTrade, 20000);
	if ( dwWaitResult == WAIT_OBJECT_0 )	//登录成功
	{
		WriteLog(LOG_INFO, _T("[Trade]: Start"));
		return TRUE;
	}
	else if ( dwWaitResult == WAIT_TIMEOUT )
	{
		WriteLog(LOG_ERROR, "[Trade]: Connect time out, exiting...");
		//登录超时，关闭引擎
		EngnDone();
		return FALSE;
	}
	return FALSE;
}

void Worker::StopTrade()
{
	EngnDone();
	WriteLog(LOG_INFO, _T("[Trade]: Stop"));
}


int Worker::EnterOrder(ORDER& order)
{
	//TODO:
	//ORDER中可能信息不足，需要从合约列表中获取完整信息

	strcpy_s(order.MsgType, "D");

	IMessage* pMsg = CreateMessage("FIX.4.2", "D");
	HSFixMsgBody* pBody = pMsg->GetMsgBody();

	//TODO:暂时不清楚要放什么字段。Unique account identifier. 
	pBody->SetFieldValue(FIELD::Account, _T("Hundsun UFOs7"));

	SYSTEMTIME st;
	GetSystemTime(&st);
	_stprintf_s(order.ClOrdID, _countof(order.ClOrdID), _T("%02d%02d%02d%03d"), st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
	//本地单号，在这里用HHMMSSsss表示
	pBody->SetFieldValue(FIELD::ClOrdID, order.ClOrdID);

	//原始本地单号
	pBody->SetFieldValue(FIELD::CorrelationClOrdID, order.ClOrdID);

	//Order submitted for automated matching on CME Globex.
	pBody->SetFieldValue(FIELD::HandlInst, "1");

	//CME合约主键
	pBody->SetFieldValue(FIELD::SecurityDesc, order.SecurityDesc);
	pBody->SetFieldValue(FIELD::Symbol, order.Symbol);
	pBody->SetFieldValue(FIELD::SecurityType, order.SecurityType);

	//委托数量
	pBody->SetFieldValue(FIELD::OrderQty, order.OrderQty);

	//买卖方向
	pBody->SetFieldValue(FIELD::Side, order.Side);

	//委托类型
	pBody->SetFieldValue(FIELD::OrdType, order.OrdType);

	switch (order.OrdType[0])
	{
	case _T('1')://Market order (with protection) 
		break;
	case _T('2')://Limit order 
		pBody->SetFieldValue(FIELD::Price, order.Price);
		break;
	case _T('3')://Stop order (with protection) 
		pBody->SetFieldValue(FIELD::StopPx, order.StopPx);
		break;
	case _T('4')://Stop-Limit order
		pBody->SetFieldValue(FIELD::Price, order.Price);
		pBody->SetFieldValue(FIELD::StopPx, order.StopPx);
		break;
	case _T('K')://Market-Limit order
		break;
	default:
		break;
	}
	pBody->SetFieldValue(FIELD::OrderQty, order.OrderQty);

	pBody->SetFieldValue(FIELD::TimeInForce, order.TimeInForce);
	switch (order.TimeInForce[0])
	{
	case _T('0')://Day
		break;
	case _T('1')://Good Till Cancel (GTC)
		break;
	case _T('3')://Fill and Kill
		pBody->SetFieldValue(FIELD::MinQty, order.MinQty);
		break;
	case _T('6')://Good Till Date
		pBody->SetFieldValue(FIELD::ExpireDate, order.ExpireDate);
		break;
	default:
		break;
	}

	//ManualOrderIndicator Y=manual N=automated
	pBody->SetFieldValue(FIELD::ManualOrderIndicator, _T("Y")); 

	//CtiCode TODO: 不是很清楚有什么用
	pBody->SetFieldValue(FIELD::CtiCode, _T("1")); 

	//The type of business conducted. 0=Customer 1=Firm 
	pBody->SetFieldValue(FIELD::CustomerOrFirm, _T("1"));

	//发送消息
	if(m_pTradeSession)
	{
		int iRet = SendMessageByID( pMsg, m_pTradeSession );
		if(iRet != 0)
		{
			//_tcscpy_s( order.ErrorInfo, _countof(order.ErrorInfo), _T("Send Fix Message Fail") );
			AfxMessageBox(_T("Send Fix Message Fail"));
			return STATUS_FAIL;
		}
		DestroyMessage( pMsg );
	}
	else
	{
		//_tcscpy_s( order.ErrorInfo, _countof(order.ErrorInfo), _T("Fix Session not initialized") );
		AfxMessageBox(_T("Fix Session not initialized"));
		return STATUS_FAIL;
	}

	//保存订单
	order.OrdStatus[0] = 'P';//Pending
	AddOrder( order );

	return STATUS_SUCCESS;
}

int Worker::CancelOrder(ORDER& order)
{
	//发撤单消息
	strcpy_s(order.MsgType, "F");

	IMessage* pMsg = CreateMessage("FIX.4.2", "F");
	HSFixMsgBody* pBody = pMsg->GetMsgBody();

	//Unique account identifier. TODO:暂时不清楚
	pBody->SetFieldValue(FIELD::Account, _T("Hundsun UFOs7"));

	SYSTEMTIME st;
	GetSystemTime(&st);
	_stprintf_s(order.ClOrdID, _countof(order.ClOrdID), _T("%02d%02d%02d%03d"), st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
	//新的本地单号
	pBody->SetFieldValue(FIELD::ClOrdID, order.ClOrdID);

	//主场单号
	pBody->SetFieldValue(FIELD::OrderID, order.OrderID);

	//上次本地单号
	strcpy_s(order.OrigClOrdID, order.ClOrdID);
	pBody->SetFieldValue(FIELD::OrigClOrdID, order.OrigClOrdID);

	//买卖方向
	pBody->SetFieldValue(FIELD::Side, order.Side);

	//Symbol
	//pBody->SetFieldValue(FIELD::Symbol, order.Symbol);

	//TransactTime (UTC format YYYYMMDD-HH:MM:SS.sss)
	TCHAR szTransactTime[64];
	_stprintf_s(szTransactTime, _countof(szTransactTime), _T("%d%02d%02d-%02d:%02d:%02d.%03d"), st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
	pBody->SetFieldValue(FIELD::TransactTime, szTransactTime); 

	//ManualOrderIndicator
	pBody->SetFieldValue(FIELD::ManualOrderIndicator, "Y");

	//SecurityDesc
	pBody->SetFieldValue(FIELD::SecurityDesc, order.SecurityDesc);

	//SecurityType
	pBody->SetFieldValue(FIELD::SecurityDesc, order.SecurityType);

	//原始本地单号
	pBody->SetFieldValue(FIELD::CorrelationClOrdID, order.CorrelationClOrdID);

	//发送消息  //即时失败也不改回被改订单
	if(m_pTradeSession)
	{
		int iRet = SendMessageByID(pMsg, m_pTradeSession);
		if(iRet != 0)
		{
			AfxMessageBox(_T("Send Fix Message Fail"));
			return STATUS_FAIL;
		}
		DestroyMessage(pMsg);
	}
	else
	{
		AfxMessageBox(_T("Fix Session not initialized"));
		return STATUS_FAIL;
	}

	//被改订单状态修改
	ORDER oldOrder = {0};
	if (GetOrderByClOrderID(order.OrigClOrdID, oldOrder))
	{
		oldOrder.OrdStatus[0] = '-';
		UpdateOrder(oldOrder);
	}

	//保存新订单
	order.OrdStatus[0] = 'P';
	AddOrder(order);

	return STATUS_SUCCESS;
}

int Worker::ReplaceOrder(ORDER& order)
{
	//发改单消息
	strcpy_s(order.MsgType, "G");

	IMessage* pMsg = CreateMessage("FIX.4.2", "G");
	HSFixMsgBody* pBody = pMsg->GetMsgBody();

	//Unique account identifier. TODO:暂时不清楚
	pBody->SetFieldValue(FIELD::Account, _T("Hundsun UFOs7"));

	SYSTEMTIME st;
	GetSystemTime(&st);
	_stprintf_s(order.ClOrdID, _countof(order.ClOrdID), _T("%02d%02d%02d%03d"), st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
	//新的本地单号
	pBody->SetFieldValue(FIELD::ClOrdID, order.ClOrdID);

	//主场单号
	pBody->SetFieldValue(FIELD::OrderID, order.OrderID);

	//HandInst
	pBody->SetFieldValue(FIELD::HandlInst, "1");
	
	//改单数量
	pBody->SetFieldValue(FIELD::OrderQty, order.OrderQty);

	//CustOrderHandlingInst

	//OrderType
	pBody->SetFieldValue(FIELD::OrdType, order.OrdType);

	//上次本地单号
	strcpy_s(order.OrigClOrdID, order.ClOrdID);
	pBody->SetFieldValue(FIELD::OrigClOrdID, order.OrigClOrdID);

	//买卖方向
	pBody->SetFieldValue(FIELD::Side, order.Side);

	//Symbol
	//pBody->SetFieldValue(FIELD::Symbol, order.Symbol);

	//TransactTime (UTC format YYYYMMDD-HH:MM:SS.sss)
	TCHAR szTransactTime[64];
	_stprintf_s(szTransactTime, _countof(szTransactTime), _T("%d%02d%02d-%02d:%02d:%02d.%03d"), st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
	pBody->SetFieldValue(FIELD::TransactTime, szTransactTime); 

	//ManualOrderIndicator
	pBody->SetFieldValue(FIELD::ManualOrderIndicator, "Y");

	//SecurityDesc
	pBody->SetFieldValue(FIELD::SecurityDesc, order.SecurityDesc);

	//SecurityType
	pBody->SetFieldValue(FIELD::SecurityDesc, order.SecurityType);

	//原始本地单号
	pBody->SetFieldValue(FIELD::CorrelationClOrdID, order.CorrelationClOrdID);

	//发送消息  //即时失败也不改回被改订单
	if(m_pTradeSession)
	{
		int iRet = SendMessageByID(pMsg, m_pTradeSession);
		if(iRet != 0)
		{
			AfxMessageBox(_T("Send Fix Message Fail"));
			return STATUS_FAIL;
		}
		DestroyMessage(pMsg);
	}
	else
	{
		AfxMessageBox(_T("Fix Session not initialized"));
		return STATUS_FAIL;
	}

	//被改订单状态修改
	ORDER oldOrder = {0};
	if (GetOrderByClOrderID(order.OrigClOrdID, oldOrder))
	{
		oldOrder.OrdStatus[0] = '-';
		UpdateOrder(oldOrder);
	}

	//保存新订单
	order.OrdStatus[0] = 'P';
	AddOrder(order);

	return STATUS_SUCCESS;

}


void Worker::ExecReport(const IMessage* pMsg)
{
	ORDER order = {0};
	EXCREPORT excReport = {0};
	HSFixMsgBody* pBody = pMsg->GetMsgBody();

	strcpy_s(excReport.ExecType, pBody->GetFieldValueDefault(FIELD::ExecType, "") );//回报类型
	strcpy_s(excReport.OrdStatus, pBody->GetFieldValueDefault(FIELD::OrdStatus, "") );//订单状态
	strcpy_s(excReport.ClOrdID, pBody->GetFieldValueDefault(FIELD::ClOrdID, "") );
	strcpy_s(excReport.OrigClOrdID, pBody->GetFieldValueDefault(FIELD::OrigClOrdID, "") );
	strcpy_s(excReport.CorrelationClOrdID, pBody->GetFieldValueDefault(FIELD::CorrelationClOrdID, "") );
	strcpy_s(excReport.OrderID, pBody->GetFieldValueDefault(FIELD::OrderID, "") );
	strcpy_s(excReport.SecurityDesc, pBody->GetFieldValueDefault(FIELD::SecurityDesc, "") );
	strcpy_s(excReport.Side, pBody->GetFieldValueDefault(FIELD::Side, "") );
	strcpy_s(excReport.OrderQty, pBody->GetFieldValueDefault(FIELD::OrderQty, "") );
	strcpy_s(excReport.MinQty, pBody->GetFieldValueDefault(FIELD::MinQty, "") );
	strcpy_s(excReport.MaxShow, pBody->GetFieldValueDefault(FIELD::MaxShow, "") );
	strcpy_s(excReport.OrdType, pBody->GetFieldValueDefault(FIELD::OrdType, "") );
	strcpy_s(excReport.Price, pBody->GetFieldValueDefault(FIELD::Price, "") );
	strcpy_s(excReport.StopPx, pBody->GetFieldValueDefault(FIELD::StopPx, "") );
	strcpy_s(excReport.TimeInForce, pBody->GetFieldValueDefault(FIELD::TimeInForce, "") );
	strcpy_s(excReport.ExpireDate, pBody->GetFieldValueDefault(FIELD::ExpireDate, "") );

	//Order Status Request Acknowledgment
	//查询订单反馈
	if ( 'I' == excReport.ExecType[0] )
	{
		ExtractOrderFromExcReport(order, excReport);
		AddOrder(order);
		return ;
	}

	//其它都是订单反馈，成交回报
	//按本地单号找订单
	if ( FALSE == GetOrderByClOrderID(excReport.ClOrdID, order) )
	{
		//找不到订单报错，结束
		WriteLog(LOG_ERROR, _T("Can't find the order, ClOrdID:%s"), excReport.ClOrdID);
		return ;
	}

	//OrderInfoList只更新订单状态
	strcpy_s( order.OrdStatus, excReport.OrdStatus );
	UpdateOrder(order);

	//成交信息通过LogList展示
	switch (excReport.OrdStatus[0])//其它反馈类型可根据订单状态判别
	{
	case '0'://Order Creation
		break;
	case '4'://Order Cancel
		break;
	case '5'://Order Modify
		break;
	case '1'://Partial fill Notice
		WriteLog(LOG_INFO, _T("[ExecReport]: Partial Fill Notice,  ClOrdID=%s, OrderID=%s, LastPx=%s, LastQty=%s"), excReport.ClOrdID, excReport.OrderID, excReport.LastPx, excReport.LastQty);
		break;
	case '2'://Complete fill Notice
		WriteLog(LOG_INFO, _T("[ExecReport]: Complete Fill Notice,  ClOrdID=%s, OrderID=%s, LastPx=%s, LastQty=%s"), excReport.ClOrdID, excReport.OrderID, excReport.LastPx, excReport.LastQty);
		break;
	case 'C'://Order Elimination
		break;
	case '8'://Rejected
		break;
	case 'H'://Trade Cancelled
		break;
	default:
		break;
	}

}


void Worker::AddOrder(ORDER& order)
{
	//保存
	EnterCriticalSection(&m_csOrder);
	m_mapClOrderIDToOrder[order.ClOrdID] = order;
	LeaveCriticalSection(&m_csOrder);

	//界面展示
	m_pDlg->m_lvOrderInfoList.InsertItem(0, order.ClOrdID);

	m_pDlg->m_lvOrderInfoList.SetItemText(0, OrderInfo_Column_OrigClOrdID, order.OrigClOrdID);

	m_pDlg->m_lvOrderInfoList.SetItemText(0, OrderInfo_Column_CorrelationClOrdID, order.CorrelationClOrdID);

	switch (order.OrdStatus[0])
	{
	case _T('P')://新订单
		{
			if (_T('D') == order.MsgType[0])//下单
				m_pDlg->m_lvOrderInfoList.SetItemText(0, OrderInfo_Column_Status, _T("Pending New"));
			else if (_T('F') == order.MsgType[0])//撤单
				m_pDlg->m_lvOrderInfoList.SetItemText(0, OrderInfo_Column_Status, _T("Pending Cancel"));
			else if (_T('G') == order.MsgType[0])//改单
				m_pDlg->m_lvOrderInfoList.SetItemText(0, OrderInfo_Column_Status, _T("Pending Modification"));
		}
		break;
	case '0'://Order Creation
		m_pDlg->m_lvOrderInfoList.SetItemText(0, OrderInfo_Column_Status, _T("New"));
		break;
	case '1'://Partial fill Notice
		m_pDlg->m_lvOrderInfoList.SetItemText(0, OrderInfo_Column_Status, _T("Partial filled"));
		break;
	case 'U'://Undefined
		m_pDlg->m_lvOrderInfoList.SetItemText(0, OrderInfo_Column_Status, _T("Undefined"));
		break;
	default: 
		WriteLog(LOG_ERROR, _T("[AddOrderInfoList]: OrdStatus=%s"), order.OrdStatus);
		m_pDlg->m_lvOrderInfoList.DeleteItem(0);
		return;
	}
	m_pDlg->m_lvOrderInfoList.SetItemText(0, OrderInfo_Column_OrderID, order.OrderID);

	m_pDlg->m_lvOrderInfoList.SetItemText(0, OrderInfo_Column_SecurityDesc, order.SecurityDesc);

	m_pDlg->m_lvOrderInfoList.SetItemText(0, OrderInfo_Column_Side, order.Side[0] == _T('1') ? _T("Buy") : _T("Sell"));

	m_pDlg->m_lvOrderInfoList.SetItemText(0, OrderInfo_Column_OrderQty, order.OrderQty);

	m_pDlg->m_lvOrderInfoList.SetItemText(0, OrderInfo_Column_MaxShow, order.MaxShow);

	CString csOrdType;
	switch (order.OrdType[0])
	{
	case _T('1'): csOrdType = _T("Market"); break;
	case _T('2'): csOrdType = _T("Limit"); break;
	case _T('3'): csOrdType = _T("Stop"); break;
	case _T('4'): csOrdType = _T("StopLimit"); break;
	case _T('K'): csOrdType = _T("MarketLimit"); break;
	default: break;
	}
	m_pDlg->m_lvOrderInfoList.SetItemText(0, OrderInfo_Column_OrderType, csOrdType);

	m_pDlg->m_lvOrderInfoList.SetItemText(0, OrderInfo_Column_Price, order.Price);

	m_pDlg->m_lvOrderInfoList.SetItemText(0, OrderInfo_Column_StopPx, order.StopPx);

	CString csTimeInForce;
	switch (order.TimeInForce[0])
	{
	case _T('0'): csTimeInForce = _T("Day"); break;
	case _T('1'): csTimeInForce = _T("GTC"); break;
	case _T('3'): csTimeInForce = _T("FaK"); break;
	case _T('6'): csTimeInForce = _T("GTD"); break;
	default: break;
	}

	m_pDlg->m_lvOrderInfoList.SetItemText(0, OrderInfo_Column_TimeInForce, csTimeInForce);

	m_pDlg->m_lvOrderInfoList.SetItemText(0, OrderInfo_Column_MinQty, order.MinQty);

	m_pDlg->m_lvOrderInfoList.SetItemText(0, OrderInfo_Column_ExpireDate, order.ExpireDate);

}

void Worker::UpdateOrder(ORDER& order)
{
	//更新
	EnterCriticalSection(&m_csOrder);
	m_mapClOrderIDToOrder[order.ClOrdID] = order;
	LeaveCriticalSection(&m_csOrder);

	//界面
	LVFINDINFO info;
	int nIndex;

	info.flags = LVFI_STRING;
	info.psz = order.ClOrdID;

	if ((nIndex = m_pDlg->m_lvOrderInfoList.FindItem(&info)) == -1)
	{
		WriteLog(LOG_ERROR, _T("[UpdateOrderInfoList]: Can't find Order. ClOrderID:%s"), order.ClOrdID);
		return;
	}

	switch (order.OrdStatus[0])
	{
	case '-'://被改、撤
		m_pDlg->m_lvOrderInfoList.SetItemText(0, OrderInfo_Column_Status, _T("Disabled"));
		break;
	case '0'://Order Creation
		m_pDlg->m_lvOrderInfoList.SetItemText(0, OrderInfo_Column_Status, _T("New"));
		break;
	case '4'://Order Cancel
		m_pDlg->m_lvOrderInfoList.SetItemText(0, OrderInfo_Column_Status, _T("Cancelled"));
		break;
	case '5'://Order Modify
		m_pDlg->m_lvOrderInfoList.SetItemText(0, OrderInfo_Column_Status, _T("Modified"));
		break;
	case '1'://Partial fill Notice
		m_pDlg->m_lvOrderInfoList.SetItemText(0, OrderInfo_Column_Status, _T("Partial filled"));
		break;
	case '2'://Complete fill Notice
		m_pDlg->m_lvOrderInfoList.SetItemText(0, OrderInfo_Column_Status, _T("Complete filled"));
		break;
	case 'C'://Order Elimination
		m_pDlg->m_lvOrderInfoList.SetItemText(0, OrderInfo_Column_Status, _T("Expired"));
		break;
	case '8'://Rejected
		m_pDlg->m_lvOrderInfoList.SetItemText(0, OrderInfo_Column_Status, _T("Rejected"));
		break;
	case 'H'://Trade Cancel
		m_pDlg->m_lvOrderInfoList.SetItemText(0, OrderInfo_Column_Status, _T("Trade Cancelled"));
		break;
	default: 
		WriteLog(LOG_ERROR, _T("[UpdateOrderInfoList]: OrdStatus=%s"), order.OrdStatus);
		return;
	}
}

void Worker::ExtractOrderFromExcReport( ORDER& order, const EXCREPORT& excReport )
{
	strcpy_s(order.ClOrdID, excReport.ClOrdID);
	strcpy_s(order.OrigClOrdID, excReport.OrigClOrdID);
	strcpy_s(order.CorrelationClOrdID, excReport.CorrelationClOrdID);
	strcpy_s(order.OrdStatus, excReport.OrdStatus);
	strcpy_s(order.OrderID, excReport.OrderID);
	strcpy_s(order.SecurityDesc, excReport.SecurityDesc);
	strcpy_s(order.Side, excReport.Side);
	strcpy_s(order.OrderQty, excReport.OrderQty);
	strcpy_s(order.MinQty, excReport.MinQty);
	strcpy_s(order.MaxShow, excReport.MaxShow);
	strcpy_s(order.OrdType, excReport.OrdType);
	strcpy_s(order.Price, excReport.Price);
	strcpy_s(order.StopPx, excReport.StopPx);
	strcpy_s(order.TimeInForce, excReport.TimeInForce);
	strcpy_s(order.ExpireDate, excReport.ExpireDate);
}

BOOL Worker::GetOrderByClOrderID(const CString csClOrderID, ORDER& order)
{
	EnterCriticalSection(&m_csOrder);
	MapClOrderIDToORDER::iterator i = m_mapClOrderIDToOrder.find(csClOrderID);
	if (i != m_mapClOrderIDToOrder.end())
	{
		order = i->second;
		LeaveCriticalSection(&m_csOrder);
		return TRUE;
	}
	else
	{
		LeaveCriticalSection(&m_csOrder);
		return FALSE; 
	}
}


UINT Worker::startQuote()
{
	WriteLog(LOG_INFO, "MDP3.0 Engine Starting, please wait...");
	//WriteLog(LOG_ERROR, "%s", szConfigPath);
	m_fLog.open("mfc.log", std::ios::out | std::ios::binary | std::ios::trunc);
	if (!m_fLog.is_open())
	{
		WriteLog(LOG_ERROR, "Open mfc.log failed\n");
		return -1;
	}

	ConfigStruct configStruct;
	GetCurrentDirectory(128, configStruct.configFile);
	GetCurrentDirectory(128, configStruct.templateFile);
	sprintf_s(configStruct.configFile, "%s\\config.xml", configStruct.configFile);//"..\\Release\\config.xml";////////////////argv[ 1 ];$(TargetDir)\\config.xml 
	sprintf_s(configStruct.templateFile, "%s\\templates_FixBinary.sbeir", configStruct.templateFile);//"..\\Release\\templates_FixBinary.sbeir";////////////////argv[ 2 ];$(TargetDir)\\templates_FixBinary.sbeir
	strcpy_s(configStruct.userName, "CME");
	strcpy_s(configStruct.passWord, "CME");
	strcpy_s(configStruct.localInterface, "172.17.120.92");//"10.25.1.148";

	if ( StartEngine(&configStruct, this) )
	{
		WriteLog(LOG_ERROR, "Start MDP3.0 Engine failed! [%s]", configStruct.errorInfo);
		return -1;
	}

	//Sleep(1000*500);

	//stop();
	WriteLog(LOG_INFO, "MDP3.0 Engine Started!");
	return 0;
}

UINT Worker::stopQuote()
{
	WriteLog(LOG_INFO, "Engine Stopping, please wait...");
	if (StopEngine())
	{
		WriteLog(LOG_ERROR, "Stop engine failed! Exiting...");
		return -1;
	}
	m_fLog.close();
	WriteLog(LOG_INFO, "Engine Stopped!");
	return 0;
}



void Worker::OnUpdate(MDPFieldMap* pFieldMap)
{
	if (pFieldMap == NULL)
		return;

	MDPField* pField = NULL;
	MDPFieldMap* pFieldMapInGroup = NULL;
	int nCount, i;

	Instrument inst;
	memset(&inst, 0, sizeof(inst));
	if (pField = pFieldMap->getField(FIELD::SecurityID))//CME合约唯一ID,Map Key
	{
		inst.SecurityID = (int)pField->getInt();
	}

	if (inst.SecurityID <= 0)
	{
		m_fLog << "[OnUpdate]: error security id:" << inst.SecurityID << std::endl;
		return ;
	}

	if (pField = pFieldMap->getField(FIELD::Symbol))//CME合约代码 Instrument Name or Symbol
	{
		pField->getArray(0, inst.Symbol, 0, pField->length());
	}
	char action;//Last Security update action on Incremental feed, 'D' or 'M' is used when a mid-week deletion or modification (i.e. extension) occurs
	if (pField = pFieldMap->getField(FIELD::SecurityUpdateAction))
	{
		action = (char)pField->getUInt();
	}

	if (action == 'D')//删除合约
	{
		//查询是否已经在合约列表中
		MapIntToInstrument::iterator iter = m_mapSecurityID2Inst.find(inst.SecurityID);
		if (iter != m_mapSecurityID2Inst.end())
		{
			m_fLog << "[OnUpdate]:Delete Instrument, Symbol:" << inst.Symbol << std::endl;
			m_mapSecurityID2Inst.erase(iter);
		}
		return;
	}

	//如果是修改合约，且不在当前的合约列表中，当作新增合约处理
	if (action == 'M')
	{
		m_fLog << "[OnUpdate]:Modify Instrument, Symbol:" << inst.Symbol << std::endl;
	}
	/************************************新增合约****************************************/
	//先获取以下关键字段
	m_fLog << "[OnUpdate]: security id:" << inst.SecurityID << ", action: " << action << std::endl;

	if (pField = pFieldMap->getField(FIELD::SecurityExchange))//外部交易所
	{
		pField->getArray(0, inst.SecurityExchange, 0, pField->length());
	}

	if (pField = pFieldMap->getField(FIELD::Asset))//外部商品
	{
		pField->getArray(0, inst.Asset, 0, pField->length());
	}

	if (pField = pFieldMap->getField(FIELD::DisplayFactor))//显示价格倍率
	{
		inst.DisplayFactor = pField->getDouble();// * pow((double)10, -7);
	}

	if (pField = pFieldMap->getField(FIELD::MDSecurityTradingStatus))//TODO:合约状态
	{
		inst.SecurityTradingStatus = (int)pField->getUInt();
	}

	if (pField = pFieldMap->getField(FIELD::ApplID))//Channel ID
	{
		inst.ApplID = (int)pField->getInt();
	}

	if (pField = pFieldMap->getField(FIELD::SecurityGroup))//交易中用到
	{
		pField->getArray(0, inst.SecurityGroup, 0, pField->length(0));
	}

	nCount = pFieldMap->getFieldMapNumInGroup(FIELD::NoMdFeedTypes);
	inst.GBXMarketDepth = 10;
	inst.GBIMarketDepth = 2;
	for (i = 0; i < nCount; i++)
	{
		if (pFieldMapInGroup = pFieldMap->getFieldMapPtrInGroup(FIELD::NoMdFeedTypes, i))
		{
			char szMDFeedType[4] = {0};
			int nMarketDepth = 0;
			if (pField = pFieldMapInGroup->getField(FIELD::MDFeedType))
			{
				pField->getArray(0, szMDFeedType, 0, pField->length());
			}
			if (pField = pFieldMapInGroup->getField(FIELD::MarketDepth))
			{
				nMarketDepth = (int)pField->getInt();
				//行情深度字段在此校验
				if (nMarketDepth < 2 || nMarketDepth > 10)
				{
					continue;
				}
			}
			if (strncmp(szMDFeedType, "GBX", 3) == 0)
			{
				//m_fnWriteLog(LOG_INFO, "[OnUpdate]:GBX Market Depth:%d", nMarketDepth);
				inst.GBXMarketDepth = nMarketDepth;
			}
			else if (strncmp(szMDFeedType, "GBI", 3) == 0)
			{
				//m_fnWriteLog(LOG_INFO, "[OnUpdate]:GBI Market Depth:%d", nMarketDepth);
				inst.GBIMarketDepth = nMarketDepth;
			}
		}
	}

	m_mapSecurityID2Inst[inst.SecurityID] = inst;
}

int Worker::GetInstrumentBySecurityID(const int securityID, Instrument& inst)
{
	MapIntToInstrument::iterator iter = m_mapSecurityID2Inst.find(securityID);
	if (iter != m_mapSecurityID2Inst.end())
	{
		inst = iter->second;
		return 0;
	}
	return -1;
}

void Worker::onMarketData(MDPFieldMap* pMDPFieldMap, const int templateID)
{
	switch (templateID)
	{
	case 4://ChannelReset4
		m_fLog << "[onMarketData]: Received ChannelReset4\n" << std::endl;
		ChannelReset(pMDPFieldMap);
		break;
	case 12://AdminHeartbeat12
		m_fLog << "[onMarketData]: Received AdminHeartbeat12\n" << std::endl;
		break;
	case 27://MDInstrumentDefinitionFuture27
		m_fLog << "[onMarketData]: Received MDInstrumentDefinitionFuture27\n";
		OnUpdate(pMDPFieldMap);
		m_fLog << std::endl;
		break;
	case 29://MDInstrumentDefinitionSpread29
		m_fLog << "[onMarketData]: Received MDInstrumentDefinitionSpread29\n";
		OnUpdate(pMDPFieldMap);
		m_fLog << std::endl;
		break;
	case 30://SecurityStatus30
		m_fLog << "[onMarketData]: Received SecurityStatus30\n" << std::endl;
		SecurityStatus(pMDPFieldMap);
		break;
	case 41://MDInstrumentDefinitionOption41
		m_fLog << "[onMarketData]: Received MDInstrumentDefinitionOption41\n";
		OnUpdate(pMDPFieldMap);
		m_fLog << std::endl;
		break;
	case 32://MDIncrementalRefreshBook32
		m_fLog << "[onMarketData]: Received MDIncrementalRefreshBook32\n";
		UpdateBook(pMDPFieldMap);
		m_fLog << std::endl;
		break;
	case 33://MDIncrementalRefreshDailyStatistics33
		m_fLog << "[onMarketData]: Received MDIncrementalRefreshDailyStatistics33\n" << std::endl;
		UpdateDailyStatistics(pMDPFieldMap);
		break;
	case 34://MDIncrementalRefreshLimitsBanding34
		m_fLog << "[onMarketData]: Received MDIncrementalRefreshLimitsBanding34\n" << std::endl;
		break;
	case 35://MDIncrementalRefreshSessionStatistics35
		m_fLog << "[onMarketData]: Received MDIncrementalRefreshSessionStatistics35\n" << std::endl;
		UpdateSessionStatistics(pMDPFieldMap);
		break;
	case 36://MDIncrementalRefreshTrade36
		m_fLog << "[onMarketData]: Received MDIncrementalRefreshTrade36\n" << std::endl;
		UpdateTradeSummary(pMDPFieldMap);
		break;
	case 37://MDIncrementalRefreshVolume37
		m_fLog << "[onMarketData]: Received MDIncrementalRefreshVolume37\n" << std::endl;
		UpdateVolume(pMDPFieldMap);
		break;
	case 38://SnapshotFullRefresh38
		m_fLog << "[onMarketData]: Received SnapshotFullRefresh38\n" << std::endl;
		SnapShot(pMDPFieldMap);
		break;
	case 39://QuoteRequest39
		m_fLog << "[onMarketData]: Received QuoteRequest39\n" << std::endl;
		break;
	case 42://MDIncrementalRefreshTradeSummary42
		m_fLog << "[onMarketData]: Received MDIncrementalRefreshTradeSummary42\n" << std::endl;
		UpdateTradeSummary(pMDPFieldMap);
		break;
	default:
		m_fLog << "[onMarketData]: Received Unknown Message ID:" << templateID << std::endl << std::endl;
		break;
	}
}



void Worker::SnapShot(MDPFieldMap* pFieldMap)
{
	if (pFieldMap == NULL)
	{
		//WriteLog(LOG_INFO, "[SnapShot]:pFieldMap is null\n");
		return ;
	}

	MDPField* pField = NULL;
	MDPFieldMap* pFieldMapInGroup = NULL;
	int nSecurityID = 0;
	int nCount = 0;
	//Instrument* pInst = NULL;
	QuoteItem* qi = NULL;

	//合约唯一ID, 检查是否处理
	if (pField = pFieldMap->getField(FIELD::SecurityID))
	{
		nSecurityID = (int)pField->getInt();
	}

	Instrument inst;
	if (GetInstrumentBySecurityID(nSecurityID, inst))
	{
		//WriteLog(LOG_INFO, "[SnapShot]:GetInstrumentBySecurityID failed\n");
		return ;
	}

	//合约按ApplID分类
	MapIntToSet::iterator it = m_mapApplID2SecurityIDs.find(inst.ApplID); 
	if (it != m_mapApplID2SecurityIDs.end())
	{
		it->second.insert(nSecurityID); 
	}
	else
	{
		SetInt securityIDs;
		securityIDs.insert(nSecurityID);
		m_mapApplID2SecurityIDs[inst.ApplID] = securityIDs;
	}

	MapIntToQuote::const_iterator iter = m_mapSecurityID2Quote.find(nSecurityID);
	if(iter != m_mapSecurityID2Quote.end())
	{
		qi = iter->second;
	}
	else
	{
		qi = new QuoteItem;
		memset(qi, 0, sizeof(QuoteItem));
		m_mapSecurityID2Quote[nSecurityID] = qi;
		qi->securityID = nSecurityID;
		// 		strcpy(qi->szExchangeType, pInst->szExchangeType);
		// 		strcpy(qi->szCommodityType, pInst->szCommodityType);
		// 		strcpy(qi->szContractCode, pInst->szContractCode);
		// 		qi->cProductType = pInst->cProductType;
		// 		qi->cOptionsType = pInst->cOptionsType;
		// 		qi->fStrikePrice = pInst->fStrikePrice;
		// 		qi->cMarketStatus = pInst->cMarketStatus;
	}

	//TODO:合约状态
	if (pField = pFieldMap->getField(FIELD::MDSecurityTradingStatus))
	{
		qi->cMarketStatus = (int)pField->getUInt();
		/*
		int nMDSecurityTradingStatus = (int)pField->getUInt();
		switch (nMDSecurityTradingStatus)
		{
		case 2://Trading halt
		qi->cMarketStatus = MKT_PAUSE;
		break;
		case 4://Close
		qi->cMarketStatus = MKT_PRECLOSE;
		break;
		case 15://New Price Indication
		break;
		case 17://Ready to trade (start of session)
		qi->cMarketStatus = MKT_OPEN;
		break;
		case 18://Not available for trading
		qi->cMarketStatus = MKT_PRECLOSE;
		break;
		case 20://Unknown or Invalid
		qi->cMarketStatus = MKT_UNKNOW;
		break;
		case 21://Pre Open
		qi->cMarketStatus = MKT_PREOPEN;
		break;
		case 24://Pre-Cross
		break;
		case 25://Cross
		break;
		case 26://Post Close
		qi->cMarketStatus = MKT_CLOSE;
		break;
		case 103:
		break;
		default:
		break;
		}
		*/
	}

	//时间戳
	if (pField = pFieldMap->getField(FIELD::TransactTime))
	{
		struct tm * timeinfo;
		unsigned __int64 temp = pField->getUInt() / 1000000;
		int mSec = temp % 1000;
		time_t rawtime = temp / 1000;
		timeinfo = localtime(&rawtime);
		qi->nTimeStamp = timeinfo->tm_hour * 10000000 + timeinfo->tm_min * 100000 + timeinfo->tm_sec * 1000 + mSec;
	}

	nCount = pFieldMap->getFieldMapNumInGroup(FIELD::NoMDEntries);
	for (int i = 0; i < nCount; ++i)
	{
		if (pFieldMapInGroup = pFieldMap->getFieldMapPtrInGroup(FIELD::NoMDEntries, i))
		{
			char cMDEntryType = 0;
			double dMDEntryPx = 0;
			int nQty = 0;
			int nLevel = 0;
			if (pField = pFieldMapInGroup->getField(FIELD::MDEntryType))
			{
				cMDEntryType = (char)pField->getUInt();
			}
			if (pField = pFieldMapInGroup->getField(FIELD::MDEntryPx))
			{
				dMDEntryPx = (double)(pField->getInt() * pow(10.0, (int)pField->getInt(1)));// * pInst->DisplayFactor * pInst->convBase;
			}
			if (pField = pFieldMapInGroup->getField(FIELD::MDEntrySize))
			{
				nQty = (int)pField->getInt();
			}
			if (pField = pFieldMapInGroup->getField(FIELD::MDPriceLevel))
			{
				nLevel = (int)pField->getInt();
				if (nLevel > 10 || nLevel < 1)
					continue;
			}
			switch (cMDEntryType)
			{
			case '0':// Bid
				LevelChange(qi->bidPrice, nLevel-1, inst.GBXMarketDepth, dMDEntryPx);
				LevelChange(qi->bidVolume, nLevel-1, inst.GBXMarketDepth, nQty);
				break;
			case '1':// Offer
				LevelChange(qi->askPrice, nLevel-1, inst.GBXMarketDepth, dMDEntryPx);
				LevelChange(qi->askVolume, nLevel-1, inst.GBXMarketDepth, nQty);
				break;
			case 'E':// Implied Bid
				LevelChange(qi->impliedBid, nLevel-1, inst.GBIMarketDepth, dMDEntryPx);
				LevelChange(qi->impliedBidVol, nLevel-1, inst.GBIMarketDepth, nQty);
				break;
			case 'F':// Implied Offer
				LevelChange(qi->impliedAsk, nLevel-1, inst.GBIMarketDepth, dMDEntryPx);
				LevelChange(qi->impliedAskVol, nLevel-1, inst.GBIMarketDepth, nQty);
				break;
			case '2':// Trade
				qi->last = dMDEntryPx;
				qi->lastVolume = nQty;
				break;
			case '4':// Opening Price
				if (pField = pFieldMapInGroup->getField(FIELD::OpenCloseSettlFlag))
				{
					int nOpenCloseSettlFlag = (int)pField->getUInt();
					if (nOpenCloseSettlFlag == 0)//Daily Open Price
						qi->open = dMDEntryPx;
				}
				break;
			case '6':// Settlement Price
				if (pField = pFieldMapInGroup->getField(FIELD::SettlPriceType))
				{
					int uSettlPriceType = (int)pField->getUInt();
					if (uSettlPriceType == 3)
						qi->prevSettlementPrice = dMDEntryPx;
				}
				break;
			case '7':// Session High Trade Price
				qi->high = dMDEntryPx;
				break;
			case '8':// Session Low Trade Price
				qi->low = dMDEntryPx;
				break;
			case 'N':// Session High Bid
				break;
			case 'O':// Session Low Offer
				break;
			case 'B':// Cleared Trade Volume
				break;
			case 'C':// Open Interest
				qi->bearVolume = nQty;
				break;
			case 'W':// Fixing Price
				break;
			case 'e':// Electronic Volume
				qi->tolVolume = nQty;
				break;
			default:
				break;
			}

		}
	}

	//WriteLog(LOG_INFO, "[SnapShot]:push quote security ID:%d", qi->securityID);
	m_fLog << "[SnapShot]:push quote security ID:" << qi->securityID << std::endl;
	PushQuote(qi);
}

void Worker::UpdateBook(MDPFieldMap* pFieldMap)
{
	if (pFieldMap == NULL)
	{
		//WriteLog(LOG_INFO, "[UpdateBook]:pFieldMap is null\n");
		return ;
	}
	MDPField* pField = NULL;
	MDPFieldMap* pFieldMapInGroup = NULL;
	unsigned char uMatchEventIndicator = 0;
	int nTimeStamp = 0;
	if (pField = pFieldMap->getField(FIELD::MatchEventIndicator))
	{
		uMatchEventIndicator = (unsigned char)pField->getUInt();
	}

	//时间戳
	if (pField = pFieldMap->getField(FIELD::TransactTime))
	{
		struct tm * timeinfo;
		unsigned __int64 temp = pField->getUInt() / 1000000;
		int mSec = temp % 1000;
		time_t rawtime = temp / 1000;
		timeinfo = localtime(&rawtime);
		nTimeStamp = timeinfo->tm_hour * 10000000 + timeinfo->tm_min * 100000 + timeinfo->tm_sec * 1000 + mSec;
	}

	int nCount = pFieldMap->getFieldMapNumInGroup(FIELD::NoMDEntries);

	for (int j = 0; j < nCount; ++j)
	{
		//获取第i条消息
		if (pFieldMapInGroup = pFieldMap->getFieldMapPtrInGroup(FIELD::NoMDEntries, j))
		{
			//Instrument* pInst = NULL;
			QuoteItem* qi = NULL;
			int nSecurityID = 0;
			int nMDUpdateAction = 0;
			char cMDEntryType = 0;
			double dMDEntryPx = 0;
			int nQty = 0;
			int nLevel = 0;

			//合约唯一ID, 外部主键
			if (pField = pFieldMapInGroup->getField(FIELD::SecurityID))
			{
				nSecurityID = (int)pField->getInt();
			}

			m_fLog << "[UpdateBook]: SecurityID:" << nSecurityID << " ";
			Instrument inst;
			//是否在合约列表中
			if (GetInstrumentBySecurityID(nSecurityID, inst))
			{
				m_fLog <<  "GetInstrumentBySecurityID failed\n";
				continue;
			}

			//合约行情获取(创建)
			MapIntToQuote::const_iterator iter = m_mapSecurityID2Quote.find(nSecurityID);
			if(iter != m_mapSecurityID2Quote.end())
			{
				qi = iter->second;
			}
			else
			{
				m_fLog << "Can't find the existing QuoteItem, newing one" << std::endl;
				qi = new QuoteItem;
				memset(qi, 0, sizeof(QuoteItem));
				m_mapSecurityID2Quote[nSecurityID] = qi;
				//strcpy(qi->szExchangeType, pInst->szExchangeType);
				//strcpy(qi->szCommodityType, pInst->szCommodityType);
				//strcpy(qi->szContractCode, pInst->szContractCode);
				//qi->cProductType = pInst->cProductType;
				//qi->cOptionsType = pInst->cOptionsType;
				//qi->fStrikePrice = pInst->fStrikePrice;
				//qi->cMarketStatus = pInst->cMarketStatus;
			}

			if (pField = pFieldMapInGroup->getField(FIELD::MDEntryType))
			{
				cMDEntryType = (char)pField->getUInt();
			}
			if (pField = pFieldMapInGroup->getField(FIELD::MDUpdateAction))
			{
				nMDUpdateAction = (int)pField->getUInt();
			}
			if (pField = pFieldMapInGroup->getField(FIELD::MDPriceLevel))
			{
				nLevel = (int)pField->getUInt();
			}
			if (nLevel < 1 || nLevel >10 )
			{
				m_fLog << "Invalid nLevel: " << nLevel << std::endl;
				continue;
			}

			if (pField = pFieldMapInGroup->getField(FIELD::MDEntryPx))
			{
				dMDEntryPx = (double)(pField->getInt() * pow(10.0, (int)pField->getInt(1)));// pow(10.0, -7) * pInst->DisplayFactor * pInst->convBase;
			}
			if (pField = pFieldMapInGroup->getField(FIELD::MDEntrySize))
			{
				nQty = (int)pField->getInt();
			}

			switch (cMDEntryType)
			{
			case '0':// Bid
				m_fLog << "Bid ";
				if (nMDUpdateAction == 0)// New
				{
					m_fLog << "New ";
					LevelInsert(qi->bidPrice, nLevel-1, inst.GBXMarketDepth, dMDEntryPx);
					LevelInsert(qi->bidVolume, nLevel-1, inst.GBXMarketDepth, nQty);
				}
				else if (nMDUpdateAction == 1)// Change
				{
					m_fLog << "Change ";
					LevelChange(qi->bidPrice, nLevel-1, inst.GBXMarketDepth, dMDEntryPx);
					LevelChange(qi->bidVolume, nLevel-1, inst.GBXMarketDepth, nQty);
				}
				else if (nMDUpdateAction == 2)// Delete
				{
					m_fLog << "Delete ";
					LevelDelete(qi->bidPrice, nLevel-1, inst.GBXMarketDepth);
					LevelDelete(qi->bidVolume, nLevel-1, inst.GBXMarketDepth);
				}
				else if (nMDUpdateAction == 3)// Delete Thru
				{
					m_fLog << "Delete Thru";
					LevelClear(qi->bidPrice, inst.GBXMarketDepth);
					LevelClear(qi->bidVolume, inst.GBXMarketDepth);
				}
				else if (nMDUpdateAction == 4)// Delete From
				{
					m_fLog << "Delete From";
					LevelDelFrom(qi->bidPrice, nLevel, inst.GBXMarketDepth);
					LevelDelFrom(qi->bidVolume, nLevel, inst.GBXMarketDepth);
				}
				break;
			case '1':// Offer
				m_fLog << "Offer ";
				if (nMDUpdateAction == 0)// New
				{
					m_fLog << "New ";
					LevelInsert(qi->askPrice, nLevel-1, inst.GBXMarketDepth, dMDEntryPx);
					LevelInsert(qi->askVolume, nLevel-1, inst.GBXMarketDepth, nQty);
				}
				else if (nMDUpdateAction == 1)// Change
				{
					m_fLog << "Change ";
					LevelChange(qi->askPrice, nLevel-1, inst.GBXMarketDepth, dMDEntryPx);
					LevelChange(qi->askVolume, nLevel-1, inst.GBXMarketDepth, nQty);
				}
				else if (nMDUpdateAction == 2)// Delete
				{
					m_fLog << "Delete ";
					LevelDelete(qi->askPrice, nLevel-1, inst.GBXMarketDepth);
					LevelDelete(qi->askVolume, nLevel-1, inst.GBXMarketDepth);
				}
				else if (nMDUpdateAction == 3)// Delete Thru
				{
					m_fLog << "Delete Thru";
					LevelClear(qi->askPrice, inst.GBXMarketDepth);
					LevelClear(qi->askVolume, inst.GBXMarketDepth);
				}
				else if (nMDUpdateAction == 4)// Delete From
				{
					m_fLog << "Delete From";
					LevelDelFrom(qi->askPrice, nLevel, inst.GBXMarketDepth);
					LevelDelFrom(qi->askVolume, nLevel, inst.GBXMarketDepth);
				}
				break;
			case 'E':// Implied Bid
				m_fLog << "Implied Bid ";
				if (nMDUpdateAction == 0)// New
				{
					m_fLog << "New ";
					LevelInsert(qi->impliedBid, nLevel-1, inst.GBIMarketDepth, dMDEntryPx);
					LevelInsert(qi->impliedBidVol, nLevel-1, inst.GBIMarketDepth, nQty);
				}
				else if (nMDUpdateAction == 1)// Change
				{
					m_fLog << "Change ";
					LevelChange(qi->impliedBid, nLevel-1, inst.GBIMarketDepth, dMDEntryPx);
					LevelChange(qi->impliedBidVol, nLevel-1, inst.GBIMarketDepth, nQty);
				}
				else if (nMDUpdateAction == 2)// Delete
				{
					m_fLog << "Delete ";
					LevelDelete(qi->impliedBid, nLevel-1, inst.GBIMarketDepth);
					LevelDelete(qi->impliedBidVol, nLevel-1, inst.GBIMarketDepth);
				}
				break;
			case 'F':// Implied Offer
				m_fLog << "Implied Offer ";
				if (nMDUpdateAction == 0)// New
				{
					m_fLog << "New ";
					LevelInsert(qi->impliedAsk, nLevel-1, inst.GBIMarketDepth, dMDEntryPx);
					LevelInsert(qi->impliedAskVol, nLevel-1, inst.GBIMarketDepth, nQty);
				}
				else if (nMDUpdateAction == 1)// Change
				{
					m_fLog << "Change ";
					LevelChange(qi->impliedAsk, nLevel-1, inst.GBIMarketDepth, dMDEntryPx);
					LevelChange(qi->impliedAskVol, nLevel-1, inst.GBIMarketDepth, nQty);
				}
				else if (nMDUpdateAction == 2)// Delete
				{
					m_fLog << "Delete ";
					LevelDelete(qi->impliedAsk, nLevel-1, inst.GBIMarketDepth);
					LevelDelete(qi->impliedAskVol, nLevel-1, inst.GBIMarketDepth);
				}
				break;
			default:
				break;
			}
			qi->nTimeStamp = nTimeStamp;
			//if (uMatchEventIndicator & 0x80)//Last message for the event
			//{
			m_fLog << "nLevel: " << nLevel << std::endl;
			PushQuote(qi);
			//}	
		}
	}
}

void Worker::UpdateSessionStatistics(MDPFieldMap* pFieldMap)
{
	if (pFieldMap == NULL)
	{
		//WriteLog(LOG_INFO, "[QuoteManger::UpdateSessionStatistics]:pFieldMap is null\n");
		return ;
	}
	MDPField* pField = NULL;
	MDPFieldMap* pFieldMapInGroup = NULL;
	unsigned char uMatchEventIndicator = 0;
	int nTimeStamp = 0;
	if (pField = pFieldMap->getField(FIELD::MatchEventIndicator))
	{
		uMatchEventIndicator = (unsigned char)pField->getUInt();
	}
	//时间戳
	if (pField = pFieldMap->getField(FIELD::TransactTime))
	{
		struct tm * timeinfo;
		unsigned __int64 temp = pField->getUInt() / 1000000;
		int mSec = temp % 1000;
		time_t rawtime = temp / 1000;
		timeinfo = localtime(&rawtime);
		nTimeStamp = timeinfo->tm_hour * 10000000 + timeinfo->tm_min * 100000 + timeinfo->tm_sec * 1000 + mSec;
	}
	int nCount = pFieldMap->getFieldMapNumInGroup(FIELD::NoMDEntries);

	for (int j = 0; j < nCount; ++j)
	{
		//获取第i条消息
		if (pFieldMapInGroup = pFieldMap->getFieldMapPtrInGroup(FIELD::NoMDEntries, j))
		{
			//Instrument* pInst = NULL;
			QuoteItem* qi = NULL;
			int nSecurityID = 0;
			int nMDUpdateAction = 0;
			char cMDEntryType = 0;
			double dMDEntryPx = 0;
			int nQty = 0;
			int nLevel = 0;



			//合约唯一ID, 外部主键
			if (pField = pFieldMapInGroup->getField(FIELD::SecurityID))
			{
				nSecurityID = (int)pField->getInt();
			}
			//是否在合约列表中
			// 		if (m_pdtMgr->GetInstrumentBySecurityID(nSecurityID, pInst))
			// 		{
			// 			continue;
			// 		}
			//合约行情获取(创建)
			MapIntToQuote::const_iterator iter = m_mapSecurityID2Quote.find(nSecurityID);
			if(iter != m_mapSecurityID2Quote.end())
			{
				qi = iter->second;
			}
			else
			{
				qi = new QuoteItem;
				memset(qi, 0, sizeof(QuoteItem));
				m_mapSecurityID2Quote[nSecurityID] = qi;
				// 			strcpy(qi->szExchangeType, pInst->szExchangeType);
				// 			strcpy(qi->szCommodityType, pInst->szCommodityType);
				// 			strcpy(qi->szContractCode, pInst->szContractCode);
				// 			qi->cProductType = pInst->cProductType;
				// 			qi->cOptionsType = pInst->cOptionsType;
				// 			qi->fStrikePrice = pInst->fStrikePrice;
				// 			qi->cMarketStatus = pInst->cMarketStatus;
			}

			if (pField = pFieldMapInGroup->getField(FIELD::MDEntryType))
			{
				cMDEntryType = (char)pField->getUInt();
			}
			if (pField = pFieldMapInGroup->getField(FIELD::MDUpdateAction))
			{
				nMDUpdateAction = (int)pField->getUInt();
			}
			if (pField = pFieldMapInGroup->getField(FIELD::MDEntryPx))
			{
				dMDEntryPx = (double)(pField->getInt() * pow(10.0, (int)pField->getInt(1)));// * pow(10.0, -7) * pInst->DisplayFactor * pInst->convBase;
			}
			switch (cMDEntryType)
			{
			case '4':// Open Price
				if (pField = pFieldMapInGroup->getField(FIELD::OpenCloseSettlFlag))
				{
					int nOpenCloseSettlFlag = (int)pField->getUInt();
					if (nOpenCloseSettlFlag == 0)//Daily Open Price
						qi->open = dMDEntryPx;
				}
				break;
			case '7':// High Trade
				qi->high = dMDEntryPx;
				break;
			case '8':// Low Trade
				qi->low = dMDEntryPx;
				break;
			case 'N':// Highest Bid
				break;
			case 'O':// Lowest Offer
				break;
			default:
				break;
			}
			qi->nTimeStamp = nTimeStamp;
			//if (uMatchEventIndicator & 0x80)
			//{
			PushQuote(qi);
			//}
		}
	}
}

void Worker::UpdateDailyStatistics(MDPFieldMap* pFieldMap)
{
	if (pFieldMap == NULL)
	{
		//WriteLog(LOG_INFO, "[UpdateDailyStatistics]:pFieldMap is null\n");
		return ;
	}
	MDPField* pField = NULL;
	MDPFieldMap* pFieldMapInGroup = NULL;
	unsigned char uMatchEventIndicator = 0;
	int nTimeStamp = 0;
	if (pField = pFieldMap->getField(FIELD::MatchEventIndicator))
	{
		uMatchEventIndicator = (unsigned char)pField->getUInt();
	}
	//时间戳
	if (pField = pFieldMap->getField(FIELD::TransactTime))
	{
		struct tm * timeinfo;
		unsigned __int64 temp = pField->getUInt() / 1000000;
		int mSec = temp % 1000;
		time_t rawtime = temp / 1000;
		timeinfo = localtime(&rawtime);
		nTimeStamp = timeinfo->tm_hour * 10000000 + timeinfo->tm_min * 100000 + timeinfo->tm_sec * 1000 + mSec;
	}
	int nCount = pFieldMap->getFieldMapNumInGroup(FIELD::NoMDEntries);

	for (int j = 0; j < nCount; ++j)
	{
		//获取第i条消息
		if (pFieldMapInGroup = pFieldMap->getFieldMapPtrInGroup(FIELD::NoMDEntries, j))
		{

			//Instrument* pInst = NULL;
			QuoteItem* qi = NULL;
			int nSecurityID = 0;
			int nMDUpdateAction = 0;
			char cMDEntryType = 0;
			double dMDEntryPx = 0;
			int nQty = 0;
			int nLevel = 0;


			//合约唯一ID, 外部主键
			if (pField = pFieldMapInGroup->getField(FIELD::SecurityID))
			{
				nSecurityID = (int)pField->getInt();
			}
			//是否在合约列表中
			// 		if (m_pdtMgr->GetInstrumentBySecurityID(nSecurityID, pInst))
			// 		{
			// 			continue;
			// 		}
			//合约行情获取(创建)
			MapIntToQuote::const_iterator iter = m_mapSecurityID2Quote.find(nSecurityID);
			if(iter != m_mapSecurityID2Quote.end())
			{
				qi = iter->second;
			}
			else
			{
				qi = new QuoteItem;
				memset(qi, 0, sizeof(QuoteItem));
				m_mapSecurityID2Quote[nSecurityID] = qi;
				// 			strcpy(qi->szExchangeType, pInst->szExchangeType);
				// 			strcpy(qi->szCommodityType, pInst->szCommodityType);
				// 			strcpy(qi->szContractCode, pInst->szContractCode);
				// 			qi->cProductType = pInst->cProductType;
				// 			qi->cOptionsType = pInst->cOptionsType;
				// 			qi->fStrikePrice = pInst->fStrikePrice;
				// 			qi->cMarketStatus = pInst->cMarketStatus;
			}

			if (pField = pFieldMapInGroup->getField(FIELD::MDEntryType))
			{
				cMDEntryType = (char)pField->getUInt();
			}
			if (pField = pFieldMapInGroup->getField(FIELD::MDUpdateAction))
			{
				nMDUpdateAction = (int)pField->getUInt();
			}
			if (pField = pFieldMapInGroup->getField(FIELD::MDEntryPx))
			{
				dMDEntryPx = (double)(pField->getInt() * pow(10.0, (int)pField->getInt(1)));// * pow(10.0, -7) * pInst->DisplayFactor * pInst->convBase;
			}
			if (pField = pFieldMapInGroup->getField(FIELD::MDEntrySize))
			{
				nQty = (int)pField->getInt();
			}
			switch (cMDEntryType)
			{
			case '6':// SettlementPrice
				if (pField = pFieldMapInGroup->getField(FIELD::SettlPriceType))
				{
					int uSettlPriceType = (int)pField->getUInt();
					if (uSettlPriceType == 3)
						qi->prevSettlementPrice = dMDEntryPx;
				}
				break;
			case 'B':// ClearedVolume
				break;
			case 'C':// OpenInterest
				qi->bearVolume = nQty;
				break;
			case 'W':// FixingPrice
				break;
			default:
				break;
			}
			qi->nTimeStamp = nTimeStamp;
			//if (uMatchEventIndicator & 0x80)
			//{
			PushQuote(qi);
			//}
		}
	}
}

void Worker::UpdateVolume(MDPFieldMap* pFieldMap)
{
	if (pFieldMap == NULL)
	{
		//WriteLog(LOG_INFO, "[UpdateVolume]:pFieldMap is null\n");
		return ;
	}
	MDPField* pField = NULL;
	MDPFieldMap* pFieldMapInGroup = NULL;
	unsigned char uMatchEventIndicator = 0;
	int nTimeStamp = 0;
	if (pField = pFieldMap->getField(FIELD::MatchEventIndicator))
	{
		uMatchEventIndicator = (unsigned char)pField->getUInt();
	}
	//时间戳
	if (pField = pFieldMap->getField(FIELD::TransactTime))
	{
		struct tm * timeinfo;
		unsigned __int64 temp = pField->getUInt() / 1000000;
		int mSec = temp % 1000;
		time_t rawtime = temp / 1000;
		timeinfo = localtime(&rawtime);
		nTimeStamp = timeinfo->tm_hour * 10000000 + timeinfo->tm_min * 100000 + timeinfo->tm_sec * 1000 + mSec;
	}
	int nCount = pFieldMap->getFieldMapNumInGroup(FIELD::NoMDEntries);

	for (int j = 0; j < nCount; ++j)
	{
		//获取第i条消息
		if (pFieldMapInGroup = pFieldMap->getFieldMapPtrInGroup(FIELD::NoMDEntries, j))
		{

			//Instrument* pInst = NULL;
			QuoteItem* qi = NULL;
			int nSecurityID = 0;
			int nMDUpdateAction = 0;
			char cMDEntryType = 0;
			int nQty = 0;

			//合约唯一ID, 外部主键
			if (pField = pFieldMapInGroup->getField(FIELD::SecurityID))
			{
				nSecurityID = (int)pField->getInt();
			}
			//是否在合约列表中
			// 		if (m_pdtMgr->GetInstrumentBySecurityID(nSecurityID, pInst))
			// 		{
			// 			continue;
			// 		}
			//合约行情获取(创建)
			MapIntToQuote::const_iterator iter = m_mapSecurityID2Quote.find(nSecurityID);
			if(iter != m_mapSecurityID2Quote.end())
			{
				qi = iter->second;
			}
			else
			{
				qi = new QuoteItem;
				memset(qi, 0, sizeof(QuoteItem));
				m_mapSecurityID2Quote[nSecurityID] = qi;
				// 			strcpy(qi->szExchangeType, pInst->szExchangeType);
				// 			strcpy(qi->szCommodityType, pInst->szCommodityType);
				// 			strcpy(qi->szContractCode, pInst->szContractCode);
				// 			qi->cProductType = pInst->cProductType;
				// 			qi->cOptionsType = pInst->cOptionsType;
				// 			qi->fStrikePrice = pInst->fStrikePrice;
				// 			qi->cMarketStatus = pInst->cMarketStatus;
			}

			if (pField = pFieldMapInGroup->getField(FIELD::MDEntryType))
			{
				cMDEntryType = (char)pField->getUInt();
			}
			if (pField = pFieldMapInGroup->getField(FIELD::MDUpdateAction))
			{
				nMDUpdateAction = (int)pField->getUInt();
			}
			if (pField = pFieldMapInGroup->getField(FIELD::MDEntrySize))
			{
				nQty = (int)pField->getInt();
			}

			if (cMDEntryType = 'e')
			{
				qi->tolVolume = nQty;
			}

			//if (uMatchEventIndicator & 0x80)
			//{
			PushQuote(qi);
			//}
		}
	}
}

void Worker::UpdateTradeSummary(MDPFieldMap* pFieldMap)
{
	if (pFieldMap == NULL)
	{
		//WriteLog(LOG_INFO, "[UpdateTradeSummary]:pFieldMap is null\n");
		return ;
	}
	MDPField* pField = NULL;
	MDPFieldMap* pFieldMapInGroup = NULL;
	unsigned char uMatchEventIndicator = 0;
	int nTimeStamp = 0;
	if (pField = pFieldMap->getField(FIELD::MatchEventIndicator))
	{
		uMatchEventIndicator = (unsigned char)pField->getUInt();
	}

	//时间戳
	if (pField = pFieldMap->getField(FIELD::TransactTime))
	{
		struct tm * timeinfo;
		unsigned __int64 temp = pField->getUInt() / 1000000;
		int mSec = temp % 1000;
		time_t rawtime = temp / 1000;
		timeinfo = localtime(&rawtime);
		nTimeStamp = timeinfo->tm_hour * 10000000 + timeinfo->tm_min * 100000 + timeinfo->tm_sec * 1000 + mSec;
	}

	int nCount = pFieldMap->getFieldMapNumInGroup(FIELD::NoMDEntries);

	for (int j = 0; j < nCount; ++j)
	{
		//获取第i条消息
		if (pFieldMapInGroup = pFieldMap->getFieldMapPtrInGroup(FIELD::NoMDEntries, j))
		{
			//Instrument* pInst = NULL;
			QuoteItem* qi = NULL;
			int nSecurityID = 0;
			int nMDUpdateAction = 0;
			char cMDEntryType = 0;
			double dMDEntryPx = 0;
			int nQty = 0;


			//合约唯一ID, 外部主键
			if (pField = pFieldMapInGroup->getField(FIELD::SecurityID))
			{
				nSecurityID = (int)pField->getInt();
			}
			//是否在合约列表中
			// 		if (m_pdtMgr->GetInstrumentBySecurityID(nSecurityID, pInst))
			// 		{
			// 			continue;
			// 		}
			//合约行情获取(创建)
			MapIntToQuote::const_iterator iter = m_mapSecurityID2Quote.find(nSecurityID);
			if(iter != m_mapSecurityID2Quote.end())
			{
				qi = iter->second;
			}
			else
			{
				qi = new QuoteItem;
				memset(qi, 0, sizeof(QuoteItem));
				m_mapSecurityID2Quote[nSecurityID] = qi;
				// 			strcpy(qi->szExchangeType, pInst->szExchangeType);
				// 			strcpy(qi->szCommodityType, pInst->szCommodityType);
				// 			strcpy(qi->szContractCode, pInst->szContractCode);
				// 			qi->cProductType = pInst->cProductType;
				// 			qi->cOptionsType = pInst->cOptionsType;
				// 			qi->fStrikePrice = pInst->fStrikePrice;
				// 			qi->cMarketStatus = pInst->cMarketStatus;
			}
			if (pField = pFieldMapInGroup->getField(FIELD::MDEntryType))
			{
				cMDEntryType = (char )pField->getUInt();
			}
			if (pField = pFieldMapInGroup->getField(FIELD::MDUpdateAction))
			{
				nMDUpdateAction = (int )pField->getUInt();
			}
			if (pField = pFieldMapInGroup->getField(FIELD::MDEntrySize))
			{
				nQty = (int )pField->getInt();
			}
			if (pField = pFieldMapInGroup->getField(FIELD::MDEntryPx))
			{
				dMDEntryPx = (double)(pField->getInt() * pow(10.0, (int)pField->getInt(1)));// * pow(10.0, -7) * pInst->DisplayFactor * pInst->convBase;
			}

			if (cMDEntryType == '2' && nMDUpdateAction == 0 )
			{
				qi->last = dMDEntryPx;
				qi->lastVolume = nQty;
			}
			qi->nTimeStamp = nTimeStamp;

			//if (uMatchEventIndicator & 0x80)
			//{
			PushQuote(qi);
			//}
		}
	}
}

void Worker::SecurityStatus(MDPFieldMap* pFieldMap)
{
	if (pFieldMap == NULL)
	{
		//WriteLog(LOG_INFO, "[QuoteManger::SecurityStatus]:pFieldMap is null\n");
		return ;
	}
	MDPField* pField = NULL;
	MDPFieldMap* pFieldMapInGroup = NULL;
	unsigned char uMatchEventIndicator = 0;	
	//Instrument* pInst = NULL;
	QuoteItem* qi = NULL;
	int nSecurityID = 0;
	//If this tag is present, 35=f message is sent for the instrument
	if (pField = pFieldMap->getField(FIELD::SecurityID))
	{
		nSecurityID = (int)pField->getInt();
	}
	else//暂时只处理合约状态
	{
		return ;
	}
	//是否在合约列表中
	// 	if (m_pdtMgr->GetInstrumentBySecurityID(nSecurityID, pInst))
	// 	{
	// 		return ;
	// 	}
	//合约行情获取(创建)
	MapIntToQuote::const_iterator iter = m_mapSecurityID2Quote.find(nSecurityID);
	if(iter != m_mapSecurityID2Quote.end())
	{
		qi = iter->second;
	}
	else
	{
		qi = new QuoteItem;
		memset(qi, 0, sizeof(QuoteItem));
		m_mapSecurityID2Quote[nSecurityID] = qi;
		// 		strcpy(qi->szExchangeType, pInst->szExchangeType);
		// 		strcpy(qi->szCommodityType, pInst->szCommodityType);
		// 		strcpy(qi->szContractCode, pInst->szContractCode);
		// 		qi->cProductType = pInst->cProductType;
		// 		qi->cOptionsType = pInst->cOptionsType;
		// 		qi->fStrikePrice = pInst->fStrikePrice;
		// 		qi->cMarketStatus = pInst->cMarketStatus;
	}

	if (pField = pFieldMap->getField(FIELD::MDSecurityTradingStatus))//TODO:合约状态
	{
		qi->cMarketStatus = (int)pField->getUInt();
		/*
		int nSecurityTradingStatus = (int)pField->getUInt();
		switch (nSecurityTradingStatus)
		{
		case 2://Trading halt
		qi->cMarketStatus = MKT_PAUSE;
		break;
		// 		case 4://Close
		// 			pInst->cMarketStatus = MKT_PRECLOSE;
		// 			break;
		// 		case 15://New Price Indication
		// 			break;
		case 17://Ready to trade (start of session)
		qi->cMarketStatus = MKT_OPEN;
		break;
		case 18://Not available for trading
		qi->cMarketStatus = MKT_CLOSE;
		break;
		case 20://Unknown or Invalid
		qi->cMarketStatus = MKT_UNKNOW;
		break;
		case 21://Pre Open
		qi->cMarketStatus = MKT_PREOPEN;
		break;
		case 24://Pre-Cross
		break;
		case 25://Cross
		break;
		// 		case 26://Post Close
		// 			pInst->cMarketStatus = MKT_CLOSE;
		// 			break;
		case 103://No Change
		break;
		default:
		break;
		}
		*/
	}
	//时间戳
	if (pField = pFieldMap->getField(FIELD::TransactTime))
	{
		struct tm * timeinfo;
		unsigned __int64 temp = pField->getUInt() / 1000000;
		int mSec = temp % 1000;
		time_t rawtime = temp / 1000;
		timeinfo = localtime(&rawtime);
		qi->nTimeStamp = timeinfo->tm_hour * 10000000 + timeinfo->tm_min * 100000 + timeinfo->tm_sec * 1000 + mSec;
	}

	if (pField = pFieldMap->getField(FIELD::MatchEventIndicator))
	{
		uMatchEventIndicator = (unsigned char)pField->getUInt();
	}

	//if (uMatchEventIndicator & 0x80)
	//{
	PushQuote(qi);
	//}
}

void Worker::ChannelReset(MDPFieldMap* pFieldMap)
{
	if (pFieldMap == NULL)
	{
		//WriteLog(LOG_INFO, "[ChannelReset]:pFieldMap is null\n");
		return ;
	}
	MDPField* pField = NULL;
	MDPFieldMap* pFieldMapInGroup = NULL;
	unsigned char uMatchEventIndicator = 0;
	int nTimeStamp = 0;
	if (pField = pFieldMap->getField(FIELD::MatchEventIndicator))
	{
		uMatchEventIndicator = (unsigned char)pField->getUInt();
	}
	//时间戳
	if (pField = pFieldMap->getField(FIELD::TransactTime))
	{
		struct tm * timeinfo;
		unsigned __int64 temp = pField->getUInt() / 1000000;
		int mSec = temp % 1000;
		time_t rawtime = temp / 1000;
		timeinfo = localtime(&rawtime);
		nTimeStamp = timeinfo->tm_hour * 10000000 + timeinfo->tm_min * 100000 + timeinfo->tm_sec * 1000 + mSec;
	}
	int nSecurityID = 0;
	int nCount = pFieldMap->getFieldMapNumInGroup(FIELD::NoMDEntries);

	for (int i = 0; i < nCount; ++i)
	{
		if (pFieldMapInGroup = pFieldMap->getFieldMapPtrInGroup(FIELD::NoMDEntries, i))
		{

			int nMDUpdateAction = 0;
			char cMDEntryType = 0;
			int nApplID = 0;

			if (pField = pFieldMapInGroup->getField(FIELD::MDUpdateAction))
			{
				nMDUpdateAction = (int)pField->getUInt();
			}
			if (pField = pFieldMapInGroup->getField(FIELD::MDEntryType))
			{
				cMDEntryType = (char)pField->getUInt();
			}
			if (pField = pFieldMapInGroup->getField(FIELD::ApplID))
			{
				nApplID = (int)pField->getInt();
			}
			if (nMDUpdateAction == 0 && cMDEntryType == 'J')//Empty Book
			{
				MapIntToSet::iterator itMap = m_mapApplID2SecurityIDs.find(nApplID);
				if (itMap != m_mapApplID2SecurityIDs.end())
				{
					SetInt::iterator itSet = itMap->second.begin();
					while (itSet != itMap->second.end())
					{
						//Empty Book
						MapIntToQuote::iterator it = m_mapSecurityID2Quote.find(*itSet);
						if (it != m_mapSecurityID2Quote.end())
						{
							QuoteItem* qi = it->second;
							EmptyQuote(qi);
							qi->nTimeStamp = nTimeStamp;
							//if (uMatchEventIndicator & 0x80)
							//{
							PushQuote(qi);
							//}
						}
						itSet++;
					}
				}
			}
		}
	}
}

void Worker::updateOrderBook(int SecurityID)
{
	MapIntToQuote::iterator iter = m_mapSecurityID2Quote.find(SecurityID);
	if (iter != m_mapSecurityID2Quote.end())
	{
		PushQuote(iter->second);
	}
	else
		m_fLog << "updateOrderBook  can't find it: "<<SecurityID <<"\n";

}

void Worker::PushQuote( const QuoteItem* qi )
{
	if (!qi)
		return ;
	/*
	m_fLog << "[PushQuote]:security id:" << qi->securityID << endl;
	for (int k = 0; k < 10; k++)
	{
	m_fLog << "B  " << qi->bidPrice[k];
	m_fLog << "  No  "  << qi->bidVolume[k];
	m_fLog << "  [" << k << "]  ";
	m_fLog << "  S  " << qi->askPrice[k];
	m_fLog << "  No  "  << qi->askVolume[k];
	m_fLog << endl;
	}
	*/
	int i, nIndex;
	CString s;
	CListCtrl& lvQuote = m_pDlg->m_lvQuoteList;
	CListCtrl& lvOrderBook = m_pDlg->m_lvOrderBookList;
	BOOL bInTheList = FALSE;

	for (i = 0; i < lvQuote.GetItemCount(); i++)
	{
		if (qi->securityID == (int)lvQuote.GetItemData(i))//Already in the list
		{
			nIndex = i;
			bInTheList = TRUE;
			break;
		}
	}

	if (!bInTheList)//create item
	{
		nIndex = lvQuote.GetItemCount();
		s.Format("%d", qi->securityID);
		lvQuote.InsertItem(nIndex, s);
		lvQuote.SetItemData(nIndex, qi->securityID);
	}

	//市场状态
	s.Format("%d", qi->cMarketStatus);
	lvQuote.SetItemText(nIndex, 1, s);

	s.Format("%lf", qi->last);
	lvQuote.SetItemText(nIndex, 2, s);

	s.Format("%lf", qi->open);
	lvQuote.SetItemText(nIndex, 3, s);

	s.Format("%lf", qi->high);
	lvQuote.SetItemText(nIndex, 4, s);

	s.Format("%lf", qi->close);
	lvQuote.SetItemText(nIndex, 5, s);

	if (qi->securityID == m_pDlg->m_securityID)
	{
		ConsolidatedBook conBook = {0};
		MergeBook(&conBook, qi);
		CString s;
		for (int i = 0; i < 10; i++)
		{
			s.Format("%lf", conBook.bidPrice[i]);
			lvOrderBook.SetItemText(i, 1, s);
			s.Format("%d", conBook.bidVolume[i]);
			lvOrderBook.SetItemText(i, 2, s);
			s.Format("%lf", conBook.askPrice[i]);
			lvOrderBook.SetItemText(i, 4, s);
			s.Format("%d", conBook.askVolume[i]);
			lvOrderBook.SetItemText(i, 5, s);
		}
	}

	/*

	PRICEINFO priceinfo = {0};
	priceinfo.cMarketStatus = qi->cMarketStatus;
	priceinfo.fOpenPrice = qi->open;
	priceinfo.fHighPrice = qi->high;
	priceinfo.fLowPrice = qi->low;
	priceinfo.fClosePrice = qi->close;
	priceinfo.fLastPrice = qi->last;
	priceinfo.nLastAmount = qi->lastVolume;
	priceinfo.nBusinAmount = qi->tolVolume;
	priceinfo.fPrevClosePrice = qi->preClose;
	priceinfo.fPrevSettlementPrice = qi->prevSettlementPrice;
	priceinfo.nBearAmount = qi->bearVolume;

	ConsolidatedBook conBook;
	MergeBook(&conBook, qi);

	priceinfo.fBuyHighPrice = conBook.bidPrice[0];
	priceinfo.fBidPrice2 = conBook.bidPrice[1];
	priceinfo.fBidPrice3 = conBook.bidPrice[2];
	priceinfo.fBidPrice4 = conBook.bidPrice[3];
	priceinfo.fBidPrice5 = conBook.bidPrice[4];
	priceinfo.fBidPrice6 = conBook.bidPrice[5];
	priceinfo.fBidPrice7 = conBook.bidPrice[6];
	priceinfo.fBidPrice8 = conBook.bidPrice[7];
	priceinfo.fBidPrice9 = conBook.bidPrice[8];
	priceinfo.fBidPrice10 = conBook.bidPrice[9];

	priceinfo.nBuyHighAmount = conBook.bidVolume[0];
	priceinfo.nBidVolume2 = conBook.bidVolume[1];
	priceinfo.nBidVolume3 = conBook.bidVolume[2];
	priceinfo.nBidVolume4 = conBook.bidVolume[3];
	priceinfo.nBidVolume5 = conBook.bidVolume[4];
	priceinfo.nBidVolume6 = conBook.bidVolume[5];
	priceinfo.nBidVolume7 = conBook.bidVolume[6];
	priceinfo.nBidVolume8 = conBook.bidVolume[7];
	priceinfo.nBidVolume9 = conBook.bidVolume[8];
	priceinfo.nBidVolume10 = conBook.bidVolume[9];

	priceinfo.fSaleLowPrice = conBook.askPrice[0];
	priceinfo.fAskPrice2 = conBook.askPrice[1];
	priceinfo.fAskPrice3 = conBook.askPrice[2];
	priceinfo.fAskPrice4 = conBook.askPrice[3];
	priceinfo.fAskPrice5 = conBook.askPrice[4];
	priceinfo.fAskPrice6 = conBook.askPrice[5];
	priceinfo.fAskPrice7 = conBook.askPrice[6];
	priceinfo.fAskPrice8 = conBook.askPrice[7];
	priceinfo.fAskPrice9 = conBook.askPrice[8];
	priceinfo.fAskPrice10 = conBook.askPrice[9];

	priceinfo.nSaleLowAmount = conBook.askVolume[0];
	priceinfo.nAskVolume2 = conBook.askVolume[1];
	priceinfo.nAskVolume3 = conBook.askVolume[2];
	priceinfo.nAskVolume4 = conBook.askVolume[3];
	priceinfo.nAskVolume5 = conBook.askVolume[4];
	priceinfo.nAskVolume6 = conBook.askVolume[5];
	priceinfo.nAskVolume7 = conBook.askVolume[6];
	priceinfo.nAskVolume8 = conBook.askVolume[7];
	priceinfo.nAskVolume9 = conBook.askVolume[8];
	priceinfo.nAskVolume10 = conBook.askVolume[9];
	*/
}

void Worker::EmptyQuote(QuoteItem* qi)
{
	int i;
	for (i = 0; i < 10; i++)
	{
		qi->askPrice[i] = 0;
		qi->askVolume[i] = 0;
		qi->bidPrice[i] = 0;
		qi->bidVolume[i] = 0;
	}
	for (i = 0; i < 2; i++)
	{
		qi->impliedAsk[i] = 0;
		qi->impliedAskVol[i] = 0;
		qi->impliedBid[i] = 0;
		qi->impliedBidVol[i] = 0;
	}
	qi->open = 0;
	qi->high = 0;
	qi->low = 0;
	qi->last = 0;
	qi->close = 0;
	qi->lastVolume = 0;
	qi->bearVolume = 0;
	qi->preClose = 0;
	qi->prevSettlementPrice = 0;
	qi->tolVolume = 0;
	qi->cMarketStatus = 0;
	return ;
}


//合并implied book 按价格排序，相同价格的数量相加
void Worker::MergeBook(ConsolidatedBook* conBook, const QuoteItem* qi)
{
	int i = 0;//Implied Book Level
	int j = 0;//Multiple-Depth Book Level
	int nLevel;//Consolidated Book Level

	Instrument inst;
	if (GetInstrumentBySecurityID(qi->securityID, inst) == -1)
		return ;


	for (nLevel = 0; nLevel < inst.GBXMarketDepth; nLevel++)
	{
		if (i < 2)
		{
			//impliedBid高且数量不为零，改成implied
			if ( qi->impliedBid[i] > qi->bidPrice[j] && qi->impliedBidVol[i] > 0)
			{
				conBook->bidPrice[nLevel] = qi->impliedBid[i];
				conBook->bidVolume[nLevel] = qi->impliedBidVol[i];
				++i;
			}//相同且数量不为零，加上implied
			else if ( qi->impliedBid[i] == qi->bidPrice[j] && qi->impliedBidVol[i] > 0)
			{
				conBook->bidPrice[nLevel] = qi->impliedBid[i];
				conBook->bidVolume[nLevel] = qi->impliedBidVol[i] + qi->bidVolume[j];
				++i;
				++j;
			}
			else//直接复制
			{
				conBook->bidPrice[nLevel] = qi->bidPrice[j];
				conBook->bidVolume[nLevel] = qi->bidVolume[j];
				++j;
			}
		}
		else
		{
			conBook->bidPrice[nLevel] = qi->bidPrice[j];
			conBook->bidVolume[nLevel] = qi->bidVolume[j];
			++j;
		}
	}

	i = 0;
	j = 0;
	for (nLevel = 0; nLevel < inst.GBXMarketDepth; nLevel++)
	{
		if (i < 2)
		{
			//impliedBid小且数量不为零，改成implied
			if ( qi->impliedAsk[i] < qi->askPrice[j] && qi->impliedAskVol[i] > 0)
			{
				conBook->askPrice[nLevel] = qi->impliedAsk[i];
				conBook->askVolume[nLevel] = qi->impliedAskVol[i];
				++i;
			}//相同且数量不为零，加上implied
			else if ( qi->impliedAsk[i] == qi->askPrice[j] && qi->impliedAskVol[i] > 0)
			{
				conBook->askPrice[nLevel] = qi->impliedAsk[i];
				conBook->askVolume[nLevel] = qi->impliedAskVol[i] + qi->askVolume[j];
				++i;
				++j;
			}
			else//直接复制
			{
				conBook->askPrice[nLevel] = qi->askPrice[j];
				conBook->askVolume[nLevel] = qi->askVolume[j];
				++j;
			}
		}
		else
		{
			conBook->askPrice[nLevel] = qi->askPrice[j];
			conBook->askVolume[nLevel] = qi->askVolume[j];
			++j;
		}
	}
}


void FUNCTION_CALL_MODE Worker::onEvent( const ISessionID * lpSessionID, int iEventID )
{
	//	out<<"[OnEvent  ] "<<lpSessionID->lpSenderCompID;
	WriteLog(LOG_INFO, "[onEvent] Called ");
	switch(iEventID)
	{
	case 1:
		WriteLog(LOG_INFO, "connect failed...\n");
		break;
	case 2:
		WriteLog(LOG_INFO, "re-connect...\n");
		break;
	case 3:
		WriteLog(LOG_INFO, "connecting...\n");
		break;
	default:
		WriteLog(LOG_INFO, "onEvent default\n");
		//	cout<<" iEventID:"<<iEventID<<endl;
		break;
	}
}

void FUNCTION_CALL_MODE Worker::OnCreate( const ISessionID * lpSessionID )
{
	WriteLog(LOG_INFO, "[OnCreate] Called \n");
}

void FUNCTION_CALL_MODE Worker::OnLogon( const ISessionID * lpSessionID )
{
	WriteLog(LOG_INFO, "[OnLogon] Called \n");
	//	cout<<"[OnLogon  ]: "<<lpSessionID->lpTargetCompID<<endl;
	//	int size = sizeof(HSFixMsgBody);	

	m_pTradeSession = (ISessionID* )lpSessionID;
	if(m_hEventReadyToTrade)
	{
		SetEvent(m_hEventReadyToTrade);
	}
}

void FUNCTION_CALL_MODE Worker::OnLogout( const ISessionID * lpSessionID )
{
	WriteLog(LOG_INFO, "[OnLogout] Called \n");
}

void FUNCTION_CALL_MODE Worker::ToAdmin( IMessage * lpMsg, const ISessionID * lpSessionID )
{
	//  g_lpSessionID = (ISessionID* )lpSessionID;
	char szLastMsgSeqNumProcessed[9] = {0};
	ISession* pSession = GetSessionByID((ISessionID *)lpSessionID);
	const char* msgType = lpMsg->GetMessageType();
	HSFixHeader *pHeader = lpMsg->GetHeader();
	HSFixMsgBody *pBody = lpMsg->GetMsgBody();

	pHeader->SetFieldValue(FIELD::SenderSubID, "ShengShaoDong");
	pHeader->SetFieldValue(FIELD::TargetSubID, "G");
	pHeader->SetFieldValue(FIELD::SenderLocationID, "CN");
	pHeader->SetFieldValue(FIELD::LastMsgSeqNumProcessed, _ltoa(pSession->getExpectedTargetNum() - 1, szLastMsgSeqNumProcessed, 10 ) );

	if ( strncmp(msgType, "0", 1 ) == 0)
	{
		//WriteLog(LOG_INFO, "[Worker::ToAdmin]:heartbeat message send...\n");
	}
	else if ( strncmp(msgType, "2", 1) == 0 )	//CME规定重发请求的序列号范围必须在2500以内,所以在此干预
	{
		long uBeginSeqNum = atol(pBody->GetFieldValue(FIELD::BeginSeqNo));
		long uLastSeqNumProcessed = pSession->getExpectedTargetNum()-1;
		if ( uLastSeqNumProcessed - uBeginSeqNum >= 2500 )
		{
			char szEndSeqNo[9] = {0};
			pBody->SetFieldValue(FIELD::EndSeqNo, _ltoa(uBeginSeqNum+2499, szEndSeqNo, 10));
		}
		//WriteLog(LOG_INFO, "[Worker::ToAdmin]:resend request message send...\n");
	}
	else if (strncmp(msgType, "4", 1) == 0)
	{
		//WriteLog(LOG_INFO, "[Worker::ToAdmin]:sequense reset message send...\n");
	}
	else if(strncmp(msgType, "5", 1) == 0)
	{
		//WriteLog(LOG_INFO, "[Worker::ToAdmin]:logout message send...\n");
	}
	else if(strncmp(msgType, "A", 1) == 0)
	{
		//登录消息，可以在此处增加用户名和密码字段
		//pBody->SetFieldValue(FIELD::RawData, "JOXKS");
		pBody->SetFieldValue(FIELD::RawData, "4S5");
		pBody->SetFieldValue(FIELD::RawDataLength, "3");
		pBody->SetFieldValue(FIELD::ResetSeqNumFlag, "N");
		pBody->SetFieldValue(FIELD::EncryptMethod, "0");
		pBody->SetFieldValue(FIELD::ApplicationSystemName, "Hundsun UFOs");
		pBody->SetFieldValue(FIELD::TradingSystemVersion, "V7.0");
		pBody->SetFieldValue(FIELD::ApplicationSystemVendor, "Hundsun");
		//pHeader->SetFieldValue(FIELD::SenderCompID, "N");
		//WriteLog(LOG_INFO, "[Worker::ToAdmin]:logon message send...\n");
	}
}

int FUNCTION_CALL_MODE Worker::ToApp( IMessage * lpMsg, const ISessionID * lpSessionID )
{
	//WriteLog(LOG_INFO, "[Worker::ToApp]: called.\n");
	char szLastMsgSeqNumProcessed[9] = {0};
	ISession* pSession = GetSessionByID((ISessionID *)lpSessionID);
	HSFixHeader *pHeader = lpMsg->GetHeader();
	HSFixMsgBody *body = lpMsg->GetMsgBody();
	pHeader->SetFieldValue(FIELD::SenderSubID, "GZENG");
	pHeader->SetFieldValue(FIELD::TargetSubID, "G");
	pHeader->SetFieldValue(FIELD::SenderLocationID, "CN");
	pHeader->SetFieldValue(FIELD::LastMsgSeqNumProcessed, _ltoa(pSession->getExpectedTargetNum() - 1, szLastMsgSeqNumProcessed, 10 ) );
	return 0;
}

int FUNCTION_CALL_MODE Worker::FromAdmin( const IMessage * lpMsg , const ISessionID * lpSessionID )
{
	const char* msgType = lpMsg->GetMessageType();
	HSFixHeader* pHeader = lpMsg->GetHeader();
	//	strcpy(m_lastSeqNumReceived, pHeader->GetFieldValue(FIELD::MsgSeqNum));

	if (strncmp(msgType, "A", 1) == 0)			//CME Globex发来的登陆确认消息
	{
		//WriteLog(LOG_INFO, "[Worker::FromAdmin]:received a Logon message\n");
	} 
	else if (strncmp(msgType, "5", 1) == 0)		//CME Globex发来的登出消息
	{
		//WriteLog(LOG_INFO, "[Worker::FromAdmin]:received a Logout message\n");
	}
	else if (strncmp(msgType, "0", 1) == 0)		//CME Globex发来的心跳消息
	{
		//WriteLog(LOG_INFO, "[Worker::FromAdmin]:received a heartbeat message\n");
	}
	else if (strncmp(msgType, "1", 1) == 0)		//CME Globex发来的心跳测试消息
	{
		//WriteLog(LOG_INFO, "[Worker::FromAdmin]:received a test request message\n");
	}
	else if (strncmp(msgType, "2", 1) == 0)		//CME Globex发来的重发请求消息
	{
		char cPossDupFlag = pHeader->GetFieldValueDefault(FIELD::PossDupFlag, "E")[0];
		if (cPossDupFlag == 'Y')
		{
			//TODO:不回应重复的重发请求，暂无实现方法
			return 0;
		}
		//WriteLog(LOG_INFO, "[Worker::FromAdmin]:received a resend request message\n");
	}
	else if (strncmp(msgType, "3", 1) == 0)		//CME Globex发来的会话层拒绝消息
	{
		//WriteLog(LOG_INFO, "[Worker::FromAdmin]:received a session level reject message\n");
	}
	else if (strncmp(msgType, "4", 1) == 0)		//CME Globex发来的Sequence Reset消息
	{
		//WriteLog(LOG_INFO, "[Worker::FromAdmin]:received a Sequence Reset message\n");
	}
	else if (strncmp(msgType, "j", 1) == 0)		//CME Globex发来的应用层拒绝消息
	{
		//WriteLog(LOG_INFO, "[Worker::FromAdmin]:received a business level reject message\n");
	}
	else
	{
		//WriteLog(LOG_DEBUG, "[Worker::FromAdmin]Unhandle message. Message type: %s\n", msgType);
	}
	return 0;
}


int FUNCTION_CALL_MODE Worker::FromApp( const IMessage *lpMsg , const ISessionID *lpSessionID )
{
	const char* msgType = lpMsg->GetMessageType();
	HSFixHeader* pHeader = lpMsg->GetHeader();
	//strcpy(m_lastSeqNumReceived, pHeader->GetFieldValue(FIELD::MsgSeqNum));

	if(strncmp(msgType, "8", 1) == 0)  // Execution Report - Order Creation, Cancel or Modify
	{
		//WriteLog(LOG_INFO, "[Worker::FromApp]:Execution Report received\n");
		ExecReport(lpMsg);
	}
	else if(strncmp(msgType, "9", 1) == 0)  // Order Cancel Reject (tag 35-MsgType=9)
	{
		//WriteLog(LOG_INFO, "[Worker::FromApp]:Order Cancel Reject received\n");
		//CancelReject(lpMsg);
	}
	else if (strncmp(msgType, "b", 1) == 0)
	{
		//WriteLog(LOG_INFO, "[Worker::FromApp]:Quote Acknowledgment received\n");
	}
	else
	{
		//WriteLog(LOG_DEBUG, "[Worker::FromApp]Unhandle message. Message type: %c\n", msgType);
	}
	return 0;
}


