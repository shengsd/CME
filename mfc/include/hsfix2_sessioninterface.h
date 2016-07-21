/****************************************************************************
  Դ��������:hsfix2_messageinterface.h
  �������Ȩ:�������ӹɷ����޹�˾
  ϵͳ����:  HSFIX20
  ģ������:  �ӿ��ļ�
  ����˵��:  
			����FIX����Ự���ƽӿ�
				
  ��    ��:  maoyj
  ��������:  20140415
  ��    ע:  
  

	// 20160118	zhouwh �ڲ��������޸Ļ�ȡ�Ự�ӿڶ���
	//	20160118	zhouwh ֧�ֶ�̬�ݻٻỰ
**/
  
#ifndef __HSFIX20_SESSION_INTERFACE__H__
#define __HSFIX20_SESSION_INTERFACE__H__
#include "hsfix2_error.h"
#include "hsfix2_messageinterface.h"
#include "hsfix2_config_interface.h"


struct ISession;

//////////////////////////////////////////////////////////////////////////
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
};


/// �Ự״̬
struct ISessionState : public IFIXBase
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
	virtual const char * FUNCTION_CALL_MODE GetlogoutReason()  = 0;
};	
//////////////////////////////////////////////////////////////////////////



///	HSFIXӦ�ûص��ӿڣ������ʹ����ʵ�ִ˽ӿڣ�������Ӧ���淢���ĸ����¼�	
/**
 * �����ʹ����ʵ�ִ˽ӿڣ����ڻỰ����ʱע��˽ӿڣ�
 * �ڽӿڶ�����¼��������У�;
 * ͨ���Ự�ӿڿ��Է��͸�����Ϣ
 *	ͨ���ص��յ�����Ϣ,�Ѿ�������Ϣ�Ϸ���У��,ȷ����Ϣ���Ϲ淶
 */
struct IApplication : public IFIXBase
{
//	virtual ~IApplication(){};

	/// ��Ӧ�����ڲ��¼�
	virtual void FUNCTION_CALL_MODE OnEvent(const ISession * lpSession, int iEventID) = 0;
	/// ��Ӧ�Ự���ӽ����¼�
	virtual void FUNCTION_CALL_MODE OnCreate(const ISession * lpSession ) = 0;
	/// ��Ӧ�Ự��¼�ɹ��¼�
	virtual void FUNCTION_CALL_MODE OnLogon( ISession * lpSession ) = 0;
	/// ��Ӧ�Ự�ǳ��¼�
	virtual void FUNCTION_CALL_MODE OnLogout( ISession * lpSession ) = 0;

	/// ��Ӧһ��ϵͳ��Ϣ�������͵��¼�
	/**ʹ�����ڴ��¼���ɶԵ�¼��Ϣ���ж��ƣ��û��������롢�Ƿ��������кŵȣ�
	*@param IMessage * lpMsg          �������͵�ϵͳ��Ϣ
	*@param ISession * lpSession  ������Ϣ�ĻỰ
	*/
	virtual void FUNCTION_CALL_MODE ToAdmin(IMessage * lpMsg,  ISession * lpSession ) = 0;

	/// ��Ӧһ��Ӧ����Ϣ�������͵��¼�
	/**ʹ���߿����ڴ�����¼�ʱ����Ҫ���͵���Ϣ���и��ֹ��˴���
	*@param IMessage * lpMsg          �������͵�Ӧ����Ϣ
	*@param ISession * lpSession  ������Ϣ�ĻỰ
	*@return �������淵�صĴ�����: ERR_DoNotSend
	*/
	virtual int FUNCTION_CALL_MODE ToApp(IMessage * lpMsg,  ISession * lpSession ) = 0;

	/// ��Ӧ�յ�һ��ϵͳ��Ϣ���¼�
	/**ϵͳ��Ϣһ��������ά����ʹ���߲��ضԴ��¼����й��ദ��
	* �����Ӧ���ڴ��¼�����ݵ�¼��Ϣ�Է�������ݽ�����֤�� 
	*@param IMessage * lpMsg          ���յ���ϵͳ��Ϣ
	*@param ISession * lpSession  �յ���Ϣ�ĻỰ 
	*@return �������淵�صĴ�����:ERR_FieldNotFound, ERR_IncorrectDataFormat, ERR_IncorrectTagValue, ERR_RejectLogon
	*/
	virtual int FUNCTION_CALL_MODE FromAdmin(IMessage * lpMsg ,  ISession * lpSession ) = 0;

	/// ��Ӧ�յ�һ��Ӧ��ϵͳ���¼�
	/**ʹ������Ҫ��Ӧ���¼�
	*@param IMessage * lpMsg          ���յ���Ӧ����Ϣ
	*@param ISession * lpSession  �յ���Ϣ�ĻỰ 
	*@return �������淵�صĴ�����:ERR_FieldNotFound, ERR_IncorrectDataFormat, ERR_IncorrectTagValue, ERR_UnsupportedMessageType
	*/
	virtual int FUNCTION_CALL_MODE FromApp(IMessage * lpMsg ,  ISession * lpSession ) = 0;
	unsigned long  FUNCTION_CALL_MODE QueryInterface(const int & iid, void **ppv )
	{
		return 0;
	}
	unsigned long  FUNCTION_CALL_MODE AddRef()
	{
		return 0;
	}
	unsigned long  FUNCTION_CALL_MODE Release()
	{
		return 0;
	}
};

///FIX�Ự�ӿڣ����ڷ�����Ϣ����ȡ�Ự�����Ϣ��
struct ISession : public IFIXBase
{
	virtual ~ISession(){};

	/// ��ȡ��ǰ�Ự��
	/**ʹ������Ҫ��Ӧ���¼�
	*@return ���Ự��
	*/
	virtual const char*	FUNCTION_CALL_MODE GetSessionName() const = 0;
	/// �ڵ�ǰ�Ự�Ϸ���һ����Ϣ
	/**ʹ������Ҫ��Ӧ���¼�
	*@param IMessage * lpMsg   ��Ҫ���͵���Ϣ,��Ϣ������Ȩ�ᷢ��ת��,���ñ�������,�û������ٶ�lpMessage���κβ���
	*@return 0: �ɹ�  ����:ʧ��
	*/
	virtual int FUNCTION_CALL_MODE SendFixMessage( IMessage * lpMessage ) = 0;

	//	�ӻỰ�л�ȡ��Ϣ(ÿ���Ự��ĳ��������Ϣ����Ϣ����)
	/**	����һ������Ϣ������ƥ�����Ϣ
		lpMsgType:	��Ϣ������
		�Ự�ڲ���������õ����ݴʵ�,���ɲ�ͬģ�����Ϣ
	**/
	virtual IMessage* FUNCTION_CALL_MODE CreateMsgByType(const char* lpMsgType) = 0;
	/// ��÷�����id
	virtual const char* FUNCTION_CALL_MODE getSenderID()const=0;
	/// ���Ŀ�ĵ�id
	virtual const char* FUNCTION_CALL_MODE getTargetID()const=0;
	/// ���Э��汾
	virtual const char* FUNCTION_CALL_MODE getBeginString()const=0;
	/// ��ûỰ��һ�޶��ı�ʶ
	virtual const ISessionID* FUNCTION_CALL_MODE GetSessionID() const =0;
	/// ��ȡ��ǰ�Ự״̬
	virtual const ISessionState* FUNCTION_CALL_MODE GetStatus() const =0;

	///	�����๦��,��ûỰ��Ϣ
	//	����Լ���һ�η��ͱ������
	virtual long FUNCTION_CALL_MODE getExpectedSenderNum()= 0;
	//	����Լ�ϣ�����յ���һ�������
	virtual long FUNCTION_CALL_MODE getExpectedTargetNum()=0;



	/// �����ڲ���ά����ȷ���շ����,�û����������������ӿ�!!!�������³���ʹ��:
	/*
		1:	�������,Ϊ��֤�������ŵ���ȷ����,������Ӧ�ò�������ô������
		2:	ĳЩ���ⳡ����,���»Ự˫������޷�ͬ��ʱ,ʹ�ñ��ӿ�ͬ�����
		�κηǱ��볡����ʹ���������ӿڻᵼ�������ڲ�״̬���ش���!!!!!!
	*/
	//	���������ӿڣ��ɹ�����0
	//	�����Լ���һ�η��ͱ������	
	virtual long FUNCTION_CALL_MODE SetExpectedSenderNum(int iMsgSeq)= 0;
	//	�����Լ�ϣ�����յ���һ�������
	virtual long FUNCTION_CALL_MODE SetExpectedTargetNum(int iMsgSeq)=0;
};

//	���ﻹ�Ǵ���һ������,����Ự����ͬ,��������ͻ,���Ը���ģ��ʹ�õĻỰ������Ψһ
struct ISessionFactory : public IFIXBase
{
	//	���������ļ�.��ʼ��һ���Ự,�������ͬ�����ļ�����ʼ��.��ô��?
	/**
	*@param const char* lpCfgName         �����ļ���
	*@param const IApplication* lpApp  	  �ص�����
	*@param int MaxSessions				  �������ļ���,���ֻ��������MaxSessions���Ự(ĿǰʧЧ����SetSessionLimits������
	*@return ����0�����ɹ�	��������Ӧ������
	��Ϊ: ���������ļ����Դ����Ự
	*/	
	virtual int	FUNCTION_CALL_MODE InitSessions(const char* lpCfgName, IApplication* lpApp, int MaxSessions) = 0;

	///	��������CreateSession�ӿ���������ʼ���Ự,��������InitSessions��������
	///	���������ļ�,�����ض��Ự
	/**
	*@param const char* lpSessionName     �ض��Ự��
	*@param const char* lpCfgName         �����ļ���
	*@param const IApplication* lpApp  		�ص�����
	*@return ���ش����ĻỰ����
	��Ϊ: ���������ļ����Դ����Ự,һ����˵,������ֵ�lpCfgNameӦ�ú�ǰ���InitSessions�����е�lpCfgNameӦ�ò�ͬ
	*/	
	virtual ISession*	FUNCTION_CALL_MODE CreateSession(const char* lpSessionName, const char* lpCfgName, IApplication* lpApp) = 0;
	///	���������ļ�,�����ض��Ự
	/**
	*@param const char* lpSessionName     �ض��Ự��
	*@param IHSFIXConfig* lpCfg			  ���ýӿ�
	*@param const IApplication* lpApp  	  �ص�����
	*@return ���ش����ĻỰ����
	��Ϊ: ���������ļ����Դ����Ự,һ����˵,������ֵ�lpCfgNameӦ�ú�ǰ���InitSessions�����е�lpCfgNameӦ�ò�ͬ
	*/	
	virtual ISession*	FUNCTION_CALL_MODE CreateSessionX(const char* lpSessionName, IHSFIXConfig* lpCfg, IApplication* lpApp) = 0;
	
	///	�����Ự
	virtual int FUNCTION_CALL_MODE GetSessionNum() const = 0;

	// �������������ӿ�ʱ������ɹ���ȡ���Ự�����Ѿ������˸ûỰ�����ü���������ʹ����֮�������ûỰ��Release�ӿڼ������ü�����
	// ����DestroySession�ӿڶ�̬ɾ���ûỰʱ���޷������ڴ棬����ڴ�й¶��������
	virtual ISession* FUNCTION_CALL_MODE GetSessionByIndex(int iIndex) const = 0;
	virtual ISession* FUNCTION_CALL_MODE GetSessionByName(char* lpSessionName) const = 0;

	/////////////////////////////////////////////////////////////
	//	20160118	zhouwh ֧�ֶ�̬�ݻٻỰ
	virtual int FUNCTION_CALL_MODE DestroySession(ISession* lpSession) = 0;
	virtual int FUNCTION_CALL_MODE DestroySessionByName(const char* lpSessionName) = 0;

	///	���³�ʼ��,��������,ֻ������ñ䶯�ĻỰ,��������   (��δ֧��)
	/**
	*@param const char* lpCfgName         �����ļ���
	*@param const IApplication* lpApp  		�ص�����
	*@param int MaxSessions					  		�������ļ���,���ֻ��������MaxSessions���Ự
	*@return ���ش����ĻỰ����
	��Ϊ: ��������ļ��еĸ����Ự,����Ự���������֮ǰ����ʱ,�б䶯,��ݻٻỰ,���´���
				��������г��������ĻỰ,���´����Ự,
				����ھɵ������ļ��д��ڵĻỰ,�����ļ��в�����,��ݻٻỰ
				�����һ���������ı�,��ݻ����о������������ĻỰ,��������,�ȴ��Զ��ؽ��Ự
				��������Ӱ���� InitSessions �����ĻỰ,����Ӱ����CreateSession��CreateSessionX�����������ĻỰ
	*/	
	virtual int	FUNCTION_CALL_MODE ReInitSessions(const char* lpCfgName, IApplication* lpApp, int MaxSessions) = 0;
};




#ifdef __cplusplus
extern "C"{
#endif
//	���º���,���ü�������.Ӧ�ÿ����߿������ʹ��FIX��ģ��ʱ,���Զ�������
FIXENGAPI ISessionFactory* FUNCTION_CALL_MODE CreateSessionFactory();
FIXENGAPI void FUNCTION_CALL_MODE DestroySessionFactory();
FIXENGAPI const char* FUNCTION_CALL_MODE GetFixErrorMsg(int iErrorNo);
//	��Ӧ�ò����ã�������������������ĻỰ����������ӿڣ��͵�������Ự�ӿڵĳ���ì�ܣ�
//	ǰ���������ǻ��ڸ���ģ������������ʹ��fix��������󣬶�û��һ��ͳһ��Э��
//	���������ûỰ���ޣ���Ȼ�Ǵ���һ��ͳһ��Э��������
FIXENGAPI int FUNCTION_CALL_MODE SetSessionLimits(int iMaxSessions);
#ifdef __cplusplus
}
#endif

#endif	//	__HSFIX20_SESSION_INTERFACE__H__
