#include "StdAfx.h"
#include "Worker.h"
#include "mfc.h"
#include <io.h>
#include <direct.h>

Worker::Worker(void)
{
	m_pTradeSession = NULL;
	m_hEventReadyToTrade = CreateEvent(NULL, TRUE, FALSE, NULL);
	InitializeCriticalSection(&m_OrderLock);
	m_pfAuditTrail = fopen("Audit Trail.txt","w+");
	//m_MessageLinkID = 0;
}

Worker::~Worker(void)
{
	if (m_hEventReadyToTrade)
		CloseHandle(m_hEventReadyToTrade);
	DeleteCriticalSection(&m_OrderLock);
	fclose(m_pfAuditTrail);
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
	TCHAR cfgPath[MAX_PATH];
	GetModuleFileName(NULL, cfgPath, MAX_PATH);
	strcpy(strrchr(cfgPath, '\\'), "\\FIX_CME.ini");
	//strcpy(strrchr(cfgPath, '\\'), "\\CLIENT.CFG");
	WriteLog(LOG_INFO, _T("%s"), cfgPath);
	/*
	// 创建会话工厂
	ISessionFactory* lpSessionFactory=CreateSessionFactory();
	if(!lpSessionFactory)
	{
	WriteLog(LOG_ERROR, "CreateSessionFactory() error...exit\n");
	return FALSE;
	}
	// 初始化配置文件中配置的所有会话
	int ret = lpSessionFactory->InitSessions(cfgPath, this, 10);
	if(ret) {
	WriteLog(LOG_ERROR, "InitSessions() error...exit\n");
	return FALSE;
	}
	*/
	//启动HSfixEngine
	if(EngnInit((char*)cfgPath, this) != 0)
	{
		WriteLog(LOG_ERROR, _T("%s"), cfgPath);
		return FALSE;
	}

	DWORD dwWaitResult;
	dwWaitResult = WaitForSingleObject(m_hEventReadyToTrade, 20000);
	if ( dwWaitResult == WAIT_OBJECT_0 )	//登录成功
	{
		WriteLog(LOG_INFO, _T("[Trade]: Start"));
		//登录后发起历史委托查询
		g_worker.QueryOrderStatus();

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

void Worker::QueryOrderStatus()
{
	IMessage* pMsg = CreateMessage("FIX.4.2", "AF");
	HSFixMsgBody* pBody = pMsg->GetMsgBody();

	//MassStatusReqID	Unique identifier for Order Mass Status Request as assigned by the client system.
	//保证唯一性即可，设为YYYYMMDD-HH:MM:SS.sss
	SYSTEMTIME st;
	GetSystemTime(&st);
	char szTimeStamp[21] = {0};
	sprintf(szTimeStamp, "%04u%02u%02u-%02u:%02u:%02u:%03u", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
	pBody->SetFieldValue(FIELD::MassStatusReqID, szTimeStamp);

	//MassStatusReqType	1=Instrument	3=Instrument Group	7=All Orders		100=Market Segment 
	//查询全部订单的状态
	pBody->SetFieldValue(FIELD::MassStatusReqType, "7");

	//TransactTime	UTC format YYYYMMDD-HH:MM:SS.sss
	pBody->SetFieldValue(FIELD::TransactTime, szTimeStamp);

	//ManualOrderIndicator
	pBody->SetFieldValue(FIELD::ManualOrderIndicator, "Y");

	//发送消息
	if(m_pTradeSession)
	{
		int iRet = SendMessageByID( pMsg, m_pTradeSession );
		if(iRet != 0)
		{
			AfxMessageBox(_T("Send Fix Message Fail"));
		}
		DestroyMessage( pMsg );
	}
	else
	{
		AfxMessageBox(_T("Fix Session not initialized"));
	}
}

int Worker::EnterOrder(ORDER& order)
{
	//TODO:
	//ORDER中可能信息不足，需要从合约列表中获取完整信息
	strcpy(order.MsgType, "D");

	IMessage* pMsg = CreateMessage("FIX.4.2", "D");
	HSFixHeader *pHeader = pMsg->GetHeader();
	HSFixMsgBody* pBody = pMsg->GetMsgBody();

	//TODO:暂时不清楚要放什么字段。Unique account identifier. 
	pBody->SetFieldValue(FIELD::Account, _T("Shengsd"));

	SYSTEMTIME st;
	GetSystemTime(&st);
	_stprintf_s(order.ClOrdID, _countof(order.ClOrdID), _T("%02d%02d%02d%03d"), st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
	//本地单号，在这里用HHMMSSsss表示
	pBody->SetFieldValue(FIELD::ClOrdID, order.ClOrdID);

	strcpy(order.CorrelationClOrdID, order.ClOrdID);
	//原始本地单号
	pBody->SetFieldValue(FIELD::CorrelationClOrdID, order.ClOrdID);

	//Order submitted for automated matching on CME Globex.
	pBody->SetFieldValue(FIELD::HandlInst, "1");

	//下单数量
	pBody->SetFieldValue(FIELD::OrderQty, order.OrderQty);

	//下单类型
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

	//买卖方向
	pBody->SetFieldValue(FIELD::Side, order.Side);

	//Product Code 行情接口里是Asset
	pBody->SetFieldValue(FIELD::Symbol, order.Symbol);

	//订单有效期
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

	//下单时间 (UTC format YYYYMMDD-HH:MM:SS.sss)
	TCHAR szTransactTime[64];
	_stprintf_s(szTransactTime, _countof(szTransactTime), _T("%d%02d%02d-%02d:%02d:%02d.%03d"), st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
	pBody->SetFieldValue(FIELD::TransactTime, szTransactTime); 

	//手工下单 Y=manual N=automated
	pBody->SetFieldValue(FIELD::ManualOrderIndicator, _T("Y")); 

	//CME合约主键，行情接口里是Symbol
	pBody->SetFieldValue(FIELD::SecurityDesc, order.SecurityDesc);

	//期货还是期权 FUT=Future OPT=Option
	pBody->SetFieldValue(FIELD::SecurityType, order.SecurityType);

	//The type of business conducted. 0=Customer 1=Firm 
	pBody->SetFieldValue(FIELD::CustomerOrFirm, _T("1"));

	//MaxShow 冰山单里的最大显示数量，这里非0才添加该字段
	//Not available for CME Interest Rate Optons
	if (atoi(order.MaxShow) != 0)
	{
		pBody->SetFieldValue(FIELD::MaxShow, order.MaxShow);
	}

	//CtiCode TODO: 不是很清楚有什么用
	pBody->SetFieldValue(FIELD::CtiCode, _T("1")); 



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
	strcpy(order.MsgType, "F");

	IMessage* pMsg = CreateMessage("FIX.4.2", "F");
	HSFixHeader *pHeader = pMsg->GetHeader();
	HSFixMsgBody* pBody = pMsg->GetMsgBody();

	//Unique account identifier. TODO:暂时不清楚
	pBody->SetFieldValue(FIELD::Account, _T("Shengsd"));

	//上次本地单号
	strcpy(order.OrigClOrdID, order.ClOrdID);
	pBody->SetFieldValue(FIELD::OrigClOrdID, order.OrigClOrdID);

	SYSTEMTIME st;
	GetSystemTime(&st);
	_stprintf_s(order.ClOrdID, _countof(order.ClOrdID), _T("%02d%02d%02d%03d"), st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
	//新的本地单号
	pBody->SetFieldValue(FIELD::ClOrdID, order.ClOrdID);

	//主场单号
	pBody->SetFieldValue(FIELD::OrderID, order.OrderID);

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
	pBody->SetFieldValue(FIELD::SecurityType, order.SecurityType);

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
		oldOrder.OrdStatus[0] = 'D';
		UpdateOrder(oldOrder);
	}
	else
	{
		WriteLog(LOG_ERROR, "[CancelOrder]: can't find oldOrder.");
	}

	//保存新订单
	order.OrdStatus[0] = 'P';
	AddOrder(order);

	return STATUS_SUCCESS;
}

int Worker::ReplaceOrder(ORDER& order)
{
	//发改单消息
	strcpy(order.MsgType, "G");

	IMessage* pMsg = CreateMessage("FIX.4.2", "G");
	HSFixHeader* pHeader = pMsg->GetHeader();
	HSFixMsgBody* pBody = pMsg->GetMsgBody();

	//Unique account identifier. TODO:暂时不清楚
	pBody->SetFieldValue(FIELD::Account, _T("Shengsd"));

	//上次本地单号
	strcpy(order.OrigClOrdID, order.ClOrdID);
	pBody->SetFieldValue(FIELD::OrigClOrdID, order.OrigClOrdID);

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

	//OrderType
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

	//买卖方向
	pBody->SetFieldValue(FIELD::Side, order.Side);

	//Symbol
	//pBody->SetFieldValue(FIELD::Symbol, order.Symbol);

	//订单有效期
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

	//ManualOrderIndicator
	pBody->SetFieldValue(FIELD::ManualOrderIndicator, "Y");

	//TransactTime (UTC format YYYYMMDD-HH:MM:SS.sss)
	TCHAR szTransactTime[64];
	_stprintf_s(szTransactTime, _countof(szTransactTime), _T("%d%02d%02d-%02d:%02d:%02d.%03d"), st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
	pBody->SetFieldValue(FIELD::TransactTime, szTransactTime); 

	//SecurityDesc
	pBody->SetFieldValue(FIELD::SecurityDesc, order.SecurityDesc);

	//SecurityType
	pBody->SetFieldValue(FIELD::SecurityType, order.SecurityType);

	//The type of business conducted. 0=Customer 1=Firm 
	pBody->SetFieldValue(FIELD::CustomerOrFirm, _T("1"));

	//MaxShow 冰山单里的最大显示数量，这里非0才添加该字段
	if (atoi(order.MaxShow) != 0)
	{
		pBody->SetFieldValue(FIELD::MaxShow, order.MaxShow);
	}

	//CtiCode TODO: 不是很清楚有什么用
	pBody->SetFieldValue(FIELD::CtiCode, _T("1")); 

	//原始本地单号
	pBody->SetFieldValue(FIELD::CorrelationClOrdID, order.CorrelationClOrdID);

	//Indicates whether the cancel/replace supports IFM.认证中没有选择支持
	pBody->SetFieldValue(FIELD::OFMOverride, "N");

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
		oldOrder.OrdStatus[0] = 'D';
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
	HSFixHeader* pHeader = pMsg->GetHeader();
	HSFixMsgBody* pBody = pMsg->GetMsgBody();

	//Audit Trail begin
	char szTargetCompID[10] = {0};
	char szSessionID[4] = {0};
	char szFirmID[4] = {0};
	strcpy(szTargetCompID, pHeader->GetFieldValueDefault(FIELD::TargetCompID, ""));
	strncpy(szSessionID, szTargetCompID, 3);//7
	strncpy(szFirmID, szTargetCompID+3, 3);//8
	//InterlockedIncrement(&m_MessageLinkID);//14

	if (m_pfAuditTrail)
	{
		fprintf(m_pfAuditTrail, ",");//1
		fprintf(m_pfAuditTrail, "%s,", pBody->GetFieldValueDefault(FIELD::TransactTime, ""));//2
		fprintf(m_pfAuditTrail, "FROM CME,");//3
		fprintf(m_pfAuditTrail, "%s,", pHeader->GetFieldValueDefault(FIELD::TargetSubID, ""));//4
		fprintf(m_pfAuditTrail, "%s,", pBody->GetFieldValueDefault(FIELD::SelfMatchPreventionID, ""));//5
		fprintf(m_pfAuditTrail, "%s,", pBody->GetFieldValueDefault(FIELD::Account, ""));//6
		fprintf(m_pfAuditTrail, "%s,", szSessionID);//7
		fprintf(m_pfAuditTrail, "%s,", szFirmID);//8
		fprintf(m_pfAuditTrail, "%s,", pBody->GetFieldValueDefault(FIELD::ManualOrderIndicator, ""));//9
		fprintf(m_pfAuditTrail, "%s/%s,", pHeader->GetFieldValueDefault(FIELD::MsgType, ""), pBody->GetFieldValueDefault(FIELD::OrdStatus, ""));//10
		fprintf(m_pfAuditTrail, ",,");//11,12
		fprintf(m_pfAuditTrail, "%s,", pBody->GetFieldValueDefault(FIELD::ExecID, ""));//13
		//fprintf(m_pfAuditTrail, "%l,", m_MessageLinkID);//14
		fprintf(m_pfAuditTrail, ",");//14
		fprintf(m_pfAuditTrail, "%s,", pBody->GetFieldValueDefault(FIELD::CorrelationClOrdID, ""));//15
		fprintf(m_pfAuditTrail, "%s,", pBody->GetFieldValueDefault(FIELD::SecondaryExecID, ""));//16
		fprintf(m_pfAuditTrail, "%s,", pBody->GetFieldValueDefault(FIELD::SecurityDesc, ""));//17
		fprintf(m_pfAuditTrail, "%s,", pHeader->GetFieldValueDefault(FIELD::SenderSubID, ""));//18
		fprintf(m_pfAuditTrail, "%s,", pBody->GetFieldValueDefault(FIELD::ClOrdID, ""));//19
		fprintf(m_pfAuditTrail, "%s,", pBody->GetFieldValueDefault(FIELD::OrderID, ""));//20
		fprintf(m_pfAuditTrail, "%s,", pBody->GetFieldValueDefault(FIELD::Side, ""));//21
		fprintf(m_pfAuditTrail, "%s,", pBody->GetFieldValueDefault(FIELD::Quantity, ""));//22
		fprintf(m_pfAuditTrail, "%s,", pBody->GetFieldValueDefault(FIELD::Price, ""));//23
		fprintf(m_pfAuditTrail, "%s,", pBody->GetFieldValueDefault(FIELD::StopPx, ""));//24
		fprintf(m_pfAuditTrail, "%s,", pBody->GetFieldValueDefault(FIELD::OrdType, ""));//25
		fprintf(m_pfAuditTrail, "%s,", pBody->GetFieldValueDefault(FIELD::TimeInForce, ""));//26
		fprintf(m_pfAuditTrail, ",");//27
		fprintf(m_pfAuditTrail, "%s,", pBody->GetFieldValueDefault(FIELD::MaxShow, ""));//28
		fprintf(m_pfAuditTrail, "%s,", pBody->GetFieldValueDefault(FIELD::MinQty, ""));//29
		fprintf(m_pfAuditTrail, ",");//30
		fprintf(m_pfAuditTrail, "%s,", pBody->GetFieldValueDefault(FIELD::LastPx, ""));//31
		fprintf(m_pfAuditTrail, "%s,", pBody->GetFieldValueDefault(FIELD::LastQty, ""));//32
		fprintf(m_pfAuditTrail, "%s,", pBody->GetFieldValueDefault(FIELD::CumQty, ""));//33
		fprintf(m_pfAuditTrail, "%s,", pBody->GetFieldValueDefault(FIELD::LeavesQty, ""));//34
		fprintf(m_pfAuditTrail, "%s,", pBody->GetFieldValueDefault(FIELD::AggressorIndicator, ""));//35
		fprintf(m_pfAuditTrail, "%s,", pBody->GetFieldValueDefault(FIELD::ExecRestatementReason, ""));//36
		fprintf(m_pfAuditTrail, "%s:%s,", pBody->GetFieldValueDefault(FIELD::OrdRejReason, ""), pBody->GetFieldValueDefault(FIELD::Text, ""));//37
		fprintf(m_pfAuditTrail, ",");//38
		fprintf(m_pfAuditTrail, "%s,", pBody->GetFieldValueDefault(FIELD::CrossID, ""));//39
		fprintf(m_pfAuditTrail, "\n");
	}
	//Audit Trail end

	strcpy(excReport.ExecType, pBody->GetFieldValueDefault(FIELD::ExecType, "") );//回报类型
	strcpy(excReport.OrdStatus, pBody->GetFieldValueDefault(FIELD::OrdStatus, "") );//订单状态
	strcpy(excReport.ClOrdID, pBody->GetFieldValueDefault(FIELD::ClOrdID, "") );
	strcpy(excReport.OrigClOrdID, pBody->GetFieldValueDefault(FIELD::OrigClOrdID, "") );
	strcpy(excReport.CorrelationClOrdID, pBody->GetFieldValueDefault(FIELD::CorrelationClOrdID, "") );
	strcpy(excReport.OrderID, pBody->GetFieldValueDefault(FIELD::OrderID, "") );
	strcpy(excReport.SecurityDesc, pBody->GetFieldValueDefault(FIELD::SecurityDesc, "") );
	strcpy(excReport.SecurityType, pBody->GetFieldValueDefault(FIELD::SecurityType, "") );
	strcpy(excReport.Side, pBody->GetFieldValueDefault(FIELD::Side, "") );
	strcpy(excReport.OrderQty, pBody->GetFieldValueDefault(FIELD::OrderQty, "") );
	strcpy(excReport.MinQty, pBody->GetFieldValueDefault(FIELD::MinQty, "") );
	strcpy(excReport.MaxShow, pBody->GetFieldValueDefault(FIELD::MaxShow, "") );
	strcpy(excReport.OrdType, pBody->GetFieldValueDefault(FIELD::OrdType, "") );
	strcpy(excReport.Price, pBody->GetFieldValueDefault(FIELD::Price, "") );
	strcpy(excReport.StopPx, pBody->GetFieldValueDefault(FIELD::StopPx, "") );
	strcpy(excReport.TimeInForce, pBody->GetFieldValueDefault(FIELD::TimeInForce, "") );
	strcpy(excReport.ExpireDate, pBody->GetFieldValueDefault(FIELD::ExpireDate, "") );
	strcpy(excReport.LastPx, pBody->GetFieldValueDefault(FIELD::LastPx, ""));
	strcpy(excReport.LastQty, pBody->GetFieldValueDefault(FIELD::LastQty, ""));
	strcpy(excReport.Text, pBody->GetFieldValueDefault(FIELD::Text, ""));

	//Order Status Request Acknowledgment
	//订单状态反馈
	if ( 'I' ==  excReport.ExecType[0])
	{
		//If the Mass Order Status Request is accepted, but no orders are found
		if ( 'U' == excReport.OrdStatus[0] )
		{
			WriteLog(LOG_INFO, _T("%s"), pBody->GetFieldValueDefault(FIELD::Text, ""));
			return ;
		}
		//如果要等到全部挂单信息都收到	可以用LastRptRequested字段， Y表示最后一条

		//提取订单信息
		ExtractOrderFromExcReport(order, excReport);

		//保存挂单信息
		AddOrder(order);
	}

	//其它都是订单反馈，成交回报
	//按本地单号找订单
	if ( FALSE == GetOrderByClOrderID(excReport.ClOrdID, order) )
	{
		//找不到订单报错，结束
		WriteLog(LOG_ERROR, _T("Can't find the order, ClOrdID:%s"), excReport.ClOrdID);
		return ;
	}

	//OrderInfoList只更新订单状态和主场单号
	order.OrdStatus[0] = excReport.OrdStatus[0];
	strcpy( order.OrderID, excReport.OrderID);
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
		WriteLog(LOG_INFO, _T("[ExecReport]: Rejected. ClOrdID=%s, Text=%s"), excReport.ClOrdID, excReport.Text);
		break;
	case 'H'://Trade Cancelled
		break;
	default:
		break;
	}
}

void Worker::CancelReject(const IMessage* pMsg)
{
	ORDER order = {0};
	EXCREPORT excReport = {0};
	HSFixHeader* pHeader = pMsg->GetHeader();
	HSFixMsgBody* pBody = pMsg->GetMsgBody();

	//Audit Trail begin
	char szTargetCompID[10] = {0};
	char szSessionID[4] = {0};
	char szFirmID[4] = {0};
	strcpy(szTargetCompID, pHeader->GetFieldValueDefault(FIELD::TargetCompID, ""));
	strncpy(szSessionID, szTargetCompID, 3);//7
	strncpy(szFirmID, szTargetCompID+3, 3);//8
	//InterlockedIncrement(&m_MessageLinkID);//14

	if (m_pfAuditTrail)
	{
		fprintf(m_pfAuditTrail, ",");//1
		fprintf(m_pfAuditTrail, "%s,", pBody->GetFieldValueDefault(FIELD::TransactTime, ""));//2
		fprintf(m_pfAuditTrail, "FROM CME,");//3
		fprintf(m_pfAuditTrail, "%s,", pHeader->GetFieldValueDefault(FIELD::TargetSubID, ""));//4
		fprintf(m_pfAuditTrail, ",");//5
		fprintf(m_pfAuditTrail, "%s,", pBody->GetFieldValueDefault(FIELD::Account, ""));//6
		fprintf(m_pfAuditTrail, "%s,", szSessionID);//7
		fprintf(m_pfAuditTrail, "%s,", szFirmID);//8
		fprintf(m_pfAuditTrail, "%s,", pBody->GetFieldValueDefault(FIELD::ManualOrderIndicator, ""));//9
		fprintf(m_pfAuditTrail, "%s,", pHeader->GetFieldValueDefault(FIELD::MsgType, ""));//10
		fprintf(m_pfAuditTrail, ",,");//11,12
		fprintf(m_pfAuditTrail, "%s,", pBody->GetFieldValueDefault(FIELD::ExecID, ""));//13
		//fprintf(m_pfAuditTrail, "%l,", m_MessageLinkID);//14
		fprintf(m_pfAuditTrail, ",");//14
		fprintf(m_pfAuditTrail, "%s,", pBody->GetFieldValueDefault(FIELD::CorrelationClOrdID, ""));//15
		fprintf(m_pfAuditTrail, ",,");//16,17
		fprintf(m_pfAuditTrail, "%s,", pHeader->GetFieldValueDefault(FIELD::SenderSubID, ""));//18
		fprintf(m_pfAuditTrail, "%s,", pBody->GetFieldValueDefault(FIELD::ClOrdID, ""));//19
		fprintf(m_pfAuditTrail, "%s,", pBody->GetFieldValueDefault(FIELD::OrderID, ""));//20
		fprintf(m_pfAuditTrail, ",,,,,,,,,,,,,,,,");//21~36
		fprintf(m_pfAuditTrail, "%s:%s,", pBody->GetFieldValueDefault(FIELD::OrdRejReason, ""), pBody->GetFieldValueDefault(FIELD::Text, ""));//37
		fprintf(m_pfAuditTrail, "\n");
	}
	//Audit Trail end

	//本地单号
	strcpy(excReport.ClOrdID, pBody->GetFieldValueDefault(FIELD::ClOrdID, "") );

	//按本地单号找订单
	if ( FALSE == GetOrderByClOrderID(excReport.ClOrdID, order) )
	{
		//找不到订单报错，结束
		WriteLog(LOG_ERROR, _T("Can't find the order, ClOrdID:%s"), excReport.ClOrdID);
		return ;
	}

	char cCancelRejResponseTo = pBody->GetFieldValueDefault(FIELD::CxlRejResponseTo, "E")[0];
	if (cCancelRejResponseTo == '1')
	{
		strcpy(order.OrdStatus, "A");
	}
	else if (cCancelRejResponseTo == '2')
	{
		strcpy(order.OrdStatus, "L");
	}

	UpdateOrder(order);

	WriteLog(LOG_INFO, "[Worker::CancelReject]: ClOrdID=%s, Text=%s", order.ClOrdID, pBody->GetFieldValueDefault(FIELD::Text, ""));
}

int Worker::Quote(ORDER& order)
{
	strcpy(order.MsgType, "R");

	IMessage* pMsg = CreateMessage("FIX.4.2", "R");
	HSFixHeader* pHeader = pMsg->GetHeader();
	HSFixMsgBody* pBody = pMsg->GetMsgBody();

	SYSTEMTIME st;
	GetSystemTime(&st);
	char szQuoteReqID[21] = {0};
	_stprintf_s(szQuoteReqID, _countof(szQuoteReqID), _T("%02d%02d%02d%03d"), st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
	//本地单号，在这里用HHMMSSsss表示
	pBody->SetFieldValue(FIELD::QuoteReqID, szQuoteReqID);

	//Must=1; only one request supported per message.
	pBody->SetFieldValue(FIELD::NoRelatedSym, "1");

	IGroup* pGroup =  pBody->SetGroup(FIELD::NoRelatedSym, FIELD::Symbol);
	if (pGroup)
	{
		IRecord* pRecord = pGroup->AddRecord();
		if (pRecord)
		{
			pRecord->SetFieldValue(FIELD::Symbol, order.Symbol);

			if (order.Side[0] == '1' || order.Side[0] == '2')
			{
				pRecord->SetFieldValue(FIELD::Side, order.Side);
				//When tag 54-Side = 1 (Buy) or 2 (Sell), this tag must be present and = 1 for tradable.
				pRecord->SetFieldValue(FIELD::QuoteType, "1");
				pRecord->SetFieldValue(FIELD::OrderQty, order.OrderQty);
			}
			else if (order.Side[0] == '8')//(Cross)
			{
				pRecord->SetFieldValue(FIELD::Side, order.Side);
			}

			//TransactTime (UTC format YYYYMMDD-HH:MM:SS.sss) 非必须
			//TCHAR szTransactTime[64];
			//_stprintf_s(szTransactTime, _countof(szTransactTime), _T("%d%02d%02d-%02d:%02d:%02d.%03d"), st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
			//pRecord->SetFieldValue(FIELD::TransactTime, szTransactTime); 

			pRecord->SetFieldValue(FIELD::SecurityDesc, order.SecurityDesc);

			pRecord->SetFieldValue(FIELD::SecurityType, order.SecurityType);
		}
	}

	//ManualOrderIndicator Y=manual N=automated
	pBody->SetFieldValue(FIELD::ManualOrderIndicator, _T("Y")); 

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

	WriteLog(LOG_INFO, "[Worker::Quote]: Quote Sent");
	return STATUS_SUCCESS;
}

void Worker::QuoteAck(const IMessage* pMsg)
{
	HSFixHeader* pHeader = pMsg->GetHeader();
	HSFixMsgBody* pBody = pMsg->GetMsgBody();

	//Audit Trail begin
	char szTargetCompID[10] = {0};
	char szSessionID[4] = {0};
	char szFirmID[4] = {0};
	strcpy(szTargetCompID, pHeader->GetFieldValueDefault(FIELD::TargetCompID, ""));
	strncpy(szSessionID, szTargetCompID, 3);//7
	strncpy(szFirmID, szTargetCompID+3, 3);//8
	//InterlockedIncrement(&m_MessageLinkID);//14

	if (m_pfAuditTrail)
	{
		fprintf(m_pfAuditTrail, ",");//1
		fprintf(m_pfAuditTrail, "%s,", pHeader->GetFieldValueDefault(FIELD::SendingTime, ""));//2
		fprintf(m_pfAuditTrail, "FROM CME,");//3
		fprintf(m_pfAuditTrail, "%s,", pHeader->GetFieldValueDefault(FIELD::TargetSubID, ""));//4
		fprintf(m_pfAuditTrail, ",,");//5,6
		fprintf(m_pfAuditTrail, "%s,", szSessionID);//7
		fprintf(m_pfAuditTrail, "%s,", szFirmID);//8
		fprintf(m_pfAuditTrail, "%s,", pBody->GetFieldValueDefault(FIELD::ManualOrderIndicator, ""));//9
		fprintf(m_pfAuditTrail, "%s/%s,", pHeader->GetFieldValueDefault(FIELD::MsgType, ""), pBody->GetFieldValueDefault(FIELD::QuoteStatus, ""));//10
		fprintf(m_pfAuditTrail, ",,,");//11,12,13
		//fprintf(m_pfAuditTrail, "%l,", m_MessageLinkID);//14
		fprintf(m_pfAuditTrail, ",");//14
		fprintf(m_pfAuditTrail, ",,,");//15,16,17
		fprintf(m_pfAuditTrail, "%s,", pHeader->GetFieldValueDefault(FIELD::SenderSubID, ""));//18
		fprintf(m_pfAuditTrail, ",,,,,,,,,,,,,,,,,,");//19~36
		fprintf(m_pfAuditTrail, "%s,", pBody->GetFieldValueDefault(FIELD::QuoteRejectReason, ""));//37
		fprintf(m_pfAuditTrail, ",,");//38,39
		fprintf(m_pfAuditTrail, "%s,", pBody->GetFieldValueDefault(FIELD::QuoteReqID, ""));//40
		fprintf(m_pfAuditTrail, "\n");
	}
	//Audit Trail end
}

void Worker::AddOrder(ORDER& order)
{
	//订单以主场单号为主键，必须确保有主场单号
	//if ( 0 == strcmp(order.OrderID, "") )
	//return ;

	//保存
	EnterCriticalSection(&m_OrderLock);
	m_mapClOrderIDToOrder[order.ClOrdID] = order;
	LeaveCriticalSection(&m_OrderLock);

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
	EnterCriticalSection(&m_OrderLock);
	m_mapClOrderIDToOrder[order.ClOrdID] = order;
	LeaveCriticalSection(&m_OrderLock);

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
	case 'D'://被改、撤
		m_pDlg->m_lvOrderInfoList.SetItemText(nIndex, OrderInfo_Column_Status, _T("Disabled"));
		break;
	case '0'://Order Creation
		m_pDlg->m_lvOrderInfoList.SetItemText(nIndex, OrderInfo_Column_Status, _T("New"));
		break;
	case '4'://Order Cancel
		m_pDlg->m_lvOrderInfoList.SetItemText(nIndex, OrderInfo_Column_Status, _T("Cancelled"));
		break;
	case '5'://Order Modify
		m_pDlg->m_lvOrderInfoList.SetItemText(nIndex, OrderInfo_Column_Status, _T("Modified"));
		break;
	case '1'://Partial fill Notice
		m_pDlg->m_lvOrderInfoList.SetItemText(nIndex, OrderInfo_Column_Status, _T("Partial filled"));
		break;
	case '2'://Complete fill Notice
		m_pDlg->m_lvOrderInfoList.SetItemText(nIndex, OrderInfo_Column_Status, _T("Complete filled"));
		break;
	case 'C'://Order Elimination
		m_pDlg->m_lvOrderInfoList.SetItemText(nIndex, OrderInfo_Column_Status, _T("Expired"));
		break;
	case '8'://Rejected
		m_pDlg->m_lvOrderInfoList.SetItemText(nIndex, OrderInfo_Column_Status, _T("Rejected"));
		break;
	case 'H'://Trade Cancel
		m_pDlg->m_lvOrderInfoList.SetItemText(nIndex, OrderInfo_Column_Status, _T("Trade Cancelled"));
		break;
	case 'A'://Cancel Reject
		m_pDlg->m_lvOrderInfoList.SetItemText(nIndex, OrderInfo_Column_Status, _T("Cancel Reject"));
		break;
	case 'L'://Alter Reject
		m_pDlg->m_lvOrderInfoList.SetItemText(nIndex, OrderInfo_Column_Status, _T("Alter Reject"));
		break;
	default: 
		WriteLog(LOG_ERROR, _T("[UpdateOrderInfoList]: OrdStatus=%s"), order.OrdStatus);
		return;
	}
}

void Worker::ExtractOrderFromExcReport( ORDER& order, const EXCREPORT& excReport )
{
	strcpy(order.ClOrdID, excReport.ClOrdID);
	strcpy(order.OrigClOrdID, excReport.OrigClOrdID);
	strcpy(order.CorrelationClOrdID, excReport.CorrelationClOrdID);
	strcpy(order.OrdStatus, excReport.OrdStatus);
	strcpy(order.OrderID, excReport.OrderID);
	strcpy(order.SecurityDesc, excReport.SecurityDesc);
	strcpy(order.Side, excReport.Side);
	strcpy(order.OrderQty, excReport.OrderQty);
	strcpy(order.MinQty, excReport.MinQty);
	strcpy(order.MaxShow, excReport.MaxShow);
	strcpy(order.OrdType, excReport.OrdType);
	strcpy(order.Price, excReport.Price);
	strcpy(order.StopPx, excReport.StopPx);
	strcpy(order.TimeInForce, excReport.TimeInForce);
	strcpy(order.ExpireDate, excReport.ExpireDate);
	strcpy(order.SecurityType, excReport.SecurityType);
}

BOOL Worker::GetOrderByClOrderID(const CString csClOrderID, ORDER& order)
{
	EnterCriticalSection(&m_OrderLock);
	MapClOrderIDToORDER::iterator i = m_mapClOrderIDToOrder.find(csClOrderID);
	if (i != m_mapClOrderIDToOrder.end())
	{
		order = i->second;
		LeaveCriticalSection(&m_OrderLock);
		return TRUE;
	}
	else
	{
		LeaveCriticalSection(&m_OrderLock);
		return FALSE; 
	}
}


UINT Worker::startMktDt()
{
	WriteLog(LOG_INFO, "MDP3.0 Engine Starting, please wait...");
	//WriteLog(LOG_ERROR, "%s", szConfigPath);
	if (_access(".\\CME", 0) != 0)
	{
		_mkdir(".\\CME");
	}
	m_fLog.open(".\\CME\\mfc.log", std::ios::out | std::ios::binary | std::ios::trunc);
	if (!m_fLog.is_open())
	{
		WriteLog(LOG_ERROR, "Open mfc.log failed\n");
		return -1;
	}

	ConfigStruct configStruct;
	GetCurrentDirectory(128, configStruct.configFile);
	GetCurrentDirectory(128, configStruct.templateFile);
	sprintf(configStruct.configFile, "%s\\config.xml", configStruct.configFile);//"..\\Release\\config.xml";////////////////argv[ 1 ];$(TargetDir)\\config.xml 
	sprintf(configStruct.templateFile, "%s\\templates_FixBinary.sbeir", configStruct.templateFile);//"..\\Release\\templates_FixBinary.sbeir";////////////////argv[ 2 ];$(TargetDir)\\templates_FixBinary.sbeir
	strcpy(configStruct.userName, "CME");
	strcpy(configStruct.passWord, "CME");
	strcpy(configStruct.localInterface, "172.17.120.92");//"10.25.1.148";

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

UINT Worker::stopMktDt()
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
	/**************************************************新增合约**************************************************/
	//先获取以下关键字段

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

	//交易中用到
	//SecurityDesc<=>Symbol
	//Symbol<=>SecurityGroup
	if (pField = pFieldMap->getField(FIELD::SecurityGroup))
	{
		pField->getArray(0, inst.SecurityGroup, 0, pField->length());
	}
	//SecurityType
	if (pField = pFieldMap->getField(FIELD::SecurityType))
	{
		pField->getArray(0, inst.SecurityType, 0, pField->length());
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

	m_fLog << "[OnUpdate]: SecurityID=" << inst.SecurityID << ", Symbol=" << inst.Symbol << ", SecurityExchange=" << inst.SecurityExchange
		<< ", Asset=" << inst.Asset << ", SecurityType=" << inst.SecurityType << ", SecurityTradingStatus=" << inst.SecurityTradingStatus << ", Action=" << action << ", " << std::endl;
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
	return 1;
}

void Worker::onMarketData(MDPFieldMap* pMDPFieldMap, const int templateID)
{
	switch (templateID)
	{
	case 4://ChannelReset4
		m_fLog << "[onMarketData]: Received ChannelReset4" << std::endl;
		ChannelReset(pMDPFieldMap);
		break;
	case 12://AdminHeartbeat12
		m_fLog << "[onMarketData]: Received AdminHeartbeat12" << std::endl;
		break;
	case 27://MDInstrumentDefinitionFuture27
		m_fLog << "[onMarketData]: Received MDInstrumentDefinitionFuture27" << std::endl;
		OnUpdate(pMDPFieldMap);
		break;
	case 29://MDInstrumentDefinitionSpread29
		m_fLog << "[onMarketData]: Received MDInstrumentDefinitionSpread29" << std::endl;
		OnUpdate(pMDPFieldMap);
		break;
	case 30://SecurityStatus30
		m_fLog << "[onMarketData]: Received SecurityStatus30" << std::endl;
		SecurityStatus(pMDPFieldMap);
		break;
	case 41://MDInstrumentDefinitionOption41
		m_fLog << "[onMarketData]: Received MDInstrumentDefinitionOption41" << std::endl;
		OnUpdate(pMDPFieldMap);
		break;
	case 32://MDIncrementalRefreshBook32
		m_fLog << "[onMarketData]: Received MDIncrementalRefreshBook32" << std::endl;
		UpdateBook(pMDPFieldMap);
		break;
	case 33://MDIncrementalRefreshDailyStatistics33
		m_fLog << "[onMarketData]: Received MDIncrementalRefreshDailyStatistics33" << std::endl;
		UpdateDailyStatistics(pMDPFieldMap);
		break;
	case 34://MDIncrementalRefreshLimitsBanding34
		m_fLog << "[onMarketData]: Received MDIncrementalRefreshLimitsBanding34" << std::endl;
		break;
	case 35://MDIncrementalRefreshSessionStatistics35
		m_fLog << "[onMarketData]: Received MDIncrementalRefreshSessionStatistics35" << std::endl;
		UpdateSessionStatistics(pMDPFieldMap);
		break;
	case 36://MDIncrementalRefreshTrade36
		m_fLog << "[onMarketData]: Received MDIncrementalRefreshTrade36" << std::endl;
		UpdateTradeSummary(pMDPFieldMap);
		break;
	case 37://MDIncrementalRefreshVolume37
		m_fLog << "[onMarketData]: Received MDIncrementalRefreshVolume37" << std::endl;
		UpdateVolume(pMDPFieldMap);
		break;
	case 38://SnapshotFullRefresh38
		m_fLog << "[onMarketData]: Received SnapshotFullRefresh38" << std::endl;
		SnapShot(pMDPFieldMap);
		break;
	case 39://QuoteRequest39
		m_fLog << "[onMarketData]: Received QuoteRequest39" << std::endl;
		break;
	case 42://MDIncrementalRefreshTradeSummary42
		m_fLog << "[onMarketData]: Received MDIncrementalRefreshTradeSummary42" << std::endl;
		UpdateTradeSummary(pMDPFieldMap);
		break;
	default:
		m_fLog << "[onMarketData]: Received Unknown Message ID:" << templateID << std::endl;
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
	MktDtItem* pItem = NULL;

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
	MapIntToSetInt::iterator it = m_mapApplID2SecurityIDs.find(inst.ApplID); 
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

	MapIntToMktDtItemPtr::const_iterator iter = m_mapSecurityIDToMktDt.find(nSecurityID);
	if(iter != m_mapSecurityIDToMktDt.end())
	{
		pItem = iter->second;
	}
	else
	{
		pItem = new MktDtItem;
		memset(pItem, 0, sizeof(MktDtItem));
		m_mapSecurityIDToMktDt[nSecurityID] = pItem;
		pItem->securityID = nSecurityID;
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
		pItem->cMarketStatus = (int)pField->getUInt();
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
		pItem->nTimeStamp = timeinfo->tm_hour * 10000000 + timeinfo->tm_min * 100000 + timeinfo->tm_sec * 1000 + mSec;
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
				LevelChange(pItem->bidPrice, nLevel-1, inst.GBXMarketDepth, dMDEntryPx);
				LevelChange(pItem->bidVolume, nLevel-1, inst.GBXMarketDepth, nQty);
				break;
			case '1':// Offer
				LevelChange(pItem->askPrice, nLevel-1, inst.GBXMarketDepth, dMDEntryPx);
				LevelChange(pItem->askVolume, nLevel-1, inst.GBXMarketDepth, nQty);
				break;
			case 'E':// Implied Bid
				LevelChange(pItem->impliedBid, nLevel-1, inst.GBIMarketDepth, dMDEntryPx);
				LevelChange(pItem->impliedBidVol, nLevel-1, inst.GBIMarketDepth, nQty);
				break;
			case 'F':// Implied Offer
				LevelChange(pItem->impliedAsk, nLevel-1, inst.GBIMarketDepth, dMDEntryPx);
				LevelChange(pItem->impliedAskVol, nLevel-1, inst.GBIMarketDepth, nQty);
				break;
			case '2':// Trade
				pItem->last = dMDEntryPx;
				pItem->lastVolume = nQty;
				break;
			case '4':// Opening Price
				if (pField = pFieldMapInGroup->getField(FIELD::OpenCloseSettlFlag))
				{
					int nOpenCloseSettlFlag = (int)pField->getUInt();
					if (nOpenCloseSettlFlag == 0)//Daily Open Price
						pItem->open = dMDEntryPx;
				}
				break;
			case '6':// Settlement Price
				if (pField = pFieldMapInGroup->getField(FIELD::SettlPriceType))
				{
					int uSettlPriceType = (int)pField->getUInt();
					if (uSettlPriceType == 3)
						pItem->prevSettlementPrice = dMDEntryPx;
				}
				break;
			case '7':// Session High Trade Price
				pItem->high = dMDEntryPx;
				break;
			case '8':// Session Low Trade Price
				pItem->low = dMDEntryPx;
				break;
			case 'N':// Session High Bid
				break;
			case 'O':// Session Low Offer
				break;
			case 'B':// Cleared Trade Volume
				break;
			case 'C':// Open Interest
				pItem->bearVolume = nQty;
				break;
			case 'W':// Fixing Price
				break;
			case 'e':// Electronic Volume
				pItem->tolVolume = nQty;
				break;
			default:
				break;
			}
		}
	}

	//WriteLog(LOG_INFO, "[SnapShot]:push quote security ID:%d", qi->securityID);
	m_fLog << "[SnapShot]:push quote security ID:" << pItem->securityID << std::endl;
	PushMktDtItem(pItem);
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
			MktDtItem* qi = NULL;
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
			MapIntToMktDtItemPtr::const_iterator iter = m_mapSecurityIDToMktDt.find(nSecurityID);
			if(iter != m_mapSecurityIDToMktDt.end())
			{
				qi = iter->second;
			}
			else
			{
				m_fLog << "Can't find the existing QuoteItem, newing one" << std::endl;
				qi = new MktDtItem;
				memset(qi, 0, sizeof(MktDtItem));
				m_mapSecurityIDToMktDt[nSecurityID] = qi;
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
			PushMktDtItem(qi);
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
			MktDtItem* qi = NULL;
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
			MapIntToMktDtItemPtr::const_iterator iter = m_mapSecurityIDToMktDt.find(nSecurityID);
			if(iter != m_mapSecurityIDToMktDt.end())
			{
				qi = iter->second;
			}
			else
			{
				qi = new MktDtItem;
				memset(qi, 0, sizeof(MktDtItem));
				m_mapSecurityIDToMktDt[nSecurityID] = qi;
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
			PushMktDtItem(qi);
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
			MktDtItem* qi = NULL;
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

			//合约行情获取(创建)
			MapIntToMktDtItemPtr::const_iterator iter = m_mapSecurityIDToMktDt.find(nSecurityID);
			if(iter != m_mapSecurityIDToMktDt.end())
			{
				qi = iter->second;
			}
			else
			{
				qi = new MktDtItem;
				memset(qi, 0, sizeof(MktDtItem));
				m_mapSecurityIDToMktDt[nSecurityID] = qi;
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
			PushMktDtItem(qi);
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
			MktDtItem* pItem = NULL;
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
			// if (m_pdtMgr->GetInstrumentBySecurityID(nSecurityID, pInst))
			// {
			// 	continue;
			// }

			//合约行情获取(创建)
			MapIntToMktDtItemPtr::const_iterator iter = m_mapSecurityIDToMktDt.find(nSecurityID);
			if(iter != m_mapSecurityIDToMktDt.end())
			{
				pItem = iter->second;
			}
			else
			{
				pItem = new MktDtItem;
				memset(pItem, 0, sizeof(MktDtItem));
				m_mapSecurityIDToMktDt[nSecurityID] = pItem;
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
			if (pField = pFieldMapInGroup->getField(FIELD::MDEntrySize))
			{
				nQty = (int)pField->getInt();
			}

			if (cMDEntryType = 'e')
			{
				pItem->tolVolume = nQty;
			}

			//if (uMatchEventIndicator & 0x80)
			//{
			PushMktDtItem(pItem);
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
			MktDtItem* qi = NULL;
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
			MapIntToMktDtItemPtr::const_iterator iter = m_mapSecurityIDToMktDt.find(nSecurityID);
			if(iter != m_mapSecurityIDToMktDt.end())
			{
				qi = iter->second;
			}
			else
			{
				qi = new MktDtItem;
				memset(qi, 0, sizeof(MktDtItem));
				m_mapSecurityIDToMktDt[nSecurityID] = qi;
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
			PushMktDtItem(qi);
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
	MktDtItem* qi = NULL;
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
	MapIntToMktDtItemPtr::const_iterator iter = m_mapSecurityIDToMktDt.find(nSecurityID);
	if(iter != m_mapSecurityIDToMktDt.end())
	{
		qi = iter->second;
	}
	else
	{
		qi = new MktDtItem;
		memset(qi, 0, sizeof(MktDtItem));
		m_mapSecurityIDToMktDt[nSecurityID] = qi;
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
	PushMktDtItem(qi);
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
				MapIntToSetInt::iterator itMap = m_mapApplID2SecurityIDs.find(nApplID);
				if (itMap != m_mapApplID2SecurityIDs.end())
				{
					SetInt::iterator itSet = itMap->second.begin();
					while (itSet != itMap->second.end())
					{
						//Empty Book
						MapIntToMktDtItemPtr::iterator it = m_mapSecurityIDToMktDt.find(*itSet);
						if (it != m_mapSecurityIDToMktDt.end())
						{
							MktDtItem* qi = it->second;
							EmptyMktDtItem(qi);
							qi->nTimeStamp = nTimeStamp;
							//if (uMatchEventIndicator & 0x80)
							//{
							PushMktDtItem(qi);
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
	MapIntToMktDtItemPtr::iterator iter = m_mapSecurityIDToMktDt.find(SecurityID);
	if (iter != m_mapSecurityIDToMktDt.end())
	{
		PushMktDtItem(iter->second);
	}
	else
		m_fLog << "updateOrderBook  can't find it: "<<SecurityID <<"\n";

}

void Worker::PushMktDtItem( const MktDtItem* pItem )
{
	if (!pItem)
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
	CListCtrl& lvMktDtInfo = m_pDlg->m_lvMktDtInfoList;
	CListCtrl& lvOrderBook = m_pDlg->m_lvOrderBookList;
	BOOL bInTheList = FALSE;

	for (i = 0; i < lvMktDtInfo.GetItemCount(); i++)
	{
		if (pItem->securityID == (int)lvMktDtInfo.GetItemData(i))//Already in the list
		{
			nIndex = i;
			bInTheList = TRUE;
			break;
		}
	}

	if (!bInTheList)//Add new item
	{
		Instrument inst;
		nIndex = lvMktDtInfo.GetItemCount();
		if (GetInstrumentBySecurityID(pItem->securityID, inst))
		{
			WriteLog(LOG_WARNING, "[Worker::PushMktDtItem]: can't find the instrument by SecurityID=%d", pItem->securityID);
			return;
		}
		lvMktDtInfo.InsertItem(nIndex, inst.Symbol);
		lvMktDtInfo.SetItemData(nIndex, pItem->securityID);

		s.Format("%d", pItem->securityID);
		lvMktDtInfo.SetItemText(nIndex, MktDtInfo_Column_SecurityID, s);

		lvMktDtInfo.SetItemText(nIndex, MktDtInfo_Column_SecurityType, inst.SecurityType);

		lvMktDtInfo.SetItemText(nIndex, MktDtInfo_Column_Asset, inst.Asset);
	}

	//市场状态
	switch (pItem->cMarketStatus)
	{
	case 21:
		s.Format("%s", "Pre Open");
		break;
	case 15:
		s.Format("%s", "Opening");
		break;
	case 17:
		s.Format("%s", "Open");
		break;
	case 2:
		s.Format("%s", "Pause");
		break;
	case 18:
		s.Format("%s", "Close - Not Final");
		break;
	case 4:
		s.Format("%s", "Close - Final");
		break;
	case 26:
		s.Format("%s", "Post Close");
		break;
	case 20:
		s.Format("%s", "Unknown or Invalid");
		break;
	case 24:
		s.Format("%s", "Pre-Cross");
		break;
	case 25:
		s.Format("%s", "Cross");
		break;
	case 103:
		s.Format("%s", "No Change");
		break;
	default:
		s.Format("%s", "-");
		break;
	}
	lvMktDtInfo.SetItemText(nIndex, MktDtInfo_Column_Status, s);

	s.Format("%lf", pItem->last);
	lvMktDtInfo.SetItemText(nIndex, MktDtInfo_Column_Last, s);

	s.Format("%lf", pItem->open);
	lvMktDtInfo.SetItemText(nIndex, MktDtInfo_Column_Open, s);

	s.Format("%lf", pItem->high);
	lvMktDtInfo.SetItemText(nIndex, MktDtInfo_Column_High, s);

	s.Format("%lf", pItem->low);
	lvMktDtInfo.SetItemText(nIndex, MktDtInfo_Column_Low, s);

	if (pItem->securityID == m_pDlg->m_OrderBookSecurityID)
	{
		ConsolidatedBook conBook = {0};
		MergeBook(&conBook, pItem);
		CString s;
		for (int i = 0; i < 10; i++)
		{
			s.Format("%lf", conBook.bidPrice[i]);
			lvOrderBook.SetItemText(i, OrderBook_Column_BuyPrice, s);
			s.Format("%d", conBook.bidVolume[i]);
			lvOrderBook.SetItemText(i, OrderBook_Column_BuyQuantity, s);
			s.Format("%lf", conBook.askPrice[i]);
			lvOrderBook.SetItemText(i, OrderBook_Column_SellPrice, s);
			s.Format("%d", conBook.askVolume[i]);
			lvOrderBook.SetItemText(i, OrderBook_Column_SellQuantity, s);
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

void Worker::EmptyMktDtItem(MktDtItem* pItem)
{
	int i;
	for (i = 0; i < 10; i++)
	{
		pItem->askPrice[i] = 0;
		pItem->askVolume[i] = 0;
		pItem->bidPrice[i] = 0;
		pItem->bidVolume[i] = 0;
	}
	for (i = 0; i < 2; i++)
	{
		pItem->impliedAsk[i] = 0;
		pItem->impliedAskVol[i] = 0;
		pItem->impliedBid[i] = 0;
		pItem->impliedBidVol[i] = 0;
	}
	pItem->open = 0;
	pItem->high = 0;
	pItem->low = 0;
	pItem->last = 0;
	pItem->close = 0;
	pItem->lastVolume = 0;
	pItem->bearVolume = 0;
	pItem->preClose = 0;
	pItem->prevSettlementPrice = 0;
	pItem->tolVolume = 0;
	pItem->cMarketStatus = 0;
	return ;
}


//合并implied book 按价格排序，相同价格的数量相加
void Worker::MergeBook(ConsolidatedBook* pBook, const MktDtItem* pItem)
{
	int i = 0;//Implied Book Level
	int j = 0;//Multiple-Depth Book Level
	int nLevel;//Consolidated Book Level

	Instrument inst;
	if (GetInstrumentBySecurityID(pItem->securityID, inst) == -1)
		return ;


	for (nLevel = 0; nLevel < inst.GBXMarketDepth; nLevel++)
	{
		if (i < 2)
		{
			//impliedBid高且数量不为零，改成implied
			if ( pItem->impliedBid[i] > pItem->bidPrice[j] && pItem->impliedBidVol[i] > 0)
			{
				pBook->bidPrice[nLevel] = pItem->impliedBid[i];
				pBook->bidVolume[nLevel] = pItem->impliedBidVol[i];
				++i;
			}//相同且数量不为零，加上implied
			else if ( pItem->impliedBid[i] == pItem->bidPrice[j] && pItem->impliedBidVol[i] > 0)
			{
				pBook->bidPrice[nLevel] = pItem->impliedBid[i];
				pBook->bidVolume[nLevel] = pItem->impliedBidVol[i] + pItem->bidVolume[j];
				++i;
				++j;
			}
			else//直接复制
			{
				pBook->bidPrice[nLevel] = pItem->bidPrice[j];
				pBook->bidVolume[nLevel] = pItem->bidVolume[j];
				++j;
			}
		}
		else
		{
			pBook->bidPrice[nLevel] = pItem->bidPrice[j];
			pBook->bidVolume[nLevel] = pItem->bidVolume[j];
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
			if ( pItem->impliedAsk[i] < pItem->askPrice[j] && pItem->impliedAskVol[i] > 0)
			{
				pBook->askPrice[nLevel] = pItem->impliedAsk[i];
				pBook->askVolume[nLevel] = pItem->impliedAskVol[i];
				++i;
			}//相同且数量不为零，加上implied
			else if ( pItem->impliedAsk[i] == pItem->askPrice[j] && pItem->impliedAskVol[i] > 0)
			{
				pBook->askPrice[nLevel] = pItem->impliedAsk[i];
				pBook->askVolume[nLevel] = pItem->impliedAskVol[i] + pItem->askVolume[j];
				++i;
				++j;
			}
			else//直接复制
			{
				pBook->askPrice[nLevel] = pItem->askPrice[j];
				pBook->askVolume[nLevel] = pItem->askVolume[j];
				++j;
			}
		}
		else
		{
			pBook->askPrice[nLevel] = pItem->askPrice[j];
			pBook->askVolume[nLevel] = pItem->askVolume[j];
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
		//pBody->SetFieldValue(FIELD::ResetSeqNumFlag, "N");
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
	const char* msgType = lpMsg->GetMessageType();
	HSFixHeader *pHeader = lpMsg->GetHeader();
	HSFixMsgBody *pBody = lpMsg->GetMsgBody();
	pHeader->SetFieldValue(FIELD::SenderSubID, "HSUFOs7");
	pHeader->SetFieldValue(FIELD::TargetSubID, "G");
	pHeader->SetFieldValue(FIELD::SenderLocationID, "CN");
	pHeader->SetFieldValue(FIELD::LastMsgSeqNumProcessed, _ltoa(pSession->getExpectedTargetNum() - 1, szLastMsgSeqNumProcessed, 10 ) );

	if (strncmp(msgType, "D", 1) == 0)
	{
		//Audit Trail
		char szSenderCompID[10] = {0};
		char szSessionID[4] = {0};
		char szFirmID[4] = {0};
		strcpy(szSenderCompID, pHeader->GetFieldValueDefault(FIELD::SenderCompID, ""));
		strncpy(szSessionID, szSenderCompID, 3);//7
		strncpy(szFirmID, szSenderCompID+3, 3);//8
		//InterlockedIncrement(&m_MessageLinkID);//14

		if (m_pfAuditTrail)
		{
			fprintf(m_pfAuditTrail, "%s,", pHeader->GetFieldValueDefault(FIELD::SendingTime, ""));//1
			fprintf(m_pfAuditTrail, ",");//2
			fprintf(m_pfAuditTrail, "TO CME,");//3
			fprintf(m_pfAuditTrail, "%s,", pHeader->GetFieldValueDefault(FIELD::SenderSubID, ""));//4
			fprintf(m_pfAuditTrail, "%s,", pBody->GetFieldValueDefault(FIELD::SelfMatchPreventionID, ""));//5
			fprintf(m_pfAuditTrail, "%s,", pBody->GetFieldValueDefault(FIELD::Account, ""));//6
			fprintf(m_pfAuditTrail, "%s,", szSessionID);//7
			fprintf(m_pfAuditTrail, "%s,", szFirmID);//8
			fprintf(m_pfAuditTrail, "%s,", pBody->GetFieldValueDefault(FIELD::ManualOrderIndicator, ""));//9
			fprintf(m_pfAuditTrail, "%s,", pHeader->GetFieldValueDefault(FIELD::MsgType, ""));//10
			fprintf(m_pfAuditTrail, "%s,", pBody->GetFieldValueDefault(FIELD::CtiCode, ""));//11
			fprintf(m_pfAuditTrail, "%s,", pBody->GetFieldValueDefault(FIELD::CustomerOrFirm, ""));//12
			fprintf(m_pfAuditTrail, ",");//13
			//fprintf(m_pfAuditTrail, "%l,", m_MessageLinkID);//14
			fprintf(m_pfAuditTrail, ",");//14
			fprintf(m_pfAuditTrail, "%s,", pBody->GetFieldValueDefault(FIELD::CorrelationClOrdID, ""));//15
			fprintf(m_pfAuditTrail, ",");//16
			fprintf(m_pfAuditTrail, "%s,", pBody->GetFieldValueDefault(FIELD::SecurityDesc, ""));//17
			fprintf(m_pfAuditTrail, "%s,", pHeader->GetFieldValueDefault(FIELD::TargetSubID, ""));//18
			fprintf(m_pfAuditTrail, "%s,", pBody->GetFieldValueDefault(FIELD::ClOrdID, ""));//19
			fprintf(m_pfAuditTrail, ",");//20
			fprintf(m_pfAuditTrail, "%s,", pBody->GetFieldValueDefault(FIELD::Side, ""));//21
			fprintf(m_pfAuditTrail, "%s,", pBody->GetFieldValueDefault(FIELD::Quantity, ""));//22
			fprintf(m_pfAuditTrail, "%s,", pBody->GetFieldValueDefault(FIELD::Price, ""));//23
			fprintf(m_pfAuditTrail, "%s,", pBody->GetFieldValueDefault(FIELD::StopPx, ""));//24
			fprintf(m_pfAuditTrail, "%s,", pBody->GetFieldValueDefault(FIELD::OrdType, ""));//25
			fprintf(m_pfAuditTrail, "%s,", pBody->GetFieldValueDefault(FIELD::TimeInForce, ""));//26
			fprintf(m_pfAuditTrail, ",");//27
			fprintf(m_pfAuditTrail, "%s,", pBody->GetFieldValueDefault(FIELD::MaxShow, ""));//28
			fprintf(m_pfAuditTrail, "%s,", pBody->GetFieldValueDefault(FIELD::MinQty, ""));//29
			fprintf(m_pfAuditTrail, "%s,", pHeader->GetFieldValueDefault(FIELD::SenderLocationID, ""));//30
			fprintf(m_pfAuditTrail, ",,,,,,,,");//31,32,33,34,35,36,37,38
			fprintf(m_pfAuditTrail, "%s,", pBody->GetFieldValueDefault(FIELD::CrossID, ""));//39
			fprintf(m_pfAuditTrail, "\n");
		}
	}
	else if (strncmp(msgType, "G", 1) == 0)
	{
		//Audit Trail
		char szSenderCompID[10] = {0};
		char szSessionID[4] = {0};
		char szFirmID[4] = {0};
		strcpy(szSenderCompID, pHeader->GetFieldValueDefault(FIELD::SenderCompID, ""));
		strncpy(szSessionID, szSenderCompID, 3);//7
		strncpy(szFirmID, szSenderCompID+3, 3);//8
		//InterlockedIncrement(&m_MessageLinkID);//14

		if (m_pfAuditTrail)
		{
			fprintf(m_pfAuditTrail, "%s,", pHeader->GetFieldValueDefault(FIELD::SendingTime, ""));//1
			fprintf(m_pfAuditTrail, ",");//2
			fprintf(m_pfAuditTrail, "TO CME,");//3
			fprintf(m_pfAuditTrail, "%s,", pHeader->GetFieldValueDefault(FIELD::SenderSubID, ""));//4
			fprintf(m_pfAuditTrail, ",");//5
			fprintf(m_pfAuditTrail, "%s,", pBody->GetFieldValueDefault(FIELD::Account, ""));//6
			fprintf(m_pfAuditTrail, "%s,", szSessionID);//7
			fprintf(m_pfAuditTrail, "%s,", szFirmID);//8
			fprintf(m_pfAuditTrail, "%s,", pBody->GetFieldValueDefault(FIELD::ManualOrderIndicator, ""));//9
			fprintf(m_pfAuditTrail, "%s,", pHeader->GetFieldValueDefault(FIELD::MsgType, ""));//10
			fprintf(m_pfAuditTrail, "%s,", pBody->GetFieldValueDefault(FIELD::CtiCode, ""));//11
			fprintf(m_pfAuditTrail, "%s,", pBody->GetFieldValueDefault(FIELD::CustomerOrFirm, ""));//12
			fprintf(m_pfAuditTrail, ",");//13
			//fprintf(m_pfAuditTrail, "%l,", m_MessageLinkID);//14
			fprintf(m_pfAuditTrail, ",");//14
			fprintf(m_pfAuditTrail, "%s,", pBody->GetFieldValueDefault(FIELD::CorrelationClOrdID, ""));//15
			fprintf(m_pfAuditTrail, ",");//16
			fprintf(m_pfAuditTrail, "%s,", pBody->GetFieldValueDefault(FIELD::SecurityDesc, ""));//17
			fprintf(m_pfAuditTrail, "%s,", pHeader->GetFieldValueDefault(FIELD::TargetSubID, ""));//18
			fprintf(m_pfAuditTrail, "%s,", pBody->GetFieldValueDefault(FIELD::ClOrdID, ""));//19
			fprintf(m_pfAuditTrail, "%s,", pBody->GetFieldValueDefault(FIELD::OrderID, ""));//20
			fprintf(m_pfAuditTrail, "%s,", pBody->GetFieldValueDefault(FIELD::Side, ""));//21
			fprintf(m_pfAuditTrail, "%s,", pBody->GetFieldValueDefault(FIELD::Quantity, ""));//22
			fprintf(m_pfAuditTrail, "%s,", pBody->GetFieldValueDefault(FIELD::Price, ""));//23
			fprintf(m_pfAuditTrail, "%s,", pBody->GetFieldValueDefault(FIELD::StopPx, ""));//24
			fprintf(m_pfAuditTrail, "%s,", pBody->GetFieldValueDefault(FIELD::OrdType, ""));//25
			fprintf(m_pfAuditTrail, "%s,", pBody->GetFieldValueDefault(FIELD::TimeInForce, ""));//26
			fprintf(m_pfAuditTrail, "%s,", pBody->GetFieldValueDefault(FIELD::OFMOverride, ""));//27
			fprintf(m_pfAuditTrail, "%s,", pBody->GetFieldValueDefault(FIELD::MaxShow, ""));//28
			fprintf(m_pfAuditTrail, "%s,", pBody->GetFieldValueDefault(FIELD::MinQty, ""));//29
			fprintf(m_pfAuditTrail, "%s,", pHeader->GetFieldValueDefault(FIELD::SenderLocationID, ""));//30
			fprintf(m_pfAuditTrail, "\n");
		}
	}
	else if (strncmp(msgType, "F", 1) == 0)
	{
		//Audit Trail
		char szSenderCompID[10] = {0};
		char szSessionID[4] = {0};
		char szFirmID[4] = {0};
		strcpy(szSenderCompID, pHeader->GetFieldValueDefault(FIELD::SenderCompID, ""));
		strncpy(szSessionID, szSenderCompID, 3);//7
		strncpy(szFirmID, szSenderCompID+3, 3);//8
		//InterlockedIncrement(&m_MessageLinkID);//14

		if (m_pfAuditTrail)
		{
			fprintf(m_pfAuditTrail, "%s,", pHeader->GetFieldValueDefault(FIELD::SendingTime, ""));//1
			fprintf(m_pfAuditTrail, ",");//2
			fprintf(m_pfAuditTrail, "TO CME,");//3
			fprintf(m_pfAuditTrail, "%s,", pHeader->GetFieldValueDefault(FIELD::SenderSubID, ""));//4
			fprintf(m_pfAuditTrail, ",");//5
			fprintf(m_pfAuditTrail, "%s,", pBody->GetFieldValueDefault(FIELD::Account, ""));//6
			fprintf(m_pfAuditTrail, "%s,", szSessionID);//7
			fprintf(m_pfAuditTrail, "%s,", szFirmID);//8
			fprintf(m_pfAuditTrail, "%s,", pBody->GetFieldValueDefault(FIELD::ManualOrderIndicator, ""));//9
			fprintf(m_pfAuditTrail, "%s,", pHeader->GetFieldValueDefault(FIELD::MsgType, ""));//10
			fprintf(m_pfAuditTrail, ",,,");//11,12,13
			//fprintf(m_pfAuditTrail, "%l,", m_MessageLinkID);//14
			fprintf(m_pfAuditTrail, ",");//14
			fprintf(m_pfAuditTrail, "%s,", pBody->GetFieldValueDefault(FIELD::CorrelationClOrdID, ""));//15
			fprintf(m_pfAuditTrail, ",");//16
			fprintf(m_pfAuditTrail, "%s,", pBody->GetFieldValueDefault(FIELD::SecurityDesc, ""));//17
			fprintf(m_pfAuditTrail, "%s,", pHeader->GetFieldValueDefault(FIELD::TargetSubID, ""));//18
			fprintf(m_pfAuditTrail, "%s,", pBody->GetFieldValueDefault(FIELD::ClOrdID, ""));//19
			fprintf(m_pfAuditTrail, "%s,", pBody->GetFieldValueDefault(FIELD::OrderID, ""));//20
			fprintf(m_pfAuditTrail, "%s,", pBody->GetFieldValueDefault(FIELD::Side, ""));//21
			fprintf(m_pfAuditTrail, ",,,,,,,,");//22,23,24,25,26,27,28,29
			fprintf(m_pfAuditTrail, "%s,", pHeader->GetFieldValueDefault(FIELD::SenderLocationID, ""));//30
			fprintf(m_pfAuditTrail, "\n");
		}
	}
	else if (strncmp(msgType, "R", 1) == 0)
	{
		//Audit Trail
		char szSenderCompID[10] = {0};
		char szSessionID[4] = {0};
		char szFirmID[4] = {0};
		strcpy(szSenderCompID, pHeader->GetFieldValueDefault(FIELD::SenderCompID, ""));
		strncpy(szSessionID, szSenderCompID, 3);//7
		strncpy(szFirmID, szSenderCompID+3, 3);//8
		//InterlockedIncrement(&m_MessageLinkID);//14
		IGroup* pGroup = pBody->GetGroup(FIELD::NoRelatedSym);
		if (pGroup)
		{
			IRecord* pRecord = pGroup->GetRecord(0);
			if (pRecord && m_pfAuditTrail)
			{
				fprintf(m_pfAuditTrail, "%s,", pHeader->GetFieldValueDefault(FIELD::SendingTime, ""));//1
				fprintf(m_pfAuditTrail, ",");//2
				fprintf(m_pfAuditTrail, "TO CME,");//3
				fprintf(m_pfAuditTrail, "%s,", pHeader->GetFieldValueDefault(FIELD::SenderSubID, ""));//4
				fprintf(m_pfAuditTrail, "%s,", pBody->GetFieldValueDefault(FIELD::SelfMatchPreventionID, ""));//5
				fprintf(m_pfAuditTrail, "%s,", pBody->GetFieldValueDefault(FIELD::Account, ""));//6
				fprintf(m_pfAuditTrail, "%s,", szSessionID);//7
				fprintf(m_pfAuditTrail, "%s,", szFirmID);//8
				fprintf(m_pfAuditTrail, "%s,", pBody->GetFieldValueDefault(FIELD::ManualOrderIndicator, ""));//9
				fprintf(m_pfAuditTrail, "%s,", pHeader->GetFieldValueDefault(FIELD::MsgType, ""));//10
				fprintf(m_pfAuditTrail, ",,,");//11,12,13
				//fprintf(m_pfAuditTrail, "%l,", m_MessageLinkID);//14
				fprintf(m_pfAuditTrail, ",,,");//14,15,16
				fprintf(m_pfAuditTrail, "%s,", pRecord->GetFieldValueDefault(FIELD::SecurityDesc, ""));//17
				fprintf(m_pfAuditTrail, "%s,", pHeader->GetFieldValueDefault(FIELD::TargetSubID, ""));//18
				fprintf(m_pfAuditTrail, ",,");//19,20
				fprintf(m_pfAuditTrail, "%s,", pRecord->GetFieldValueDefault(FIELD::Side, ""));//21
				fprintf(m_pfAuditTrail, ",,,,,,,,");//22~29
				fprintf(m_pfAuditTrail, "%s,", pHeader->GetFieldValueDefault(FIELD::SenderLocationID, ""));//30
				fprintf(m_pfAuditTrail, ",,,,,,,,,");//31,32,33,34,35,36,37,38,39
				fprintf(m_pfAuditTrail, "%s,", pBody->GetFieldValueDefault(FIELD::QuoteReqID, ""));//40
				fprintf(m_pfAuditTrail, "\n");
			}
		}
	}
	return 0;
}

int FUNCTION_CALL_MODE Worker::FromAdmin( const IMessage * lpMsg , const ISessionID * lpSessionID )
{
	const char* msgType = lpMsg->GetMessageType();
	HSFixHeader* pHeader = lpMsg->GetHeader();
	HSFixMsgBody* pBody = lpMsg->GetMsgBody();
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
		WriteLog(LOG_INFO, "[Worker::FromAdmin]:received a session level reject message\n");

		//Audit Trail begin
		char szTargetCompID[10] = {0};
		char szSessionID[4] = {0};
		char szFirmID[4] = {0};
		strcpy(szTargetCompID, pHeader->GetFieldValueDefault(FIELD::TargetCompID, ""));
		strncpy(szSessionID, szTargetCompID, 3);//7
		strncpy(szFirmID, szTargetCompID+3, 3);//8
		//InterlockedIncrement(&m_MessageLinkID);//14

		if (m_pfAuditTrail)
		{
			fprintf(m_pfAuditTrail, ",");//1
			fprintf(m_pfAuditTrail, "%s,", pHeader->GetFieldValueDefault(FIELD::SendingTime, ""));//2
			fprintf(m_pfAuditTrail, "FROM CME,");//3
			fprintf(m_pfAuditTrail, "%s,", pHeader->GetFieldValueDefault(FIELD::TargetSubID, ""));//4
			fprintf(m_pfAuditTrail, ",,");//5,6
			fprintf(m_pfAuditTrail, "%s,", szSessionID);//7
			fprintf(m_pfAuditTrail, "%s,", szFirmID);//8
			fprintf(m_pfAuditTrail, "%s,", pBody->GetFieldValueDefault(FIELD::ManualOrderIndicator, ""));//9
			fprintf(m_pfAuditTrail, "%s,", pHeader->GetFieldValueDefault(FIELD::MsgType, ""));//10
			fprintf(m_pfAuditTrail, ",,,");//11,12,13
			//fprintf(m_pfAuditTrail, "%l,", m_MessageLinkID);//14
			fprintf(m_pfAuditTrail, ",");//14
			fprintf(m_pfAuditTrail, ",,,");//15,16,17
			fprintf(m_pfAuditTrail, "%s,", pHeader->GetFieldValueDefault(FIELD::SenderSubID, ""));//18
			fprintf(m_pfAuditTrail, ",,,,,,,,,,,,,,,,,,");//19~36
			fprintf(m_pfAuditTrail, "%s:%s,", pBody->GetFieldValueDefault(FIELD::OrdRejReason, ""), pBody->GetFieldValueDefault(FIELD::Text, ""));//37
			fprintf(m_pfAuditTrail, "\n");
		}
		//Audit Trail end
	}
	else if (strncmp(msgType, "4", 1) == 0)		//CME Globex发来的Sequence Reset消息
	{
		//WriteLog(LOG_INFO, "[Worker::FromAdmin]:received a Sequence Reset message\n");
	}
	else if (strncmp(msgType, "j", 1) == 0)		//CME Globex发来的应用层拒绝消息
	{
		WriteLog(LOG_INFO, "[Worker::FromAdmin]:received a business level reject message\n");
		//Audit Trail begin
		char szTargetCompID[10] = {0};
		char szSessionID[4] = {0};
		char szFirmID[4] = {0};
		strcpy(szTargetCompID, pHeader->GetFieldValueDefault(FIELD::TargetCompID, ""));
		strncpy(szSessionID, szTargetCompID, 3);//7
		strncpy(szFirmID, szTargetCompID+3, 3);//8
		//InterlockedIncrement(&m_MessageLinkID);//14

		if (m_pfAuditTrail)
		{
			fprintf(m_pfAuditTrail, ",");//1
			fprintf(m_pfAuditTrail, "%s,", pBody->GetFieldValueDefault(FIELD::TransactTime, ""));//2
			fprintf(m_pfAuditTrail, "FROM CME,");//3
			fprintf(m_pfAuditTrail, "%s,", pHeader->GetFieldValueDefault(FIELD::TargetSubID, ""));//4
			fprintf(m_pfAuditTrail, ",,");//5,6
			fprintf(m_pfAuditTrail, "%s,", szSessionID);//7
			fprintf(m_pfAuditTrail, "%s,", szFirmID);//8
			fprintf(m_pfAuditTrail, "%s,", pBody->GetFieldValueDefault(FIELD::ManualOrderIndicator, ""));//9
			fprintf(m_pfAuditTrail, "%s,", pHeader->GetFieldValueDefault(FIELD::MsgType, ""));//10
			fprintf(m_pfAuditTrail, ",,,");//11,12,13
			//fprintf(m_pfAuditTrail, "%l,", m_MessageLinkID);//14
			fprintf(m_pfAuditTrail, ",");//14
			fprintf(m_pfAuditTrail, ",,,");//15,16,17
			fprintf(m_pfAuditTrail, "%s,", pHeader->GetFieldValueDefault(FIELD::SenderSubID, ""));//18
			fprintf(m_pfAuditTrail, ",,,,,,,,,,,,,,,,,,");//19~36
			fprintf(m_pfAuditTrail, "%s:%s,", pBody->GetFieldValueDefault(FIELD::OrdRejReason, ""), pBody->GetFieldValueDefault(FIELD::Text, ""));//37
			fprintf(m_pfAuditTrail, "\n");
		}
		//Audit Trail end
	}
	else
	{
		WriteLog(LOG_DEBUG, "[Worker::FromAdmin]Unhandle message. Message type: %s\n", msgType);
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
		WriteLog(LOG_INFO, "[Worker::FromApp]:Execution Report received\n");
		ExecReport(lpMsg);
	}
	else if(strncmp(msgType, "9", 1) == 0)  // Order Cancel Reject (tag 35-MsgType=9)
	{
		WriteLog(LOG_INFO, "[Worker::FromApp]:Order Cancel Reject received\n");
		CancelReject(lpMsg);
	}
	else if (strncmp(msgType, "b", 1) == 0)
	{
		WriteLog(LOG_INFO, "[Worker::FromApp]:Quote Acknowledgment received\n");
		QuoteAck(lpMsg);
	}
	else
	{
		WriteLog(LOG_DEBUG, "[Worker::FromApp]Unhandle message. Message type: %c\n", msgType);
	}
	return 0;
}


