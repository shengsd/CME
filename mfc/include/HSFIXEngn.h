///HSFIX�������ӿڶ��壬֧��FIX4.0~4.4���������Э��
/**��������������Ϊ���ͻ��ˡ���ͻ��ˡ������Ӧ�á��ͻ��˷���˻��Ӧ��;
 * ������������ļ���ά������Ự��ÿ���Ự������(C/S)������Э��ȿ��Բ�ͬ��
 * ������ʹ���������˻Ự���͡�����Э��Ȳ��죻
 * ������
 * (1)���������������ע��ӿڡ���ѯ�Ự	
 * (2)�����Ӧ�ûص��ӿ�IApplication��������Ӧ����ĸ���Ӧ���¼���
 * (3)����Ự�ӿ�ISession�����ڴ����뷢����Ϣ����ȡ�Ự״̬�ȣ�

 *	ע�����HSFixEngApi	V1.0�ײ��ǵ��߳�ʵ�֣��û���IApplication
 *	�ӿ�ʵ����,���뾡�췵��,�ڻص�������,������г�ʱ����������
 *	�ȴ�,�Է�ֹӰ�쵽���������.
 *	��2.0�汾������ײ㽫��֧�ֶ��߳�,����,�û�������뿼�ǵ��̰߳�ȫ����.
 *	��������API�汾������,�ϵ�Ӧ�ÿ��Լ���ʹ��.

  //	maoyinjie 20111008	���ݻỰ������
	//	maoyinjie 201203/29	������Ϣ�����ӿ�
	maoyinjie 20120911	�����������ĻỰ���޵Ľӿ�
	2014/01/03	maoyinjie	���ӻ�ûỰ�ڲ�����/������ŵĽӿ�		�汾������2.0.0.4

 */
 
#ifndef HSFIX_APPLICATION_H
#define HSFIX_APPLICATION_H

#include "MessageInterface.h"        //��Ϣ�������ӿڶ���

/// �ỰID
/**
 * һ���ỰID��ǰ׺��, ����ID��Ŀ�귽ID��Ŀ�귽�����޶����Ĳ���������ɣ�
 * Ŀ�귽�����޶�������������ͬһĿ�귽�Ĳ�ͬ�Ự��
 *
 * ����ͨ���ỰID����ʶһ���Ự��ʹ����ͨ���ỰID����ȡƥ��ĻỰ�ӿ�;
 *
 */
struct ISessionID
{
	///ǰ׺��		
	const char* lpBeginString;
	///����ID
	const char* lpSenderCompID;
	///Ŀ�귽ID
	const char* lpTargetCompID;
	///Ŀ�귽�����޶���
	const char* lpSessionQualifier;
/*
	bool operator==(ISessionID& rht)
	{
		if( strcmp(lpBeginString, rht.lpBeginString) )
			return false;
		if( strcmp(lpSenderCompID, rht.lpSenderCompID) )
			return false;
		if( strcmp(lpTargetCompID, rht.lpTargetCompID) )
			return false;
		if( strcmp(lpSessionQualifier, rht.lpSessionQualifier) )
			return false;
		return true;
	}
*/
};
bool operator==(ISessionID& lft, ISessionID& rht);

///�Ự״̬
struct ISessionState : public IKnownV1
{
	/// �Ự�Ƿ���Ч
	virtual bool FUNCTION_CALL_MODE IsEnabled( ) const = 0;
  
	///��ǰ�Ự�Ƿ��ѵ�¼
	virtual bool FUNCTION_CALL_MODE IsLoggedOn() const = 0;

	///�Ƿ��Ƿ�����
	virtual bool FUNCTION_CALL_MODE IsInitiate() const = 0;

	///��¼ʱ��
	virtual int FUNCTION_CALL_MODE LogonTimeout() const = 0;

	///�ǳ�ʱ��
	virtual int FUNCTION_CALL_MODE LogoutTimeout() const = 0;

	///�������
	virtual int FUNCTION_CALL_MODE GetheartBtInt() const = 0;
	///��һ������ʱ��
	virtual bool FUNCTION_CALL_MODE NeedHeartbeat() const = 0;
	///���һ�η���ʱ��
	virtual int FUNCTION_CALL_MODE GetlastSentTime() const = 0;
  	
	///���һ�ν���ʱ��
	virtual int FUNCTION_CALL_MODE GetlastReceivedTime() const = 0;

	///�ǳ�ԭ��
	virtual const char * FUNCTION_CALL_MODE GetlogoutReason() const = 0;

    unsigned long  FUNCTION_CALL_MODE QueryInterface(const int & iid, IKnownV1 **ppv ){return 0;}
    unsigned long  FUNCTION_CALL_MODE AddRef(){return 0;}
    unsigned long  FUNCTION_CALL_MODE Release(){return 0;}

	//	maoyinjie 2012/08/30	�������þ����Ƿ���־�ļ�,ȱʡ��
	virtual bool FUNCTION_CALL_MODE keeplog() const =0;
	virtual void FUNCTION_CALL_MODE keeplog(bool bkeeplog) =0;
};	


///HSFIXӦ�ûص��ӿڣ������ʹ����ʵ�ִ˽ӿڣ�������Ӧ���淢���ĸ����¼�	
/**
 * �����ʹ����ʵ�ִ˽ӿڣ����ڳ�ʼ��ʱ������ע��˽ӿڣ�
 * �ڽӿڶ�����¼��������У�����ͨ��SessionID��ȡ��Ӧ��Fix�Ự�ӿ�ISession;
 * ͨ���Ự�ӿڿ��Է��͸�����Ϣ(�ɻỰ���þ���FIX��Ϣ�������������Ϣ)��
 */
struct IApplication : public IKnownV1
{
	/// ��Ӧ�����ڲ��¼�
	virtual void FUNCTION_CALL_MODE onEvent(const ISessionID * lpSessionID, int iEventID) = 0;
	/// ��Ӧ�Ự���ӽ����¼�
	virtual void FUNCTION_CALL_MODE OnCreate(const ISessionID * lpSessionID ) = 0;
	/// ��Ӧ�Ự��¼�ɹ��¼�
	virtual void FUNCTION_CALL_MODE OnLogon(const ISessionID * lpSessionID ) = 0;
	/// ��Ӧ�Ự�ǳ��¼�
	virtual void FUNCTION_CALL_MODE OnLogout(const ISessionID * lpSessionID ) = 0;

	/// ��Ӧһ��ϵͳ��Ϣ�������͵��¼�
	/**ʹ�����ڴ��¼���ɶԵ�¼��Ϣ���ж��ƣ��û��������롢�Ƿ��������кŵȣ�
	*@param IMessage * lpMsg          �������͵�ϵͳ��Ϣ
	*@param ISessionID * lpSessionID  ������Ϣ�ĻỰID
	*/
	virtual void FUNCTION_CALL_MODE ToAdmin(IMessage * lpMsg, const ISessionID * lpSessionID ) = 0;

	/// ��Ӧһ��Ӧ����Ϣ�������͵��¼�
	/**ʹ���߿����ڴ�����¼�ʱ����Ҫ���͵���Ϣ���и��ֹ��˴���
	*@param IMessage * lpMsg          �������͵�Ӧ����Ϣ
	*@param ISessionID * lpSessionID  ������Ϣ�ĻỰID
	*@return �������淵�صĴ�����: DoNotSend
	*/
	virtual int FUNCTION_CALL_MODE ToApp(IMessage * lpMsg, const ISessionID * lpSessionID ) = 0;

	/// ��Ӧ�յ�һ��ϵͳ��Ϣ���¼�
	/**ϵͳ��Ϣһ��������ά����ʹ���߲��ضԴ��¼����й��ദ��
	* �����Ӧ���ڴ��¼�����ݵ�¼��Ϣ�Է�������ݽ�����֤�� 
	*@param IMessage * lpMsg          ���յ���ϵͳ��Ϣ
	*@param ISessionID * lpSessionID  �յ���Ϣ�ĻỰID  
	*@return �������淵�صĴ�����:FieldNotFound, IncorrectDataFormat, IncorrectTagValue, RejectLogon
	*/
	virtual int FUNCTION_CALL_MODE FromAdmin(const IMessage * lpMsg , const ISessionID * lpSessionID ) = 0;

	/// ��Ӧ�յ�һ��Ӧ��ϵͳ���¼�
	/**ʹ������Ҫ��Ӧ���¼�
	*@param IMessage * lpMsg          ���յ���Ӧ����Ϣ
	*@param ISessionID * lpSessionID  �յ���Ϣ�ĻỰID  
	*@return �������淵�صĴ�����:FieldNotFound, IncorrectDataFormat, IncorrectTagValue, UnsupportedMessageType
	*/
	virtual int FUNCTION_CALL_MODE FromApp(const IMessage * lpMsg , const ISessionID * lpSessionID ) = 0;


    unsigned long  FUNCTION_CALL_MODE QueryInterface(const int & iid, IKnownV1 **ppv ){return 0;}
    unsigned long  FUNCTION_CALL_MODE AddRef(){return 0;}
    unsigned long  FUNCTION_CALL_MODE Release(){return 0;}

};

///FIX�Ự�ӿڣ����ڷ�����Ϣ����ȡ�Ự�����Ϣ��
struct ISession : public IKnownV1
{
	/// ��ȡ��ǰ�Ự��SessionID
	virtual const ISessionID*	 FUNCTION_CALL_MODE GetSessionID() const = 0;
	
	/// ��ȡ��ǰ�Ự״̬
	virtual const ISessionState* FUNCTION_CALL_MODE GetStatus() const = 0;

	/// �ڵ�ǰ�Ự�Ϸ���һ����Ϣ
	virtual int FUNCTION_CALL_MODE SendMessage( IMessage * lpMessage ) = 0;

	virtual void* FUNCTION_CALL_MODE GetDataDictionary() = 0;


	//	2014/01/03	maoyinjie	���ӻ�ûỰ�ڲ�����/������ŵĽӿ�
	//	����Լ���һ�η��ͱ������
	virtual long FUNCTION_CALL_MODE getExpectedSenderNum()= 0;
	//	����Լ�ϣ�����յ���һ�������
	virtual long FUNCTION_CALL_MODE getExpectedTargetNum()=0;



    unsigned long  FUNCTION_CALL_MODE QueryInterface(const int & iid, IKnownV1 **ppv ){return 0;}
    unsigned long  FUNCTION_CALL_MODE AddRef(){return 0;}
    unsigned long  FUNCTION_CALL_MODE Release(){return 0;}


};

#ifdef __cplusplus
extern "C"{
#endif
	///��ʼ������
	/**���������ļ�����ʼ�����棬�������ӣ���¼�Ự
	* @param char * sCfg           �����ļ���
	* @param IApplication * lpApp  ������ע���Ӧ�ûص��ӿ�	
	* @return 0 �ɹ�
	*/
	FIXENGAPI int FUNCTION_CALL_MODE EngnInit(char* sCfg, IApplication* lpApp);
	///��ֹ����(�Զ��ǳ����Ͽ�����)
	FIXENGAPI void FUNCTION_CALL_MODE EngnDone();


	
	///���ݻỰID��ȡ�Ự�ӿ�
	/**
	 *@param ISessionID * �ỰID(�ɷ���ID��Ŀ�귽ID��Ŀ�귽�����޶������ )
	 *@return ISession * ��Ӧ�Ự�ӿ�
	 */
	FIXENGAPI ISession * FUNCTION_CALL_MODE GetSessionByID(ISessionID* lpSessionID );
	///ȡ�������õĻỰ��
	FIXENGAPI int FUNCTION_CALL_MODE GetSessionNum();
	///�������ȡ�Ự�ӿ�
	/**
	 *@param int iIndex  ���õĻỰ���(��ʼ���Ϊ:0)
	 *@return ISession * ��Ӧ�Ự�ӿ�	
	 */
	FIXENGAPI ISession * FUNCTION_CALL_MODE GetSessionByIndex( int iIndex );
	

	/// ����һ����Ϣ
	/**
	*@param char * lpBeginString	
	*@param char * lpMsgType		
	*/
	FIXENGAPI IMessage * FUNCTION_CALL_MODE CreateMessage( char * lpBeginString, char * lpMsgType );

	/// ������Ϣ�ַ��������ݴʵ䴴��һ����Ϣ
	/**
	*@param char * lpMsg			��Ϣ��	
	*@param char * lpSession	���ݴʵ�	
	*/
	FIXENGAPI IMessage * FUNCTION_CALL_MODE CreateMessageByString( char * lpMsg, void * lpSession );

	/// �ݻ�һ����Ϣ
	/**
	*@param IMessage* lpMessage	���ͷŵ���Ϣָ�� 
	*/
	FIXENGAPI void FUNCTION_CALL_MODE DestroyMessage( IMessage* lpMessage );
	
	///	����һ����Ϣ���ỰID��Ϣ�Ѿ���������Ϣ��
	FIXENGAPI int FUNCTION_CALL_MODE SendFixMessage(IMessage* lpMessage,	char* lpQualifier);
	///	���ݻỰID������Ϣ,��Ϣ����δ�����ỰID��Ϣ
	FIXENGAPI int FUNCTION_CALL_MODE SendMessageByID(IMessage* lpMessage, ISessionID* lpsessionID );


	//	maoyinjie 20111008	���ݻỰ������
	FIXENGAPI int FUNCTION_CALL_MODE SendMessageBySessionName(IMessage* lpMessage, const char* lpsessionName );

	//	maoyinjie 201203/29	������Ϣ�����ӿ�
	/**
	*@param IMessage* lpMessage	Դ��Ϣָ�� 
	*@param void * lpDataDictionary	����Ϣʹ�õ����ݴʵ�,���Դ�ISession�ӿڻ�����ݴʵ�ӿ�
	*/
	FIXENGAPI IMessage * FUNCTION_CALL_MODE CopyMessage( IMessage* lpMessage, ISession* lpSession);

	//	maoyinjie 20120911	�����������ĻỰ���޵Ľӿ�
	/*
		����˵��:
		iMaxAcceptor:		����������ı����Ự,	0:	������
		iMaxInitiator:		����������������Ự	0:	������
		ʹ�÷���:
			�ڵ���EngnInit����֮ǰ�ȵ���SetSessionLimit,�����ûỰ����,��������,
			ȱʡ������
	*/
	FIXENGAPI void FUNCTION_CALL_MODE SetSessionLimit(int iMaxAcceptor, int iMaxInitiator);
#ifdef __cplusplus
}
#endif


#endif //HSFIX_APPLICATION_H
