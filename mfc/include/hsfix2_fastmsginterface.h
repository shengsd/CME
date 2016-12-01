
/*******************************************************
  Դ��������:hsfix2_fastmsginterface.h
  �������Ȩ:�������ӹɷ����޹�˾
  ϵͳ����:  HSFIX20
  ģ������:  �ӿ��ļ�
  ����˵��:  
			����FAST��Ϣ�Ľӿ�
				
  ��    ��:  maoyj
  ��������:  20140415
  ��    ע:  δʵ��
*********************************************************/

#ifndef __HSFIX20_FASTMSG_INTERFACE__H__
#define __HSFIX20_FASTMSG_INTERFACE__H__

#include "hsfix2_messageinterface.h"

///��Ϣ��������Ϣ�壬ͬʱǶ��Header��Trailer
struct  IFastMessage : public IFIXBase
{
	virtual HSFixMsgBody* FUNCTION_CALL_MODE GetMsgBody() const = 0;
	///	!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	///	����2����,��������ʹ����Ϣ����ģ���Ӧ�õ���,ֻ�дӰ����ݴʵ����Ϣ������
	///	�������Ϣ,������ȷ�ĵ�������2����.
	///	����2������Ҫ������ݴʵ������ȷ����,����Ϊ���ⲻ��ȫ�Ĳ���,����Ϣδ��
	///	���ݴʵ�������,ִ������2���ӿڷ���,������
	/// �����л�,����ַ���   ???????????????????
	virtual const char* FUNCTION_CALL_MODE GetBuffer(int* iBuffLen) = 0;
	/// ���л�,�����ַ��������Ϣ�ṹ
	virtual int FUNCTION_CALL_MODE SetBuffer(const char*, int iBuffLen) = 0;
	///////////////////////////////////////////////////////////////
};

//	�Ӹ���Ϣ�����������Ϣ,���ڲ�������ݴʵ�
struct IFastMessageFactory : public IFIXBase
{
	/// ֻҪ��Դ�㹻,���ܷ���һ���յ���Ϣ,ֻ����Դ����,�ŷ���NULL
	virtual IFastMessage* FUNCTION_CALL_MODE GetEmptyMessage()=0;
	/// �����ݷǷ�ʱ,�᷵��NULL,����,Ӧ��һ��Ҫ�жϷ���ֵ
	virtual IFastMessage* FUNCTION_CALL_MODE GetMessage(const char*, int iBuffLen)=0;
	
	
	
	//////////////////////		�����ӿ�
	/// ����fieldid���fieldname
	/**
	 *@param int iFieldID		�ֶ�id
	 *@param int iBizModulID	����ʱ�õ���ҵ��ģ��
	 * @return fieldname, ���ʧ��,����""
	*/
	const char* FUNCTION_CALL_MODE GetFieldNameByID(int iFieldID);
	/// �������ֻ��id
	/**
	 *@param const char* lpFieldName		�ֶ���
	 *@param int iBizModulID	����ʱ�õ���ҵ��ģ��
	 * @return >=0�ֶ�ID	<0,˵��û�ҵ�
	*/
	int FUNCTION_CALL_MODE GetFieldIDByName(const char* lpFieldName);
	const char* FUNCTION_CALL_MODE GetFuncNameByID(int iFuncD);

	//	���һ���ֶε�ȡֵ����
	int FUNCTION_CALL_MODE GetFieldType(int iTagID);
	//	���ĳ��array���͵��ֶ��ڲ�,�����ض���Ա��ȡֵ����
	int FUNCTION_CALL_MODE GetElementType(int iTagID, int iIndex);

	//	maoyinjie 20101230
	const char* FUNCTION_CALL_MODE GetFuncNote(int iFuncD);	
}	
//	__attribute__((visibility ("default"))) int MyAdd(int a, int b)


FIXENGAPI IFastMessageFactory* FUNCTION_CALL_MODE CreateSpecialMsgFactory(char* lpDataDictionaryFile);
FIXENGAPI void FUNCTION_CALL_MODE DestroyMessageFactory(IFastMessageFactory*);
	
#endif	//	__HSFIX20_FASTMSG_INTERFACE__H__
