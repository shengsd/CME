
/*******************************************************
  源程序名称:hsfix2_fastmsginterface.h
  软件著作权:恒生电子股份有限公司
  系统名称:  HSFIX20
  模块名称:  接口文件
  功能说明:  
			处理FAST消息的接口
				
  作    者:  maoyj
  开发日期:  20140415
  备    注:  未实现
*********************************************************/

#ifndef __HSFIX20_FASTMSG_INTERFACE__H__
#define __HSFIX20_FASTMSG_INTERFACE__H__

#include "hsfix2_messageinterface.h"

///消息，包含消息体，同时嵌套Header，Trailer
struct  IFastMessage : public IFIXBase
{
	virtual HSFixMsgBody* FUNCTION_CALL_MODE GetMsgBody() const = 0;
	///	!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	///	以下2方法,仅供独立使用消息解析模块的应用调用,只有从绑定数据词典的消息工厂中
	///	分配的消息,才能正确的调用以下2方法.
	///	以下2函数需要配合数据词典才能正确操作,所以为避免不安全的操作,在消息未绑定
	///	数据词典的情况下,执行下面2个接口方法,将报错
	/// 反序列化,获得字符串   ???????????????????
	virtual const char* FUNCTION_CALL_MODE GetBuffer(int* iBuffLen) = 0;
	/// 序列化,根据字符串获得消息结构
	virtual int FUNCTION_CALL_MODE SetBuffer(const char*, int iBuffLen) = 0;
	///////////////////////////////////////////////////////////////
};

//	从该消息工厂分配的消息,在内部会绑定数据词典
struct IFastMessageFactory : public IFIXBase
{
	/// 只要资源足够,总能返回一个空的消息,只有资源不足,才返回NULL
	virtual IFastMessage* FUNCTION_CALL_MODE GetEmptyMessage()=0;
	/// 当数据非法时,会返回NULL,所以,应用一定要判断返回值
	virtual IFastMessage* FUNCTION_CALL_MODE GetMessage(const char*, int iBuffLen)=0;
	
	
	
	//////////////////////		遍历接口
	/// 根据fieldid获得fieldname
	/**
	 *@param int iFieldID		字段id
	 *@param int iBizModulID	操作时用到的业务模板
	 * @return fieldname, 如果失败,返回""
	*/
	const char* FUNCTION_CALL_MODE GetFieldNameByID(int iFieldID);
	/// 根据名字获得id
	/**
	 *@param const char* lpFieldName		字段名
	 *@param int iBizModulID	操作时用到的业务模板
	 * @return >=0字段ID	<0,说明没找到
	*/
	int FUNCTION_CALL_MODE GetFieldIDByName(const char* lpFieldName);
	const char* FUNCTION_CALL_MODE GetFuncNameByID(int iFuncD);

	//	获得一个字段的取值类型
	int FUNCTION_CALL_MODE GetFieldType(int iTagID);
	//	获得某个array类型的字段内部,具体特定成员的取值类型
	int FUNCTION_CALL_MODE GetElementType(int iTagID, int iIndex);

	//	maoyinjie 20101230
	const char* FUNCTION_CALL_MODE GetFuncNote(int iFuncD);	
}	
//	__attribute__((visibility ("default"))) int MyAdd(int a, int b)


FIXENGAPI IFastMessageFactory* FUNCTION_CALL_MODE CreateSpecialMsgFactory(char* lpDataDictionaryFile);
FIXENGAPI void FUNCTION_CALL_MODE DestroyMessageFactory(IFastMessageFactory*);
	
#endif	//	__HSFIX20_FASTMSG_INTERFACE__H__
