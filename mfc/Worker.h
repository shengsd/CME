#pragma once
#include "ExMDP.h"
//#include "include/hsfix2_interface.h"
#include "cmeDlg.h"
#include <set>
#include <fstream>
#include "include/HSFIXEngn.h"
#include "include/fix_tag.h"
#include <map>

#define N_QUOTE_LEVEL  10  //10������
#define IMPLIED_QUOTE_LEVEL 2

#define LOG_INFO		0
#define LOG_DEBUG		1
#define LOG_WARNING		2
#define LOG_ERROR		3
#define LOG_FATAL		4

//��������״̬
#define STATUS_SUCCESS		0
#define STATUS_FAIL			1

///����
typedef struct 
{
	TCHAR MsgType[2];//D=New Order F=Order Cancel Request G=Order Cancel/Replace Request
	TCHAR	Account[13];	//Unique account identifier.
	TCHAR	ClOrdID[21];	//���ص��ţ�ÿ��������ί��ָ���Ӧһ�����ص���|Unique order identifier assigned by client system. 
	TCHAR OrigClOrdID[21];//����������һ�����ص��ţ��ĵ���������Ӧ�Ķ����ı��ص���|Last accepted ClOrdID in the order chain.
	TCHAR	CorrelationClOrdID[21];//�������ʼ�ı��ص��ţ��ĵ���������Ӧ���µ��ı��ص���|This tag should contain the same value as the tag 11-ClOrdID of the original New Order (tag 35-MsgType-D) message and is used to correlate iLink messages associated with a single order chain.
	//TCHAR	HandInst;		//'1'
	TCHAR OrderID[18];//��������
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
	TCHAR OrdStatus[2]; //������ʵ��״̬. D=disable���� P=Pending���� 	0=New�ѱ� 	4=Cancelled�ѳ�	5=Modified�Ѹ� 1=Partial Filled���� 2=Complete Filled�ѳ� C=Expired 8=Rejected H=Trade Cancel U=Undefined A=CAncel Reject L=ALter Reject
	TCHAR ErrorInfo[64];//������Ϣ���Զ����ֶ�
} ORDER;

//�����������ɽ��ر�
typedef struct 
{
	TCHAR	Account[13];	//Unique account identifier.
	TCHAR ClOrdID[21];//���ص��ţ�ÿ��������ί��ָ���Ӧһ�����ص���|Unique order identifier assigned by client system. 
	TCHAR OrderID[19];//��������
	TCHAR CumQty[10];//������Ч���ڵ��ۼƳɽ�����	This value does not reset if order is cancel/replace.
	TCHAR ExecID[41];//CME Globex assigned message identifier; unique per market segment per trading session.
	TCHAR LastPx[21];//�ɽ���
	TCHAR LastQty[10];//�ɽ���
	TCHAR OrderQty[10];//��������
	TCHAR OrdStatus[2];//����״̬
	TCHAR	OrdType[2];//��������
	TCHAR OrigClOrdID[21];//����������һ�����ص��ţ��ĵ���������Ӧ�Ķ����ı��ص���|Last accepted ClOrdID in the order chain.
	TCHAR Price[21];//�м۵������м�ֹ�𵥵ı����м�
	TCHAR SecurityID[13];//Identifier of the instrument defined in tag 107-SecurityDesc.
	TCHAR	Side[2];//1=Buy 2=Sell
	TCHAR	Symbol[8];//This tag contains the instrument group code of the instrument.
	TCHAR	TimeInForce[2];	//Specifies how long the order remains in effect. If not present, DAY order is the default. For GTD, ExpireDate is required. For FAK, MinQty can also be specified.
	TCHAR TransactTime[22];//UTC format YYYYMMDD-HH:MM:SS.sss e.g. 20091216-19:21:41.109
	TCHAR TradeDate[9];//Indicates date of trade reference in this message in YYYYMMDD format.
	TCHAR SecurityDesc[21];//Instrument identifier. 	e.g. "ESM0'
	TCHAR	MinQty[10];		//Minimum quantity of an order to be executed. This tag is used only when tag 59-TimeInForce=3 (Fill and Kill). The value of MinQty must be between 1 and the value in tag 38-OrderQty. 
	TCHAR	SecurityType[8];//Indicates instrument is future or option.
	TCHAR ExecType[2];//ִ�б�������
	TCHAR LeavesQty[10];//ʣ�µ�û�ɽ�������
	TCHAR	ExpireDate[9];	//YYYYMMDD Required only if tag 59-TimeInForce=Good Till Date (GTD).
	TCHAR	MaxShow[10];	//Display quantity of an order to be shown in the order book at any given time.
	TCHAR	StopPx[21];		//Required for stop and stop-limit orders.
	TCHAR	CorrelationClOrdID[21];//�������ʼ�ı��ص��ţ��ĵ���������Ӧ���µ��ı��ص���|This tag should contain the same value as the tag 11-ClOrdID of the original New Order (tag 35-MsgType-D) message and is used to correlate iLink messages associated with a single order chain.
	TCHAR Text[151];
}EXCREPORT;

typedef struct 
{
	int  securityID;		//CME��ԼΨһID,����
	unsigned int RptSeq;		//��Լ�������
	//	char szExchangeType[12];		//�������
	//	char szCommodityType[12];	//Ʒ�ִ������
	//	char szContractCode[32];		//��Լ����
	//	char cProductType;				//��Ʒ���,futu_product_type  1:�ڻ� 2:��Ȩ 3:�۲�
	//	char cOptionsType;				//��Ȩ���,options_type 0:����Ȩ 1:���� 2:����
	//	double fStrikePrice;				//ִ�м�
	double bidPrice[N_QUOTE_LEVEL];	//���
	int bidVolume[N_QUOTE_LEVEL];		//����
	double askPrice[N_QUOTE_LEVEL];	//����
	int askVolume[N_QUOTE_LEVEL];		//����
	double impliedBid[IMPLIED_QUOTE_LEVEL];	//�������
	int impliedBidVol[IMPLIED_QUOTE_LEVEL];	//��������
	double impliedAsk[IMPLIED_QUOTE_LEVEL];	//��������
	int impliedAskVol[IMPLIED_QUOTE_LEVEL];	//��������
	double open, high, low, last, close;//�����ߣ��ͣ��£���
	long lastVolume;					//��ǰ�ɽ���
	long bearVolume;					//�ֲ���
	double preClose;					//�����̼�
	double prevSettlementPrice;	//������
	long tolVolume;					//�ܳɽ�
	int cMarketStatus;				//��Լ�г�״̬
	int nTimeStamp;					//ʱ���������ʱ�䣩
}QuoteItem;

//�ϲ���������̣���
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
	TCHAR LegSymbol[50];
	TCHAR LegSecurityExchange[11];
	int  LegRatioQty;
	TCHAR LegSide;
	TCHAR LegContractCode[32];
}InstrumentLeg;

//��Լ��Ϣ
typedef struct tagInstrument
{
	//�ⲿ
	int  SecurityID;					//��Լ���� Unique instrument ID as qualified by the exchange per tag 22-SecurityIDSource.
	TCHAR SecurityGroup[8];				//Security Group Code. iLink�����ֶ�tag 55-Symbol ��ʾ
	TCHAR Symbol[24];					//Instrument Name or Symbol. iLink����tag 107-SecurityDesc��ʾ
	TCHAR Asset[8];						//The underlying asset code also known as Product Code. iLink����tag1151-SecurityGroup��ʾ
	TCHAR SecurityExchange[8];			//�������
	TCHAR SecurityType[8];			//FUT = Future or Future Spread	OOF = Options on Futures or Options on Futures Spread
	TCHAR CFICode[8];					//Indicate the type of security.
	int  ApplID;						//Indicates the channel ID as defined in the XML configuration file
	double DisplayFactor;				//prices and ticks ����ʾ�۸���Ҫ���Դ���
	int  GBXMarketDepth;				//�������
	int  GBIMarketDepth;				//implied�������
	InstrumentLeg insLeg[2];			//����
	int  SecurityTradingStatus;			//�����г�״̬
	unsigned char MDSecurityTradingStatus;	//����״̬
}Instrument;

//����FIX��Ϣ��������ʹ�ã������Զ�������Ϣ
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

	//��д��ʽ��ֱ�ӵ��ú���
	void setDlg(CcmeDlg* pDlg) 
	{
		m_pDlg = pDlg;
	}

	// ���Ի��� ֱ�Ӳ���ͼ�ν���
	CcmeDlg* m_pDlg;
	//HWND m_hWindDlg;

	//��ӡ��־��ͼ�ν���
	void __cdecl WriteLog(int nLevel, const TCHAR *szFormat, ...);

	//�������濪��
	UINT startQuote();
	UINT stopQuote();

	//����MDP3.0�������
	std::ofstream m_fLog;

	//���׹���
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

	//��������
	BOOL StartTrade();

	//�رս���
	void StopTrade();

	///��ѯ����״̬���������׺��һ����
	void MassOrderStatusQuery();

	///�µ�����д(�޸Ľ���)
	/**
	*@param  ORDER& order �µ���Ϣ,���ܲ����
	*/
	int EnterOrder(ORDER& order);

	///����
	/**
	*@param  ORDER& order ��������
	*/
	int CancelOrder(ORDER& order);
	
	///�ĵ�
	/**
	*@param  ORDER& order ���Ķ���
	*/
	int AlterOrder(ORDER& order);

	///�µ����ĵ���������������ѯ�������ɽ��ر�
	/**
	*@param  const IMessage* pMsg FIX��Ϣ
	*/
	void ExecReport(const IMessage* pMsg);

	//�ķϡ�����
	void CancelReject(const IMessage* pMsg);

	///ѯ��
	int RequestForQuote(ORDER& order);

	///ѯ�۷���
	void QuoteAck(const IMessage* pMsg);

	///������ѯ����ת���ɶ���
	/**
	*@param  order ������Ϣ
	*@param  const EXCREPORT& excReport  �鶩�����ض�����Ϣ
	*/
	void ExtractOrderFromExcReport( ORDER& order, const EXCREPORT& excReport );

	///���Ӷ�����Ϣ���̰߳�ȫ����
	/**
	*@param ORDER& order  ������Ϣ
	*/
	void AddOrder(ORDER& order);

	///���¶�����Ϣ
	/**
	*@param ORDER& order ������Ϣ
	*/
	void UpdateOrder(ORDER& order);

	///������ѯ
	/**
	*@param  nClOrderID ���ص���
	*@param  order ������Ϣ
	*@return TRUE-�ɹ�; FALSE-ʧ��
	*/
	BOOL GetOrderByClOrderID(const CString csClOrderID, ORDER& order);

private:

	HANDLE m_hEventReadyToTrade;

	ISessionID* m_pTradeSession;

	CListCtrl* m_pLvOrderInfoList;

	typedef std::map<CString, ORDER> MapClOrderIDToORDER;

	MapClOrderIDToORDER m_mapClOrderIDToOrder;//���ص��� -> ����

	CRITICAL_SECTION m_OrderLock;

	//Audit Trail
	FILE* m_pfAuditTrail;

	//long m_MessageLinkID;
	//��Լ����
public:
	//����SecurityID��ȡInstrument
	//���� 0 �ɹ� 1 ʧ��
	int GetInstrumentBySecurityID(const int securityID, Instrument& inst);

	//ͨ��������պ�Լ
	void OnUpdateContract(MDPFieldMap* pFieldMap);
private:
	typedef std::map<int, Instrument> MapIntToInstrument;
	MapIntToInstrument m_mapSecurityID2Inst;//�ⲿ��ԼID -> Instrument

	//���鹦��
public:
	//����ص�����
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

	MapIntToQuoteItemPtr m_mapSecurityID2Quote;//�ⲿ��Լ -> ������Ϣ

	MapIntToSetInt m_mapApplID2SecurityIDs;//ChannelID -> �������ⲿ��Լ

	//����������Ϣ
	void PushMktDtItem(const QuoteItem* pItem);

	//���ú�Լ������Ϣ
	void EmptyMktDtItem(QuoteItem* pItem);

	//merge actual book and implied book
	void MergeBook(ConsolidatedBook* pBook, const QuoteItem* pItem);

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







