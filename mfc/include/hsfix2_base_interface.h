
/****************************************************************************
  Դ��������:hsfix2_base_interface.h
  �������Ȩ:�������ӹɷ����޹�˾
  ϵͳ����:  HSFIX20
  ģ������:  �����ӿڶ����ļ�
  ����˵��:  

				
  ��    ��:  maoyj
  ��������:  20140415
  ��    ע:  
  
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
	///�ӿڲ�ѯ
	virtual unsigned long  FUNCTION_CALL_MODE QueryInterface(const int & iid, void **ppv ) = 0;
	virtual unsigned long  FUNCTION_CALL_MODE AddRef() = 0;
	virtual unsigned long  FUNCTION_CALL_MODE Release() =  0;
};

#endif
