/***********************************************************************
ϵͳ����: HSFIX20
ģ������: ���ýӿ�
�� �� ��: FIX�Ự��������Ϣ��ȡ
����˵��: 
	�����ļ��л�����������͵�����ֵ��
		BOOL		�Ƿ���ظ��󶨣� �Ƿ�������	��ӦxxxBool
		INT			����ʱ���						��ӦxxxInt
		DAY			������������(���ڼ���			��ӦxxxDay
		TIME		����ʱ������					��ӦxxxTime
		STRING		��������						��ӦxxxString

		����	����ö��	��־�����������ã���־��¼��ʽ������������
	

��    ��: maoyinjie
��������: 2014-06-24
�޸ļ�¼: 
***********************************************************************/

#ifndef _HSFIX_CONFIG_INTERFACE_H__
#define _HSFIX_CONFIG_INTERFACE_H__

#include "hsfix2_base_interface.h"


const int MAX_SESSION_NAME=64;	//	Լ�����ĻỰ������64���ֽ�

//	���þ���
/*
	[SESSION_HQ]	�Ự��
	KEY1=VALUE1		
*/
/// �ض��ĵ����Ự�����ã���ʵ���Ǽ򵥵�ӳ��Ե�ά��
/// �̲߳���ȫ�� ��������߳�ִ�� Validate ����
struct ISessionConfig : IFIXBase
{
public:
	/**
	* ȡ���õĻỰ����
	* @return �ַ���ֵ���Ự��
	*/
	virtual const char * FUNCTION_CALL_MODE GetSessionName() const = 0;	//�����о��ǻ�� SESSION_HQ

	/**
	* ȡ�ַ���ֵ
	* @param lpKey   ������
	* @param lpDefaultValue ȱʡֵ
	* @return �ַ���ֵ��û���ҵ�ʱ����szDefault
	*/
	virtual const char * FUNCTION_CALL_MODE GetString(const char *lpKey, const char *lpDefaultValue) const = 0;
	/**
	* ȡ����ֵ
	* @param lpKey   ������
	* @param iDefaultValue  ȱʡֵ
	* @return ����ֵ��û���ҵ�ʱ���� iDefaultValue
	*/
	virtual int FUNCTION_CALL_MODE GetInt(const char *lpKey, int iDefaultValue) const = 0;

	/**
	* ȡ����ֵ
	* @param lpKey   ������
	* @param bDefaultValue  ȱʡֵ
	* @return ����ֵ��û���ҵ�ʱ���� bDefaultValue
	*/
	virtual bool FUNCTION_CALL_MODE GetBool(const char *lpKey, bool bDefaultValue) const = 0;

	/**
	* �Ƿ����ĳ��������
	* @param lpKey   ������
	* @return true ���� false ������
	*/
	virtual bool FUNCTION_CALL_MODE Has( const char *lpKey ) const = 0;

	/**
	* ����������Ƿ�Ϸ�������ĳЩ�ؼ������������ֵ������Ϲ淶
	* @return 0 �Ϸ���	�����Ƿ�
	*/
	virtual int FUNCTION_CALL_MODE Validate() = 0;

	/**
	* ��Validateʧ�ܵ�ʱ��,����ͨ�����������û��ʧ�ܵ�ԭ��
	* @return ����ԭ��
	*/
	virtual const char* FUNCTION_CALL_MODE GetCfgErrorInfo() const = 0;
};

/// ���½����û������Լ���֯���ýṹʱ������, �̲߳���ȫ
struct IHSFIXConfig : public IFIXBase
{
public:
	/**
	* ���ļ�����
	* @param lpCfgFileName �ļ�������ʽ����ini������ο�������ʾ����API�ֲ�
	* @return ����0��ʾ�ɹ�������ʧ��
	*/
	virtual int FUNCTION_CALL_MODE Load(const char *lpCfgFileName)  = 0;

	/**
	* ���浽�ļ�
	* @param lpCfgFileName �ļ���
	* @return ����0��ʾ�ɹ�������ʧ��
	*/
	virtual int FUNCTION_CALL_MODE Save(const char *lpCfgFileName) const = 0;

	/**
	* ����ض��Ự���ĻỰ����
	* @param lpSessionName	�Ự��
	* @return ���ظ����ֵĻỰ����
	*/
	virtual int FUNCTION_CALL_MODE GetGivenSessions(const char *lpSessionName) const = 0;
	/**
	* ����ض��Ự����ĳ���Ựʵ��������
	* @param lpSessionName	�Ự��	
	* @param iIndex			�Ự���±�
	* @return ���ظûỰ������
	*/
	virtual ISessionConfig* FUNCTION_CALL_MODE GetCfgForSession(const char *lpSessionName, int iIndex)const=0;



	////////////	�����ظ��Ự���ĻỰ,���½ӿھ�ֻ�ܷ��ʵ�һ��	//////////////////////
	/// �����Ҫ�����ظ��Ự�ĺ����Ự,�����GetCfgForSession��þ���Ự�ٷ���
	/**
	* ȡ�ַ���ֵ
	* @param lpSessionName ����
	* @param lpKey   ������
	* @param lpDefault ȱʡֵ
	* @return �ַ���ֵ��û���ҵ�ʱ����szDefault
	*/
	virtual const char * FUNCTION_CALL_MODE GetString(const char *lpSessionName, const char *lpKey, const char *lpDefault) const= 0;
	/**
	* ȡ����ֵ
	* @param lpSessionName ����
	* @param lpKey   ������
	* @param iDefault ȱʡֵ
	* @return ����ֵ��û���ҵ�ʱ����iDefault
	*/
	virtual int FUNCTION_CALL_MODE GetInt(const char *lpSessionName, const char *lpKey, int iDefault) const = 0;
	/**
	* ȡ����
	* @param lpSessionName ����
	* @param lpKey   ������
	* @param bDefault ȱʡֵ
	* @return ����ֵ��û���ҵ�ʱ����iDefault
	*/
	virtual bool FUNCTION_CALL_MODE GetBool(const char *lpSessionName, const char *lpKey, bool bDefault) const = 0;
	/**
	* �����ַ���ֵ
	* @param lpSessionName ����
	* @param lpKey   ������
	* @param lpValue ȱʡֵ
	* @return 0��ʾ�ɹ�������ʧ��
	*/
	virtual int FUNCTION_CALL_MODE SetString(const char *lpSessionName, const char *lpKey,  const char *lpValue) = 0;
	/**
	* ��������ֵ
	* @param lpSessionName ����
	* @param lpKey   ������
	* @param iValue ȱʡֵ
	* @return 0��ʾ�ɹ�������ʧ��
	*/
	virtual int FUNCTION_CALL_MODE SetInt(const char *lpSessionName, const char *lpKey, int iValue) = 0;
	/**
	* ȡ����
	* @param lpSessionName ����
	* @param lpKey   ������
	* @param bDefault ȱʡֵ
	* @return ����ֵ��û���ҵ�ʱ����iDefault
	*/
	virtual int FUNCTION_CALL_MODE SetBool(const char *lpSessionName, const char *lpKey, bool bValue) = 0;
	/**
	* �Ƿ����ĳ��������
	* @param lpSessionName ����
	* @param lpKey   ������
	* @return true ���� false ������
	*/
	virtual bool FUNCTION_CALL_MODE Has( const char *lpSessionName, const char *lpKey ) const=0;
	//////////////////////////////////////////////////////////////////////////
	
	/**
	* ����������Ƿ�Ϸ�������ĳЩ�ؼ������������ֵ������Ϲ淶
	* @return 0 �Ϸ���	�����Ƿ�
	*/
	virtual int FUNCTION_CALL_MODE Validate() = 0;
	/**
	* ��Load��Save��Setxxx ʧ�ܵ�ʱ��,����ͨ�����������û��ʧ�ܵ�ԭ��
	* @return ����ԭ��
	*/
	virtual const char* FUNCTION_CALL_MODE GetCfgErrorInfo()const=0;

	virtual int FUNCTION_CALL_MODE BeginTraverse()=0;
	virtual ISessionConfig* FUNCTION_CALL_MODE GetCurrCfgForSession()const=0;
	//	ȡ��һ�Ự��	<0ʧ��
	virtual int FUNCTION_CALL_MODE Next()=0;

};


#ifdef __cplusplus
extern "C"{
#endif

/**
 * ����һ�����ö���	
 *@return ���ö���
 */
FIXENGAPI IHSFIXConfig* FUNCTION_CALL_MODE CreateConfigInterface();
FIXENGAPI void FUNCTION_CALL_MODE DestroyConfigInterface(IHSFIXConfig*);

#ifdef __cplusplus
}
#endif


#endif
