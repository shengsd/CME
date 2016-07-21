
/****************************************************************************
  源程序名称:hsfix2_base_interface.h
  软件著作权:恒生电子股份有限公司
  系统名称:  HSFIX20
  模块名称:  基础接口定义文件
  功能说明:  

				
  作    者:  maoyj
  开发日期:  20140415
  备    注:  
  
**/

#ifndef _HSFIX_BASE_INTERFACE_H__
#define _HSFIX_BASE_INTERFACE_H__

#ifdef WIN32
	#if !defined(SEPARATE_COMPILE)
		#ifdef	HSFIXENG20_EXPORTS
			#define FIXENGAPI __declspec(dllexport)
		#else
			#define FIXENGAPI __declspec(dllimport)
		#endif
	#else
		#define FIXENGAPI
	#endif

	#if !defined( FUNCTION_CALL_MODE )
		#define FUNCTION_CALL_MODE		__stdcall
	#endif
#else
	#define FIXENGAPI	__attribute__((visibility ("default")))
	#define FUNCTION_CALL_MODE
#endif

struct IFIXBase
{
	///接口查询
	virtual unsigned long  FUNCTION_CALL_MODE QueryInterface(const int & iid, void **ppv ) = 0;
	virtual unsigned long  FUNCTION_CALL_MODE AddRef() = 0;
	virtual unsigned long  FUNCTION_CALL_MODE Release() =  0;
};

#endif
