/***********************************************************************
系统名称: HSFIX20
模块名称: 配置接口
文 件 名: FIX会话的配置信息读取
功能说明: 
	配置文件中会出现以下类型的配置值：
		BOOL		是否可重复绑定， 是否带毫秒等	对应xxxBool
		INT			心跳时间等						对应xxxInt
		DAY			工作日期配置(星期几）			对应xxxDay
		TIME		工作时间配置					对应xxxTime
		STRING		各类名字						对应xxxString

		其他	特殊枚举	日志命名规则配置，日志记录方式等以整数处理
	

作    者: maoyinjie
开发日期: 2014-06-24
修改记录: 
***********************************************************************/

#ifndef _HSFIX_CONFIG_INTERFACE_H__
#define _HSFIX_CONFIG_INTERFACE_H__

#include "hsfix2_base_interface.h"


const int MAX_SESSION_NAME=64;	//	约束最大的会话名配置64个字节

//	配置举例
/*
	[SESSION_HQ]	会话名
	KEY1=VALUE1		
*/
/// 特定的单个会话的配置，其实就是简单的映射对的维护
/// 线程不安全， 不允许多线程执行 Validate 操作
struct ISessionConfig : IFIXBase
{
public:
	/**
	* 取配置的会话名称
	* @return 字符串值，会话名
	*/
	virtual const char * FUNCTION_CALL_MODE GetSessionName() const = 0;	//上例中就是获得 SESSION_HQ

	/**
	* 取字符串值
	* @param lpKey   变量名
	* @param lpDefaultValue 缺省值
	* @return 字符串值，没有找到时返回szDefault
	*/
	virtual const char * FUNCTION_CALL_MODE GetString(const char *lpKey, const char *lpDefaultValue) const = 0;
	/**
	* 取整数值
	* @param lpKey   变量名
	* @param iDefaultValue  缺省值
	* @return 整数值，没有找到时返回 iDefaultValue
	*/
	virtual int FUNCTION_CALL_MODE GetInt(const char *lpKey, int iDefaultValue) const = 0;

	/**
	* 取整数值
	* @param lpKey   变量名
	* @param bDefaultValue  缺省值
	* @return 整数值，没有找到时返回 bDefaultValue
	*/
	virtual bool FUNCTION_CALL_MODE GetBool(const char *lpKey, bool bDefaultValue) const = 0;

	/**
	* 是否存在某个配置项
	* @param lpKey   变量名
	* @return true 存在 false 不存在
	*/
	virtual bool FUNCTION_CALL_MODE Has( const char *lpKey ) const = 0;

	/**
	* 检查配置项是否合法，对于某些关键配置项，其配置值必须符合规范
	* @return 0 合法，	其他非法
	*/
	virtual int FUNCTION_CALL_MODE Validate() = 0;

	/**
	* 在Validate失败的时候,可以通过本函数调用获得失败的原因
	* @return 错误原因
	*/
	virtual const char* FUNCTION_CALL_MODE GetCfgErrorInfo() const = 0;
};

/// 以下仅在用户程序自己组织配置结构时起作用, 线程不安全
struct IHSFIXConfig : public IFIXBase
{
public:
	/**
	* 从文件加载
	* @param lpCfgFileName 文件名，格式类似ini，具体参考开发包示例和API手册
	* @return 返回0表示成功，否则失败
	*/
	virtual int FUNCTION_CALL_MODE Load(const char *lpCfgFileName)  = 0;

	/**
	* 保存到文件
	* @param lpCfgFileName 文件名
	* @return 返回0表示成功，否则失败
	*/
	virtual int FUNCTION_CALL_MODE Save(const char *lpCfgFileName) const = 0;

	/**
	* 获得特定会话名的会话个数
	* @param lpSessionName	会话名
	* @return 返回该名字的会话个数
	*/
	virtual int FUNCTION_CALL_MODE GetGivenSessions(const char *lpSessionName) const = 0;
	/**
	* 获得特定会话名的某个会话实例的配置
	* @param lpSessionName	会话名	
	* @param iIndex			会话名下标
	* @return 返回该会话的配置
	*/
	virtual ISessionConfig* FUNCTION_CALL_MODE GetCfgForSession(const char *lpSessionName, int iIndex)const=0;



	////////////	如有重复会话名的会话,以下接口均只能访问第一个	//////////////////////
	/// 如果需要访问重复会话的后续会话,请调用GetCfgForSession获得具体会话再访问
	/**
	* 取字符串值
	* @param lpSessionName 节名
	* @param lpKey   变量名
	* @param lpDefault 缺省值
	* @return 字符串值，没有找到时返回szDefault
	*/
	virtual const char * FUNCTION_CALL_MODE GetString(const char *lpSessionName, const char *lpKey, const char *lpDefault) const= 0;
	/**
	* 取整数值
	* @param lpSessionName 节名
	* @param lpKey   变量名
	* @param iDefault 缺省值
	* @return 整数值，没有找到时返回iDefault
	*/
	virtual int FUNCTION_CALL_MODE GetInt(const char *lpSessionName, const char *lpKey, int iDefault) const = 0;
	/**
	* 取布尔
	* @param lpSessionName 节名
	* @param lpKey   变量名
	* @param bDefault 缺省值
	* @return 整数值，没有找到时返回iDefault
	*/
	virtual bool FUNCTION_CALL_MODE GetBool(const char *lpSessionName, const char *lpKey, bool bDefault) const = 0;
	/**
	* 设置字符串值
	* @param lpSessionName 节名
	* @param lpKey   变量名
	* @param lpValue 缺省值
	* @return 0表示成功，否则失败
	*/
	virtual int FUNCTION_CALL_MODE SetString(const char *lpSessionName, const char *lpKey,  const char *lpValue) = 0;
	/**
	* 设置整数值
	* @param lpSessionName 节名
	* @param lpKey   变量名
	* @param iValue 缺省值
	* @return 0表示成功，否则失败
	*/
	virtual int FUNCTION_CALL_MODE SetInt(const char *lpSessionName, const char *lpKey, int iValue) = 0;
	/**
	* 取布尔
	* @param lpSessionName 节名
	* @param lpKey   变量名
	* @param bDefault 缺省值
	* @return 整数值，没有找到时返回iDefault
	*/
	virtual int FUNCTION_CALL_MODE SetBool(const char *lpSessionName, const char *lpKey, bool bValue) = 0;
	/**
	* 是否存在某个配置项
	* @param lpSessionName 节名
	* @param lpKey   变量名
	* @return true 存在 false 不存在
	*/
	virtual bool FUNCTION_CALL_MODE Has( const char *lpSessionName, const char *lpKey ) const=0;
	//////////////////////////////////////////////////////////////////////////
	
	/**
	* 检查配置项是否合法，对于某些关键配置项，其配置值必须符合规范
	* @return 0 合法，	其他非法
	*/
	virtual int FUNCTION_CALL_MODE Validate() = 0;
	/**
	* 在Load、Save、Setxxx 失败的时候,可以通过本函数调用获得失败的原因
	* @return 错误原因
	*/
	virtual const char* FUNCTION_CALL_MODE GetCfgErrorInfo()const=0;

	virtual int FUNCTION_CALL_MODE BeginTraverse()=0;
	virtual ISessionConfig* FUNCTION_CALL_MODE GetCurrCfgForSession()const=0;
	//	取下一会话的	<0失败
	virtual int FUNCTION_CALL_MODE Next()=0;

};


#ifdef __cplusplus
extern "C"{
#endif

/**
 * 创建一个配置对象	
 *@return 配置对象
 */
FIXENGAPI IHSFIXConfig* FUNCTION_CALL_MODE CreateConfigInterface();
FIXENGAPI void FUNCTION_CALL_MODE DestroyConfigInterface(IHSFIXConfig*);

#ifdef __cplusplus
}
#endif


#endif
