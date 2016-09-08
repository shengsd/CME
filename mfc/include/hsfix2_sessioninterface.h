/****************************************************************************
  源程序名称:hsfix2_messageinterface.h
  软件著作权:恒生电子股份有限公司
  系统名称:  HSFIX20
  模块名称:  接口文件
  功能说明:  
			恒生FIX引擎会话控制接口
				
  作    者:  maoyj
  开发日期:  20140415
  备    注:  
  

	// 20160118	zhouwh 内部加锁，修改获取会话接口定义
	//	20160118	zhouwh 支持动态摧毁会话
**/
  
#ifndef __HSFIX20_SESSION_INTERFACE__H__
#define __HSFIX20_SESSION_INTERFACE__H__
#include "hsfix2_error.h"
#include "hsfix2_messageinterface.h"
#include "hsfix2_config_interface.h"


struct ISession;

//////////////////////////////////////////////////////////////////////////
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
};


/// 会话状态
struct ISessionState : public IFIXBase
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
	virtual const char * FUNCTION_CALL_MODE GetlogoutReason()  = 0;
};	
//////////////////////////////////////////////////////////////////////////



///	HSFIX应用回调接口，引擎的使用者实现此接口，用于响应引擎发生的各类事件	
/**
 * 引擎的使用者实现此接口，并在会话创建时注册此接口；
 * 在接口定义的事件处理函数中，;
 * 通过会话接口可以发送各类消息
 *	通过回调收到的消息,已经经过消息合法性校验,确保消息符合规范
 */
struct IApplication : public IFIXBase
{
//	virtual ~IApplication(){};

	/// 响应引擎内部事件
	virtual void FUNCTION_CALL_MODE OnEvent(const ISession * lpSession, int iEventID) = 0;
	/// 响应会话连接建立事件
	virtual void FUNCTION_CALL_MODE OnCreate(const ISession * lpSession ) = 0;
	/// 响应会话登录成功事件
	virtual void FUNCTION_CALL_MODE OnLogon( ISession * lpSession ) = 0;
	/// 响应会话登出事件
	virtual void FUNCTION_CALL_MODE OnLogout( ISession * lpSession ) = 0;

	/// 响应一个系统消息将被发送的事件
	/**使用者在此事件里可对登录消息进行定制（用户名、密码、是否重置序列号等）
	*@param IMessage * lpMsg          将被发送的系统消息
	*@param ISession * lpSession  发送消息的会话
	*/
	virtual void FUNCTION_CALL_MODE ToAdmin(IMessage * lpMsg,  ISession * lpSession ) = 0;

	/// 响应一个应用消息将被发送的事件
	/**使用者可以在处理此事件时，对要发送的消息进行各种过滤处理。
	*@param IMessage * lpMsg          将被发送的应用消息
	*@param ISession * lpSession  发送消息的会话
	*@return 可向引擎返回的错误码: ERR_DoNotSend
	*/
	virtual int FUNCTION_CALL_MODE ToApp(IMessage * lpMsg,  ISession * lpSession ) = 0;

	/// 响应收到一个系统消息的事件
	/**系统消息一般由引擎维护，使用者不必对此事件进行过多处理。
	* 服务端应用在此事件里，根据登录消息对发起者身份进行认证。 
	*@param IMessage * lpMsg          将收到的系统消息
	*@param ISession * lpSession  收到消息的会话 
	*@return 可向引擎返回的错误码:ERR_FieldNotFound, ERR_IncorrectDataFormat, ERR_IncorrectTagValue, ERR_RejectLogon
	*/
	virtual int FUNCTION_CALL_MODE FromAdmin(IMessage * lpMsg ,  ISession * lpSession ) = 0;

	/// 响应收到一个应用系统的事件
	/**使用者主要响应的事件
	*@param IMessage * lpMsg          将收到的应用消息
	*@param ISession * lpSession  收到消息的会话 
	*@return 可向引擎返回的错误码:ERR_FieldNotFound, ERR_IncorrectDataFormat, ERR_IncorrectTagValue, ERR_UnsupportedMessageType
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

///FIX会话接口，用于发送消息、获取会话相关信息等
struct ISession : public IFIXBase
{
	virtual ~ISession(){};

	/// 获取当前会话名
	/**使用者主要响应的事件
	*@return 本会话名
	*/
	virtual const char*	FUNCTION_CALL_MODE GetSessionName() const = 0;
	/// 在当前会话上发送一个消息
	/**使用者主要响应的事件
	*@param IMessage * lpMsg   需要发送的消息,消息的所有权会发生转移,调用本函数后,用户不能再对lpMessage做任何操作
	*@return 0: 成功  其他:失败
	*/
	virtual int FUNCTION_CALL_MODE SendFixMessage( IMessage * lpMessage ) = 0;

	//	从会话中获取消息(每个会话就某类特殊消息的消息工厂)
	/**	创建一个和消息类型所匹配的消息
		lpMsgType:	消息的类型
		会话内部会根据配置的数据词典,生成不同模版的消息
	**/
	virtual IMessage* FUNCTION_CALL_MODE CreateMsgByType(const char* lpMsgType) = 0;
	/// 获得发送者id
	virtual const char* FUNCTION_CALL_MODE getSenderID()const=0;
	/// 获得目的地id
	virtual const char* FUNCTION_CALL_MODE getTargetID()const=0;
	/// 获得协议版本
	virtual const char* FUNCTION_CALL_MODE getBeginString()const=0;
	/// 获得会话独一无二的标识
	virtual const ISessionID* FUNCTION_CALL_MODE GetSessionID() const =0;
	/// 获取当前会话状态
	virtual const ISessionState* FUNCTION_CALL_MODE GetStatus() const =0;

	///	管理类功能,获得会话信息
	//	获得自己下一次发送报文序号
	virtual long FUNCTION_CALL_MODE getExpectedSenderNum()= 0;
	//	获得自己希望接收的下一报文序号
	virtual long FUNCTION_CALL_MODE getExpectedTargetNum()=0;



	/// 引擎内部会维护正确的收发序号,用户切切慎用以下两接口!!!仅在以下场景使用:
	/*
		1:	引擎测试,为验证引擎对序号的正确处理,可以在应用层故意设置错误序号
		2:	某些特殊场景下,导致会话双方序号无法同步时,使用本接口同步序号
		任何非必须场景下使用以下两接口会导致引擎内部状态严重错误!!!!!!
	*/
	//	以下两个接口，成功返回0
	//	设置自己下一次发送报文序号	
	virtual long FUNCTION_CALL_MODE SetExpectedSenderNum(int iMsgSeq)= 0;
	//	设置自己希望接收的下一报文序号
	virtual long FUNCTION_CALL_MODE SetExpectedTargetNum(int iMsgSeq)=0;
};

//	这里还是存在一个问题,如果会话名相同,则会产生冲突,所以各个模块使用的会话名必须唯一
struct ISessionFactory : public IFIXBase
{
	//	根据配置文件.初始化一批会话,如果以相同配置文件来初始化.怎么办?
	/**
	*@param const char* lpCfgName         配置文件名
	*@param const IApplication* lpApp  	  回调对象
	*@param int MaxSessions				  该配置文件中,最多只允许配置MaxSessions个会话(目前失效，由SetSessionLimits决定）
	*@return 返回0创建成功	其他：相应错误码
	行为: 按照配置文件属性创建会话
	*/	
	virtual int	FUNCTION_CALL_MODE InitSessions(const char* lpCfgName, IApplication* lpApp, int MaxSessions) = 0;

	///	以下两个CreateSession接口允许单独初始化会话,和上述的InitSessions互不干扰
	///	根据配置文件,创建特定会话
	/**
	*@param const char* lpSessionName     特定会话名
	*@param const char* lpCfgName         配置文件名
	*@param const IApplication* lpApp  		回调对象
	*@return 返回创建的会话个数
	行为: 按照配置文件属性创建会话,一般来说,这里出现的lpCfgName应该和前面的InitSessions方法中的lpCfgName应该不同
	*/	
	virtual ISession*	FUNCTION_CALL_MODE CreateSession(const char* lpSessionName, const char* lpCfgName, IApplication* lpApp) = 0;
	///	根据配置文件,创建特定会话
	/**
	*@param const char* lpSessionName     特定会话名
	*@param IHSFIXConfig* lpCfg			  配置接口
	*@param const IApplication* lpApp  	  回调对象
	*@return 返回创建的会话个数
	行为: 按照配置文件属性创建会话,一般来说,这里出现的lpCfgName应该和前面的InitSessions方法中的lpCfgName应该不同
	*/	
	virtual ISession*	FUNCTION_CALL_MODE CreateSessionX(const char* lpSessionName, IHSFIXConfig* lpCfg, IApplication* lpApp) = 0;
	
	///	遍历会话
	virtual int FUNCTION_CALL_MODE GetSessionNum() const = 0;

	// 调用以下两个接口时，如果成功获取到会话，则已经增加了该会话的引用计数，所以使用完之后必须调用会话的Release接口减少引用计数，
	// 否则，DestroySession接口动态删除该会话时将无法回收内存，造成内存泄露！！！！
	virtual ISession* FUNCTION_CALL_MODE GetSessionByIndex(int iIndex) const = 0;
	virtual ISession* FUNCTION_CALL_MODE GetSessionByName(char* lpSessionName) const = 0;

	/////////////////////////////////////////////////////////////
	//	20160118	zhouwh 支持动态摧毁会话
	virtual int FUNCTION_CALL_MODE DestroySession(ISession* lpSession) = 0;
	virtual int FUNCTION_CALL_MODE DestroySessionByName(const char* lpSessionName) = 0;

	///	重新初始化,会检查配置,只针对配置变动的会话,重新启动   (暂未支持)
	/**
	*@param const char* lpCfgName         配置文件名
	*@param const IApplication* lpApp  		回调对象
	*@param int MaxSessions					  		该配置文件中,最多只允许配置MaxSessions个会话
	*@return 返回创建的会话个数
	行为: 检查配置文件中的各个会话,如果会话属性如果和之前创建时,有变动,则摧毁会话,重新创建
				如果配置中出现新增的会话,则新创建会话,
				如果在旧的配置文件中存在的会话,在新文件中不存在,则摧毁会话
				如果是一个侦听被改变,则摧毁所有经本侦听建立的会话,重置侦听,等待对端重建会话
				本方法仅影响由 InitSessions 创建的会话,并不影响由CreateSession和CreateSessionX所单独创建的会话
	*/	
	virtual int	FUNCTION_CALL_MODE ReInitSessions(const char* lpCfgName, IApplication* lpApp, int MaxSessions) = 0;
};




#ifdef __cplusplus
extern "C"{
#endif
//	以下函数,引用计数保护.应用开发者开发多个使用FIX的模块时,可以独立开发
FIXENGAPI ISessionFactory* FUNCTION_CALL_MODE CreateSessionFactory();
FIXENGAPI void FUNCTION_CALL_MODE DestroySessionFactory();
FIXENGAPI const char* FUNCTION_CALL_MODE GetFixErrorMsg(int iErrorNo);
//	由应用层设置，允许本进程最多允许创建的会话个数（这个接口，和单独拎出会话接口的初衷矛盾）
//	前述的需求是基于各个模块均会独立的有使用fix引擎的需求，而没有一个统一的协调
//	这里，如果设置会话上限，显然是存在一个统一的协调？？？
FIXENGAPI int FUNCTION_CALL_MODE SetSessionLimits(int iMaxSessions);
#ifdef __cplusplus
}
#endif

#endif	//	__HSFIX20_SESSION_INTERFACE__H__
