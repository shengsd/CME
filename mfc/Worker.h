#pragma once
#include "ExMDP.h"
#include "include/HSFIXEngn.h"
#include "include/fix_tag.h"
#include "mfcDlg.h"
#include <map>
#include <set>
#include <fstream>


using namespace std;

#define N_QUOTE_LEVEL  10  //10档行情
#define IMPLIED_QUOTE_LEVEL 2

#define LOG_INFO		0
#define LOG_DEBUG		1
#define LOG_WARNING		2
#define LOG_ERROR		3
#define LOG_FATAL		4

void __cdecl WriteLog(int nLevel, const char *szFormat, ...);

typedef struct tagQuoteItem
{
	int  securityID;						//CME合约唯一ID,主键
	//	char szExchangeType[12];			//交易类别
	//	char szCommodityType[12];			//品种代码类别
	//	char szContractCode[32];			//合约代码
	//	char cProductType;					//产品类别,futu_product_type  1:期货 2:期权 3:价差
	//	char cOptionsType;					//期权类别,options_type 0:非期权 1:看涨 2:看跌
	//	double fStrikePrice;				//执行价
	double bidPrice[N_QUOTE_LEVEL];
	int bidVolume[N_QUOTE_LEVEL];
	double askPrice[N_QUOTE_LEVEL];
	int askVolume[N_QUOTE_LEVEL];
	double impliedBid[IMPLIED_QUOTE_LEVEL];
	int impliedBidVol[IMPLIED_QUOTE_LEVEL];
	double impliedAsk[IMPLIED_QUOTE_LEVEL];
	int impliedAskVol[IMPLIED_QUOTE_LEVEL];
	double open, high, low, last, close;
	long lastVolume;					//当前成交量
	long bearVolume;					//持仓量
	double preClose;					//昨收盘价
	double prevSettlementPrice;			//昨结算价
	long tolVolume;						//总成交
	int cMarketStatus;					//合约市场状态
	int nTimeStamp;						//时间戳（本地时间）
}QuoteItem;

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
	char LegSymbol[50];
	char LegSecurityExchange[11];
	int  LegRatioQty;
	char LegSide;
	char LegContractCode[32];
}InstrumentLeg;

//合约信息
typedef struct tagInstrument
{
	//外部
	int  SecurityID;					//合约主键 Unique instrument ID as qualified by the exchange per tag 22-SecurityIDSource.
	char SecurityGroup[8];				//Security Group Code. iLink中用字段tag 55-Symbol 表示
	char Symbol[24];					//Instrument Name or Symbol. iLink中用tag 107-SecurityDesc表示
	char Asset[8];						//The underlying asset code also known as Product Code. iLink中用tag1151-SecurityGroup表示
	char SecurityExchange[8];			//交易类别
	char CFICode[8];					//Indicate the type of security.
	int  ApplID;						//Indicates the channel ID as defined in the XML configuration file
	double DisplayFactor;				//prices and ticks 的显示价格需要乘以此数
	int  GBXMarketDepth;				//行情深度
	int  GBIMarketDepth;				//implied行情深度
	InstrumentLeg insLeg[2];			//套利
	int  SecurityTradingStatus;		//交易市场状态

}Instrument;


class Worker: public Application, public IApplication
{
public:
	Worker(void);

	~Worker(void);

	UINT startTrade();

	UINT stopTrade();

	UINT startQuote();

	UINT stopQuote();

	void setDlg(CmfcDlg* pDlg) { m_pDlg = pDlg; }

	CmfcDlg*		m_pDlg;				/**< 主对话框 */

	std::ofstream m_fLog;

	//ProductManager
public:
	//根据SecurityID获取Instrument
	//返回 0 成功 -1 失败
	int GetInstrumentBySecurityID(const int securityID, Instrument& inst);

	//通过行情接收合约
	void OnUpdate(MDPFieldMap* pFieldMap);
private:

	typedef map<int, Instrument> MapIntToInstrument;//保存Instrument

	MapIntToInstrument m_mapSecurityID2Inst;//外部合约 -> Instrument

	//OrderManager
public:
	virtual void FUNCTION_CALL_MODE onEvent( const ISessionID * lpSessionID, int iEventID );

	virtual void FUNCTION_CALL_MODE OnCreate( const ISessionID * lpSessionID );

	virtual void FUNCTION_CALL_MODE OnLogon( const ISessionID * lpSessionID );

	virtual void FUNCTION_CALL_MODE OnLogout( const ISessionID * lpSessionID );

	virtual void FUNCTION_CALL_MODE ToAdmin( IMessage * lpMsg, const ISessionID * lpSessionID );

	virtual int FUNCTION_CALL_MODE ToApp( IMessage * lpMsg, const ISessionID * lpSessionID );

	virtual int FUNCTION_CALL_MODE FromAdmin( const IMessage * lpMsg , const ISessionID * lpSessionID );

	virtual int FUNCTION_CALL_MODE FromApp( const IMessage * lpMsg , const ISessionID * lpSessionID );

private:
	HANDLE m_hEventReadyToTrade;

	ISessionID* m_pTradeSession;

	//QuoteManager
public:
	//行情回调函数
	void onMarketData(MDPFieldMap* pMDPFieldMap, const int templateID);

	//SnapshotFullRefresh
	void SnapShot(MDPFieldMap* pFieldMap);

	//SecurityStatus
	void SecurityStatus(MDPFieldMap* pFieldMap);

	//MDIncrementalRefreshBook
	void UpdateBook(MDPFieldMap* pFieldMap);

	//MDIncrementalRefreshSessionStatistics
	void UpdateSessionStatistics(MDPFieldMap* pFieldMap);

	//MDIncrementalRefreshDailyStatistics
	void UpdateDailyStatistics(MDPFieldMap* pFieldMap);

	//MDIncrementalRefreshVolume
	void UpdateVolume(MDPFieldMap* pFieldMap);

	//MDIncrementalRefreshTrade
	//void UpdateTrade(MDPFieldMap* pFieldMap);

	//MDIncrementalRefreshTradeSummary
	void UpdateTradeSummary(MDPFieldMap* pFieldMap);

	//ChannelReset
	void ChannelReset(MDPFieldMap* pFieldMap);

	void updateOrderBook(int SecurityID);

private:
	typedef map<int, QuoteItem*> MapIntToQuote;

	typedef set<int> SetInt;

	typedef map<int, SetInt> MapIntToSet;

	MapIntToQuote m_mapSecurityID2Quote;//外部合约 -> 行情信息

	MapIntToSet m_mapApplID2SecurityIDs;//ChannelID -> 包含的外部合约

	//推送行情消息
	void PushQuote(const QuoteItem* qi);	

	//重置合约行情信息
	void EmptyQuote(QuoteItem* qi);
	//merge actual book and implied book
	void MergeBook(ConsolidatedBook* conBook, const QuoteItem* qi);

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







