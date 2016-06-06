///HSFIX引擎对外接口定义，支持FIX4.0~4.4及三方存管协议
/**本引擎适用于作为单客户端、多客户端、服务端应用、客户端服务端混合应用;
 * 引擎根据配置文件，维护多个会话，每个会话的类型(C/S)、所用协议等可以不同；
 * 引擎向使用者屏蔽了会话类型、所用协议等差异；
 * 包括：
 * (1)引擎库引出函数，注册接口、查询会话	
 * (2)引擎库应用回调接口IApplication，用于响应引擎的各类应用事件；
 * (3)引擎会话接口ISession，用于创建与发送消息、获取会话状态等；

 *	注意事项：HSFixEngApi	V1.0底层是单线程实现，用户在IApplication
 *	接口实现中,必须尽快返回,在回调函数中,避免进行长时间的运算或者
 *	等待,以防止影响到引擎的性能.
 *	在2.0版本的引擎底层将会支持多线程,所以,用户代码必须考虑到线程安全问题.
 *	这样引擎API版本升级后,老的应用可以继续使用.

  //	maoyinjie 20111008	根据会话名发送
	//	maoyinjie 201203/29	增加消息拷贝接口
	maoyinjie 20120911	增加允许创建的会话上限的接口
	2014/01/03	maoyinjie	增加获得会话内部接收/发送序号的接口		版本升级成2.0.0.4

 */
 
#ifndef HSFIX_APPLICATION_H
#define HSFIX_APPLICATION_H

#include "MessageInterface.h"        //消息打包解包接口定义

/// 会话ID
/**
 * 一个会话ID由前缀串, 发起方ID，目标方ID，目标方附加限定符四部分内容组成；
 * 目标方附加限定符用于区分于同一目标方的不同会话；
 *
 * 引擎通过会话ID来标识一个会话，使用者通过会话ID来获取匹配的会话接口;
 *
 */
struct ISessionID
{
	///前缀串		
	const char* lpBeginString;
	///发起方ID
	const char* lpSenderCompID;
	///目标方ID
	const char* lpTargetCompID;
	///目标方附加限定符
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

///会话状态
struct ISessionState : public IKnownV1
{
	/// 会话是否有效
	virtual bool FUNCTION_CALL_MODE IsEnabled( ) const = 0;
  
	///当前会话是否已登录
	virtual bool FUNCTION_CALL_MODE IsLoggedOn() const = 0;

	///是否是发起者
	virtual bool FUNCTION_CALL_MODE IsInitiate() const = 0;

	///登录时延
	virtual int FUNCTION_CALL_MODE LogonTimeout() const = 0;

	///登出时延
	virtual int FUNCTION_CALL_MODE LogoutTimeout() const = 0;

	///心跳间隔
	virtual int FUNCTION_CALL_MODE GetheartBtInt() const = 0;
	///下一次心跳时间
	virtual bool FUNCTION_CALL_MODE NeedHeartbeat() const = 0;
	///最后一次发送时间
	virtual int FUNCTION_CALL_MODE GetlastSentTime() const = 0;
  	
	///最后一次接收时间
	virtual int FUNCTION_CALL_MODE GetlastReceivedTime() const = 0;

	///登出原因
	virtual const char * FUNCTION_CALL_MODE GetlogoutReason() const = 0;

    unsigned long  FUNCTION_CALL_MODE QueryInterface(const int & iid, IKnownV1 **ppv ){return 0;}
    unsigned long  FUNCTION_CALL_MODE AddRef(){return 0;}
    unsigned long  FUNCTION_CALL_MODE Release(){return 0;}

	//	maoyinjie 2012/08/30	根据配置决定是否换日志文件,缺省换
	virtual bool FUNCTION_CALL_MODE keeplog() const =0;
	virtual void FUNCTION_CALL_MODE keeplog(bool bkeeplog) =0;
};	


///HSFIX应用回调接口，引擎的使用者实现此接口，用于响应引擎发生的各类事件	
/**
 * 引擎的使用者实现此接口，并在初始化时向引擎注册此接口；
 * 在接口定义的事件处理函数中，可以通过SessionID获取相应的Fix会话接口ISession;
 * 通过会话接口可以发送各类消息(由会话配置决定FIX消息还是三方存管消息)；
 */
struct IApplication : public IKnownV1
{
	/// 响应引擎内部事件
	virtual void FUNCTION_CALL_MODE onEvent(const ISessionID * lpSessionID, int iEventID) = 0;
	/// 响应会话连接建立事件
	virtual void FUNCTION_CALL_MODE OnCreate(const ISessionID * lpSessionID ) = 0;
	/// 响应会话登录成功事件
	virtual void FUNCTION_CALL_MODE OnLogon(const ISessionID * lpSessionID ) = 0;
	/// 响应会话登出事件
	virtual void FUNCTION_CALL_MODE OnLogout(const ISessionID * lpSessionID ) = 0;

	/// 响应一个系统消息将被发送的事件
	/**使用者在此事件里可对登录消息进行定制（用户名、密码、是否重置序列号等）
	*@param IMessage * lpMsg          将被发送的系统消息
	*@param ISessionID * lpSessionID  发送消息的会话ID
	*/
	virtual void FUNCTION_CALL_MODE ToAdmin(IMessage * lpMsg, const ISessionID * lpSessionID ) = 0;

	/// 响应一个应用消息将被发送的事件
	/**使用者可以在处理此事件时，对要发送的消息进行各种过滤处理。
	*@param IMessage * lpMsg          将被发送的应用消息
	*@param ISessionID * lpSessionID  发送消息的会话ID
	*@return 可向引擎返回的错误码: DoNotSend
	*/
	virtual int FUNCTION_CALL_MODE ToApp(IMessage * lpMsg, const ISessionID * lpSessionID ) = 0;

	/// 响应收到一个系统消息的事件
	/**系统消息一般由引擎维护，使用者不必对此事件进行过多处理。
	* 服务端应用在此事件里，根据登录消息对发起者身份进行认证。 
	*@param IMessage * lpMsg          将收到的系统消息
	*@param ISessionID * lpSessionID  收到消息的会话ID  
	*@return 可向引擎返回的错误码:FieldNotFound, IncorrectDataFormat, IncorrectTagValue, RejectLogon
	*/
	virtual int FUNCTION_CALL_MODE FromAdmin(const IMessage * lpMsg , const ISessionID * lpSessionID ) = 0;

	/// 响应收到一个应用系统的事件
	/**使用者主要响应的事件
	*@param IMessage * lpMsg          将收到的应用消息
	*@param ISessionID * lpSessionID  收到消息的会话ID  
	*@return 可向引擎返回的错误码:FieldNotFound, IncorrectDataFormat, IncorrectTagValue, UnsupportedMessageType
	*/
	virtual int FUNCTION_CALL_MODE FromApp(const IMessage * lpMsg , const ISessionID * lpSessionID ) = 0;


    unsigned long  FUNCTION_CALL_MODE QueryInterface(const int & iid, IKnownV1 **ppv ){return 0;}
    unsigned long  FUNCTION_CALL_MODE AddRef(){return 0;}
    unsigned long  FUNCTION_CALL_MODE Release(){return 0;}

};

///FIX会话接口，用于发送消息、获取会话相关信息等
struct ISession : public IKnownV1
{
	/// 获取当前会话的SessionID
	virtual const ISessionID*	 FUNCTION_CALL_MODE GetSessionID() const = 0;
	
	/// 获取当前会话状态
	virtual const ISessionState* FUNCTION_CALL_MODE GetStatus() const = 0;

	/// 在当前会话上发送一个消息
	virtual int FUNCTION_CALL_MODE SendMessage( IMessage * lpMessage ) = 0;

	virtual void* FUNCTION_CALL_MODE GetDataDictionary() = 0;


	//	2014/01/03	maoyinjie	增加获得会话内部接收/发送序号的接口
	//	获得自己下一次发送报文序号
	virtual long FUNCTION_CALL_MODE getExpectedSenderNum()= 0;
	//	获得自己希望接收的下一报文序号
	virtual long FUNCTION_CALL_MODE getExpectedTargetNum()=0;



    unsigned long  FUNCTION_CALL_MODE QueryInterface(const int & iid, IKnownV1 **ppv ){return 0;}
    unsigned long  FUNCTION_CALL_MODE AddRef(){return 0;}
    unsigned long  FUNCTION_CALL_MODE Release(){return 0;}


};

#ifdef __cplusplus
extern "C"{
#endif
	///初始化引擎
	/**根据配置文件，初始化引擎，建立连接，登录会话
	* @param char * sCfg           配置文件名
	* @param IApplication * lpApp  向引擎注册的应用回调接口	
	* @return 0 成功
	*/
	FIXENGAPI int FUNCTION_CALL_MODE EngnInit(char* sCfg, IApplication* lpApp);
	///终止引擎(自动登出、断开连接)
	FIXENGAPI void FUNCTION_CALL_MODE EngnDone();


	
	///根据会话ID获取会话接口
	/**
	 *@param ISessionID * 会话ID(由发起方ID，目标方ID，目标方附加限定符组成 )
	 *@return ISession * 相应会话接口
	 */
	FIXENGAPI ISession * FUNCTION_CALL_MODE GetSessionByID(ISessionID* lpSessionID );
	///取引擎配置的会话数
	FIXENGAPI int FUNCTION_CALL_MODE GetSessionNum();
	///根据序号取会话接口
	/**
	 *@param int iIndex  配置的会话序号(起始序号为:0)
	 *@return ISession * 相应会话接口	
	 */
	FIXENGAPI ISession * FUNCTION_CALL_MODE GetSessionByIndex( int iIndex );
	

	/// 创建一个消息
	/**
	*@param char * lpBeginString	
	*@param char * lpMsgType		
	*/
	FIXENGAPI IMessage * FUNCTION_CALL_MODE CreateMessage( char * lpBeginString, char * lpMsgType );

	/// 根据消息字符串和数据词典创建一个消息
	/**
	*@param char * lpMsg			消息体	
	*@param char * lpSession	数据词典	
	*/
	FIXENGAPI IMessage * FUNCTION_CALL_MODE CreateMessageByString( char * lpMsg, void * lpSession );

	/// 摧毁一个消息
	/**
	*@param IMessage* lpMessage	待释放的消息指针 
	*/
	FIXENGAPI void FUNCTION_CALL_MODE DestroyMessage( IMessage* lpMessage );
	
	///	发送一个消息，会话ID信息已经包含在消息中
	FIXENGAPI int FUNCTION_CALL_MODE SendFixMessage(IMessage* lpMessage,	char* lpQualifier);
	///	根据会话ID发送消息,消息中尚未包含会话ID信息
	FIXENGAPI int FUNCTION_CALL_MODE SendMessageByID(IMessage* lpMessage, ISessionID* lpsessionID );


	//	maoyinjie 20111008	根据会话名发送
	FIXENGAPI int FUNCTION_CALL_MODE SendMessageBySessionName(IMessage* lpMessage, const char* lpsessionName );

	//	maoyinjie 201203/29	增加消息拷贝接口
	/**
	*@param IMessage* lpMessage	源消息指针 
	*@param void * lpDataDictionary	该消息使用的数据词典,可以从ISession接口获得数据词典接口
	*/
	FIXENGAPI IMessage * FUNCTION_CALL_MODE CopyMessage( IMessage* lpMessage, ISession* lpSession);

	//	maoyinjie 20120911	增加允许创建的会话上限的接口
	/*
		参数说明:
		iMaxAcceptor:		最大允许创建的被动会话,	0:	无限制
		iMaxInitiator:		最大允许创建的主动会话	0:	无限制
		使用方法:
			在调用EngnInit函数之前先调用SetSessionLimit,以设置会话上限,若不调用,
			缺省无上限
	*/
	FIXENGAPI void FUNCTION_CALL_MODE SetSessionLimit(int iMaxAcceptor, int iMaxInitiator);
#ifdef __cplusplus
}
#endif


#endif //HSFIX_APPLICATION_H
