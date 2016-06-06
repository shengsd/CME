#ifndef _EXMDP_H_
#define _EXMDP_H_

// 下列 ifdef 块是创建使从 DLL 导出更简单的
// 宏的标准方法。此 DLL 中的所有文件都是用命令行上定义的 EXMDP_EXPORTS
// 符号编译的。在使用此 DLL 的
// 任何其他项目上不应定义此符号。这样，源文件中包含此文件的任何其他项目都会将
// EXMDP_API 函数视为是从 DLL 导入的，而此 DLL 则将用此宏定义的
// 符号视为是被导出的。
#ifdef EXMDP_EXPORTS
#define EXMDP_API __declspec(dllexport)
#else
#define EXMDP_API __declspec(dllimport)
#endif

///MDP引擎回调接口，引擎的使用者实现此接口，用于响应引擎发生的各类事件	
/**
 * 引擎的使用者实现此接口，并在初始化时向引擎注册此接口；
 */
//#include <stdint.h>
typedef __int64 int64_t;
typedef unsigned __int64 uint64_t;

typedef struct tagConfigStruct
{
	char configFile[128];	//CME提供的配置文件
	char localInterface[16]; //本地interface
	char templateFile[128];	//CME行情报文解析模板
	char userName[64];		//重发服务用户名
	char passWord[64];		//重发服务密码
	char errorInfo[128];	//启动失败返回的错误信息
}ConfigStruct;

//一个字段，一定要根据模板文件里的type类型选择获取值的函数
class MDPField
{
public:
	///根据字段值的类型调用对应的函数
	/**
	*@param  const int index	字段中第index个值，默认-1表示首个，一般情况下不需要传值
	*@return 对应类型的值
	*/
	virtual int64_t getInt(const int index = -1) const = 0;

	virtual uint64_t getUInt(const int index  = -1) const = 0;

	virtual double getDouble(const int index = -1) const =0;

	///字段值的长度
	/**
	*@param  const int index = -1		字段中第index个值
	*@return int 值的长度
	*/
	virtual int length(const int index = -1) const = 0;

	///获取字段值，类型为字符串的
	/**
	*@param  const int index	字段中第index个值，一般情况下为0
	*@param  char *dst			需要拷贝到的目标地址
	*@param  const int offset	源字符串起点, 一般情况下为0
	*@param  const int length	字符串长度，一般情况下可以通过length()获取
	*/
	virtual void getArray(const int index, char *dst, const int offset, const int length) const = 0;
};

//一条记录（包含不重复的多个字段）
class MDPFieldMap
{
public:
	///获取表示重复组字段标签对应的记录条数
	/**
	*@param  int tag	重复组字段标签
	*@return int		对应的记录条数
	*/
	virtual int getFieldMapNumInGroup(int tag) = 0;

	///获取重复组字段标签对应的第index条记录
	/**
	*@param  int tag		重复组字段标签
	*@return MDPFieldMap*	对应的记录指针
	*/
	virtual MDPFieldMap* getFieldMapPtrInGroup(int tag, int index) = 0;

	///获取当前记录中字段标签对应的字段
	/**
	*@param  int tag		字段标签
	*@return MDPField*		对应的字段指针
	*/
	virtual MDPField* getField(int tag) = 0;
};

//使用者必须定义派生类，并实现其中的回调函数
class Application
{
public:
	///行情报文回调函数
	/**
	*@param  MDPFieldMap* pMDPFieldMap		报文即一条记录
	*@return const int templateID			报文的模板ID
	*/
	virtual void onMarketData( MDPFieldMap* pMDPFieldMap, const int templateID) = 0;
};

#ifdef __cplusplus
extern "C"{
#endif

/////行情引擎启动函数
/**
*@param  ConfigStruct* configStruct		配置信息
*@param  Application* application		行情回调类
*@param  LPFN_WRITELOG lpfnWriteLog		日志函数指针
*@return int	0 - 成功  else - 失败
*/
EXMDP_API int StartEngine( ConfigStruct* configStruct, Application* application );

///行情引擎关闭函数
/**
*@return int 0 - 成功  else - 失败
*/
EXMDP_API int StopEngine();

#ifdef __cplusplus
}
#endif

#endif

