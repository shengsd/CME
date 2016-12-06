#pragma once
#include "ExMDP.h"
//#include "include/hsfix2_interface.h"
#include "cmeDlg.h"
#include <set>
#include <fstream>
#include "include/HSFIXEngn.h"
#include "include/fix_tag.h"
#include <map>

#define N_QUOTE_LEVEL  10  //10档行情
#define IMPLIED_QUOTE_LEVEL 2

#define LOG_INFO		0
#define LOG_DEBUG		1
#define LOG_WARNING		2
#define LOG_ERROR		3
#define LOG_FATAL		4

//订单返回状态
#define STATUS_SUCCESS		0
#define STATUS_FAIL			1

///订单
typedef struct 
{
	TCHAR MsgType[2];//D=New Order F=Order Cancel Request G=Order Cancel/Replace Request
	TCHAR	Account[13];	//Unique account identifier.
	TCHAR	ClOrdID[21];	//本地单号，每条订单（委托指令）对应一个本地单号|Unique order identifier assigned by client system. 
	TCHAR OrigClOrdID[21];//订单链的上一个本地单号，改单，撤单对应的订单的本地单号|Last accepted ClOrdID in the order chain.
	TCHAR	CorrelationClOrdID[21];//订单链最开始的本地单号，改单，撤单对应的下单的本地单号|This tag should contain the same value as the tag 11-ClOrdID of the original New Order (tag 35-MsgType-D) message and is used to correlate iLink messages associated with a single order chain.
	//TCHAR	HandInst;		//'1'
	TCHAR OrderID[18];//主场单号
	TCHAR	OrderQty[10];	//Order quantity. Must be a positive integer.
	TCHAR	OrdType[2];		//Order type.
	TCHAR	Price[21];		//Required for limit or stop-limit orders.
	TCHAR	Side[2];			//1=Buy 2=Sell
	TCHAR	Symbol[7];		//This tag contains the instrument group code of the instrument.
	TCHAR	TimeInForce[2];	//Specifies how long the order remains in effect. If not present, DAY order is the default. For GTD, ExpireDate is required. For FAK, MinQty can also be specified.
	TCHAR	ManualOrderIndicator;//Y=manual N=automated
	TCHAR	StopPx[21];		//Required for stop and stop-limit orders.
	TCHAR	SecurityDesc[21];//Instrument identifier. Future Example: GEZ8 Option Example: CEZ9 C9375
	TCHAR	MinQty[10];		//Minimum quantity of an order to be executed. This tag is used only when tag 59-TimeInForce=3 (Fill and Kill). The value of MinQty must be between 1 and the value in tag 38-OrderQty. 
	TCHAR	SecurityType[4];//Indicates instrument is future or option.
	//TCHAR	CustomerOrFirm;	//0=Customer 1=Firm
	TCHAR	MaxShow[10];	//Display quantity of an order to be shown in the order book at any given time.
	TCHAR	ExpireDate[9];	//YYYYMMDD Required only if tag 59-TimeInForce=Good Till Date (GTD).
	//TCHAR	CtiCode;		//'1'
	TCHAR OrdStatus[2]; //订单的实际状态. D=disable作废 P=Pending正在 	0=New已报 	4=Cancelled已撤	5=Modified已改 1=Partial Filled部成 2=Complete Filled已成 C=Expired 8=Rejected H=Trade Cancel U=Undefined A=CAncel Reject L=ALter Reject
	TCHAR ErrorInfo[64];//错误信息，自定义字段
} ORDER;

//订单反馈、成交回报
typedef struct 
{
	TCHAR	Account[13];	//Unique account identifier.
	TCHAR ClOrdID[21];//本地单号，每条订单（委托指令）对应一个本地单号|Unique order identifier assigned by client system. 
	TCHAR OrderID[19];//主场单号
	TCHAR CumQty[10];//订单有效期内的累计成交数量	This value does not reset if order is cancel/replace.
	TCHAR ExecID[41];//CME Globex assigned message identifier; unique per market segment per trading session.
	TCHAR LastPx[21];//成交价
	TCHAR LastQty[10];//成交量
	TCHAR OrderQty[10];//订单数量
	TCHAR OrdStatus[2];//订单状态
	TCHAR	OrdType[2];//订单类型
	TCHAR OrigClOrdID[21];//订单链的上一个本地单号，改单，撤单对应的订单的本地单号|Last accepted ClOrdID in the order chain.
	TCHAR Price[21];//市价单或者市价止损单的保护市价
	TCHAR SecurityID[13];//Identifier of the instrument defined in tag 107-SecurityDesc.
	TCHAR	Side[2];//1=Buy 2=Sell
	TCHAR	Symbol[8];//This tag contains the instrument group code of the instrument.
	TCHAR	TimeInForce[2];	//Specifies how long the order remains in effect. If not present, DAY order is the default. For GTD, ExpireDate is required. For FAK, MinQty can also be specified.
	TCHAR TransactTime[22];//UTC format YYYYMMDD-HH:MM:SS.sss e.g. 20091216-19:21:41.109
	TCHAR TradeDate[9];//Indicates date of trade reference in this message in YYYYMMDD format.
	TCHAR SecurityDesc[21];//Instrument identifier. 	e.g. "ESM0'
	TCHAR	MinQty[10];		//Minimum quantity of an order to be executed. This tag is used only when tag 59-TimeInForce=3 (Fill and Kill). The value of MinQty must be between 1 and the value in tag 38-OrderQty. 
	TCHAR	SecurityType[8];//Indicates instrument is future or option.
	TCHAR ExecType[2];//执行报告类型
	TCHAR LeavesQty[10];//剩下的没成交的数量
	TCHAR	ExpireDate[9];	//YYYYMMDD Required only if tag 59-TimeInForce=Good Till Date (GTD).
	TCHAR	MaxShow[10];	//Display quantity of an order to be shown in the order book at any given time.
	TCHAR	StopPx[21];		//Required for stop and stop-limit orders.
	TCHAR	CorrelationClOrdID[21];//订单链最开始的本地单号，改单，撤单对应的下单的本地单号|This tag should contain the same value as the tag 11-ClOrdID of the original New Order (tag 35-MsgType-D) message and is used to correlate iLink messages associated with a single order chain.
	TCHAR Text[151];
}EXCREPORT;

typedef struct 
{
	int  securityID;		//CME合约唯一ID,主键
	unsigned int RptSeq;		//合约更新序号
	//	char szExchangeType[12];		//交易类别
	//	char szCommodityType[12];	//品种代码类别
	//	char szContractCode[32];		//合约代码
	//	char cProductType;				//产品类别,futu_product_type  1:期货 2:期权 3:价差
	//	char cOptionsType;				//期权类别,options_type 0:非期权 1:看涨 2:看跌
	//	double fStrikePrice;				//执行价
	double bidPrice[N_QUOTE_LEVEL];	//买价
	int bidVolume[N_QUOTE_LEVEL];		//买量
	double askPrice[N_QUOTE_LEVEL];	//卖价
	int askVolume[N_QUOTE_LEVEL];		//卖量
	double impliedBid[IMPLIED_QUOTE_LEVEL];	//隐含买价
	int impliedBidVol[IMPLIED_QUOTE_LEVEL];	//隐含买量
	double impliedAsk[IMPLIED_QUOTE_LEVEL];	//隐含卖价
	int impliedAskVol[IMPLIED_QUOTE_LEVEL];	//隐含卖量
	double open, high, low, last, close;//开，高，低，新，收
	long lastVolume;					//当前成交量
	long bearVolume;					//持仓量
	double preClose;					//昨收盘价
	double prevSettlementPrice;	//昨结算价
	long tolVolume;					//总成交
	int cMarketStatus;				//合约市场状态
	int nTimeStamp;					//时间戳（本地时间）
}QuoteItem;

//合并后的买卖盘（）
typedef struct
{
	double bidPrice[10];
	double askPrice[10];
	int bidVolume[10];
	int askVolume[10];
}ConsolidatedBook;

//组合合约单腿信息
typedef struct ComponentInstrumentLeg
{
	TCHAR LegSymbol[50];
	TCHAR LegSecurityExchange[11];
	int  LegRatioQty;
	TCHAR LegSide;
	TCHAR LegContractCode[32];
}InstrumentLeg;

//合约信息
typedef struct tagInstrument
{
	//外部
	int  SecurityID;					//合约主键 Unique instrument ID as qualified by the exchange per tag 22-SecurityIDSource.
	TCHAR SecurityGroup[8];				//Security Group Code. iLink中用字段tag 55-Symbol 表示
	TCHAR Symbol[24];					//Instrument Name or Symbol. iLink中用tag 107-SecurityDesc表示
	TCHAR Asset[8];						//The underlying asset code also known as Product Code. iLink中用tag1151-SecurityGroup表示
	TCHAR SecurityExchange[8];			//交易类别
	TCHAR SecurityType[8];			//FUT = Future or Future Spread	OOF = Options on Futures or Options on Futures Spread
	TCHAR CFICode[8];					//Indicate the type of security.
	int  ApplID;						//Indicates the channel ID as defined in the XML configuration file
	double DisplayFactor;				//prices and ticks 的显示价格需要乘以此数
	int  GBXMarketDepth;				//行情深度
	int  GBIMarketDepth;				//implied行情深度
	InstrumentLeg insLeg[2];			//套利
	int  SecurityTradingStatus;			//交易市场状态
	unsigned char MDSecurityTradingStatus;	//交易状态
}Instrument;

//创建FIX消息，函数中使用，可以自动销毁消息
class CFIXMSG
{
public:
	CFIXMSG(char * lpBeginString, char * lpMsgType)
	{
		lpMsg = CreateMessage(lpBeginString, lpMsgType);
	}

	~CFIXMSG()
	{
		DestroyMessage(lpMsg);
	}

	IMessage * GetMsg()
	{
		return lpMsg;
	}

private:
	IMessage * lpMsg;
};

class Worker: public Application, public IApplication
{
public:
	Worker(void);
	~Worker(void);

	//回写方式，直接调用函数
	void setDlg(CcmeDlg* pDlg) 
	{
		m_pDlg = pDlg;
	}

	// 主对话框 直接操作图形界面
	CcmeDlg* m_pDlg;
	//HWND m_hWindDlg;

	//打印日志到图形界面
	void __cdecl WriteLog(int nLevel, const TCHAR *szFormat, ...);

	//行情引擎开关
	UINT startQuote();
	UINT stopQuote();

	//用于MDP3.0行情调试
	std::ofstream m_fLog;

	//交易功能
public:
	//HS FIXENGINE INTERFACE
	virtual void FUNCTION_CALL_MODE onEvent( const ISessionID * lpSessionID, int iEventID );

	virtual void FUNCTION_CALL_MODE OnCreate( const ISessionID * lpSessionID );

	virtual void FUNCTION_CALL_MODE OnLogon( const ISessionID * lpSessionID );

	virtual void FUNCTION_CALL_MODE OnLogout( const ISessionID * lpSessionID );

	virtual void FUNCTION_CALL_MODE ToAdmin( IMessage * lpMsg, const ISessionID * lpSessionID );

	virtual int FUNCTION_CALL_MODE ToApp( IMessage * lpMsg, const ISessionID * lpSessionID );

	virtual int FUNCTION_CALL_MODE FromAdmin( const IMessage * lpMsg , const ISessionID * lpSessionID );

	virtual int FUNCTION_CALL_MODE FromApp( const IMessage * lpMsg , const ISessionID * lpSessionID );

	//开启交易
	BOOL StartTrade();

	//关闭交易
	void StopTrade();

	///查询订单状态，开启交易后第一件事
	void MassOrderStatusQuery();

	///下单，回写(修改界面)
	/**
	*@param  ORDER& order 下单信息,可能不充分
	*/
	int EnterOrder(ORDER& order);

	///撤单
	/**
	*@param  ORDER& order 被撤订单
	*/
	int CancelOrder(ORDER& order);
	
	///改单
	/**
	*@param  ORDER& order 被改订单
	*/
	int AlterOrder(ORDER& order);

	///下单、改单、撤单、订单查询反馈，成交回报
	/**
	*@param  const IMessage* pMsg FIX消息
	*/
	void ExecReport(const IMessage* pMsg);

	//改废、撤废
	void CancelReject(const IMessage* pMsg);

	///询价
	int RequestForQuote(ORDER& order);

	///询价反馈
	void QuoteAck(const IMessage* pMsg);

	///订单查询反馈转换成订单
	/**
	*@param  order 订单信息
	*@param  const EXCREPORT& excReport  查订单返回订单信息
	*/
	void ExtractOrderFromExcReport( ORDER& order, const EXCREPORT& excReport );

	///增加订单信息，线程安全函数
	/**
	*@param ORDER& order  订单信息
	*/
	void AddOrder(ORDER& order);

	///更新订单信息
	/**
	*@param ORDER& order 订单信息
	*/
	void UpdateOrder(ORDER& order);

	///订单查询
	/**
	*@param  nClOrderID 本地单号
	*@param  order 订单信息
	*@return TRUE-成功; FALSE-失败
	*/
	BOOL GetOrderByClOrderID(const CString csClOrderID, ORDER& order);

private:

	HANDLE m_hEventReadyToTrade;

	ISessionID* m_pTradeSession;

	CListCtrl* m_pLvOrderInfoList;

	typedef std::map<CString, ORDER> MapClOrderIDToORDER;

	MapClOrderIDToORDER m_mapClOrderIDToOrder;//本地单号 -> 订单

	CRITICAL_SECTION m_OrderLock;

	//Audit Trail
	FILE* m_pfAuditTrail;

	//long m_MessageLinkID;
	//合约功能
public:
	//根据SecurityID获取Instrument
	//返回 0 成功 1 失败
	int GetInstrumentBySecurityID(const int securityID, Instrument& inst);

	//通过行情接收合约
	void OnUpdateContract(MDPFieldMap* pFieldMap);
private:
	typedef std::map<int, Instrument> MapIntToInstrument;
	MapIntToInstrument m_mapSecurityID2Inst;//外部合约ID -> Instrument

	//行情功能
public:
	//行情回调函数
	void onMarketData(MDPFieldMap* pMDPFieldMap, const int templateID);

	//SnapshotFullRefresh
	void SnapShot(MDPFieldMap* pFieldMap);

	//SecurityStatus
	void SecurityStatus(MDPFieldMap* pFieldMap);

	//MDIncrementalRefreshBook
	void UpdateQuoteItem(MDPFieldMap* pFieldMap);

	void updateOrderBook(int SecurityID);

private:
	typedef std::map<int, QuoteItem*> MapIntToQuoteItemPtr;

	typedef std::set<int> SetInt;

	typedef std::map<int, SetInt> MapIntToSetInt;

	MapIntToQuoteItemPtr m_mapSecurityID2Quote;//外部合约 -> 行情信息

	MapIntToSetInt m_mapApplID2SecurityIDs;//ChannelID -> 包含的外部合约

	//推送行情消息
	void PushMktDtItem(const QuoteItem* pItem);

	//重置合约行情信息
	void EmptyMktDtItem(QuoteItem* pItem);

	//merge actual book and implied book
	void MergeBook(ConsolidatedBook* pBook, const QuoteItem* pItem);

	//把val插入lvs数组的pos位置中，并丢弃最后一个元素
	//对于错误的pos，不做任何动作
	template <typename T>
	void LevelInsert(T lvs[], int pos, int size, T val)
	{
		if(pos < 0 || pos >= size)
			return;
		for(int i = size - 1; i > pos; i--)
			lvs[i] = lvs[i-1];
		lvs[pos] = val;
	}

	//从lvs数字删除pos位置的元素
	//对于错误的pos，不做任何动作
	template <typename T>
	void LevelDelete(T lvs[], int pos, int size)
	{
		if(pos < 0 || pos >= size)
			return;
		for(int i = pos; i < size - 1; i++)
		{
			lvs[i] = lvs[i+1];
		}
		lvs[size-1] = 0;  //末位置零
	}

	//更新数组 arr[lvl] = val
	template <typename T>
	void LevelChange(T arr[], int lvl, int size, T val)
	{
		if (lvl < 0 || lvl >= size )
			return ;
		arr[lvl] = val; 
	}

	//数组清零
	template <typename T>
	void LevelClear(T arr[], int size)
	{
		for (int i = 0; i < size; i++)
			arr[i] = 0;
	}

	//删除数组前lvl个，后面0补齐
	template <typename T>
	void LevelDelFrom(T arr[], int lvl, int size)
	{
		if (lvl < 0 || lvl > size)
			return ;
		for (int i = 0; i < size - lvl; i++)
		{
			arr[i] = arr[i+lvl];
		}
		for (int j = lvl; j < size; j++)
		{
			arr[j] = 0;
		}
	}
};







