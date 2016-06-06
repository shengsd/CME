#pragma once
#include "ExMDP.h"
#include "include/HSFIXEngn.h"
#include "include/fix_tag.h"
#include "mfcDlg.h"
#include <map>
#include <set>
#include <fstream>


using namespace std;

#define N_QUOTE_LEVEL  10  //10������
#define IMPLIED_QUOTE_LEVEL 2

#define LOG_INFO		0
#define LOG_DEBUG		1
#define LOG_WARNING		2
#define LOG_ERROR		3
#define LOG_FATAL		4

void __cdecl WriteLog(int nLevel, const char *szFormat, ...);

typedef struct tagQuoteItem
{
	int  securityID;						//CME��ԼΨһID,����
	//	char szExchangeType[12];			//�������
	//	char szCommodityType[12];			//Ʒ�ִ������
	//	char szContractCode[32];			//��Լ����
	//	char cProductType;					//��Ʒ���,futu_product_type  1:�ڻ� 2:��Ȩ 3:�۲�
	//	char cOptionsType;					//��Ȩ���,options_type 0:����Ȩ 1:���� 2:����
	//	double fStrikePrice;				//ִ�м�
	double bidPrice[N_QUOTE_LEVEL];
	int bidVolume[N_QUOTE_LEVEL];
	double askPrice[N_QUOTE_LEVEL];
	int askVolume[N_QUOTE_LEVEL];
	double impliedBid[IMPLIED_QUOTE_LEVEL];
	int impliedBidVol[IMPLIED_QUOTE_LEVEL];
	double impliedAsk[IMPLIED_QUOTE_LEVEL];
	int impliedAskVol[IMPLIED_QUOTE_LEVEL];
	double open, high, low, last, close;
	long lastVolume;					//��ǰ�ɽ���
	long bearVolume;					//�ֲ���
	double preClose;					//�����̼�
	double prevSettlementPrice;			//������
	long tolVolume;						//�ܳɽ�
	int cMarketStatus;					//��Լ�г�״̬
	int nTimeStamp;						//ʱ���������ʱ�䣩
}QuoteItem;

typedef struct
{
	double bidPrice[10];
	double askPrice[10];
	int bidVolume[10];
	int askVolume[10];
}ConsolidatedBook;

//��Ϻ�Լ������Ϣ
typedef struct ComponentInstrumentLeg
{
	char LegSymbol[50];
	char LegSecurityExchange[11];
	int  LegRatioQty;
	char LegSide;
	char LegContractCode[32];
}InstrumentLeg;

//��Լ��Ϣ
typedef struct tagInstrument
{
	//�ⲿ
	int  SecurityID;					//��Լ���� Unique instrument ID as qualified by the exchange per tag 22-SecurityIDSource.
	char SecurityGroup[8];				//Security Group Code. iLink�����ֶ�tag 55-Symbol ��ʾ
	char Symbol[24];					//Instrument Name or Symbol. iLink����tag 107-SecurityDesc��ʾ
	char Asset[8];						//The underlying asset code also known as Product Code. iLink����tag1151-SecurityGroup��ʾ
	char SecurityExchange[8];			//�������
	char CFICode[8];					//Indicate the type of security.
	int  ApplID;						//Indicates the channel ID as defined in the XML configuration file
	double DisplayFactor;				//prices and ticks ����ʾ�۸���Ҫ���Դ���
	int  GBXMarketDepth;				//�������
	int  GBIMarketDepth;				//implied�������
	InstrumentLeg insLeg[2];			//����
	int  SecurityTradingStatus;		//�����г�״̬

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

	CmfcDlg*		m_pDlg;				/**< ���Ի��� */

	std::ofstream m_fLog;

	//ProductManager
public:
	//����SecurityID��ȡInstrument
	//���� 0 �ɹ� -1 ʧ��
	int GetInstrumentBySecurityID(const int securityID, Instrument& inst);

	//ͨ��������պ�Լ
	void OnUpdate(MDPFieldMap* pFieldMap);
private:

	typedef map<int, Instrument> MapIntToInstrument;//����Instrument

	MapIntToInstrument m_mapSecurityID2Inst;//�ⲿ��Լ -> Instrument

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
	//����ص�����
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

	MapIntToQuote m_mapSecurityID2Quote;//�ⲿ��Լ -> ������Ϣ

	MapIntToSet m_mapApplID2SecurityIDs;//ChannelID -> �������ⲿ��Լ

	//����������Ϣ
	void PushQuote(const QuoteItem* qi);	

	//���ú�Լ������Ϣ
	void EmptyQuote(QuoteItem* qi);
	//merge actual book and implied book
	void MergeBook(ConsolidatedBook* conBook, const QuoteItem* qi);

	//��val����lvs�����posλ���У����������һ��Ԫ��
	//���ڴ����pos�������κζ���
	template <typename T>
	void LevelInsert(T lvs[], int pos, int size, T val)
	{
		if(pos < 0 || pos >= size)
			return;
		for(int i = size - 1; i > pos; i--)
			lvs[i] = lvs[i-1];
		lvs[pos] = val;
	}

	//��lvs����ɾ��posλ�õ�Ԫ��
	//���ڴ����pos�������κζ���
	template <typename T>
	void LevelDelete(T lvs[], int pos, int size)
	{
		if(pos < 0 || pos >= size)
			return;
		for(int i = pos; i < size - 1; i++)
		{
			lvs[i] = lvs[i+1];
		}
		lvs[size-1] = 0;  //ĩλ����
	}

	//�������� arr[lvl] = val
	template <typename T>
	void LevelChange(T arr[], int lvl, int size, T val)
	{
		if (lvl < 0 || lvl >= size )
			return ;
		arr[lvl] = val; 
	}

	//��������
	template <typename T>
	void LevelClear(T arr[], int size)
	{
		for (int i = 0; i < size; i++)
			arr[i] = 0;
	}

	//ɾ������ǰlvl��������0����
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







