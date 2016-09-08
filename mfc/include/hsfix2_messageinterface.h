/****************************************************************************
  Դ��������:hsfix2_messageinterface.h
  �������Ȩ:�������ӹɷ����޹�˾
  ϵͳ����:  HSFIX20
  ģ������:  �ӿ��ļ�
  ����˵��:  
			����FIX������Ϣ��֯�ͽ����ӿ�
				
  ��    ��:  maoyj
  ��������:  20140415
  ��    ע:  
**����FIX������Ϣ����ṹ��
**	(1)��Ϣ(IMessage �ӿ�)
**     ÿ����Ϣ����Ϣͷ����Ϣ�塢��Ϣβ������¼(IRecord �ӿ�)���;
**     ��Ϣͷ�е�BeginString�ֶα�ʾ����Ϣ��������Э��汾��FIX4.2   FIX4.4  etc
**     ��Ϣͷ�е�MessageType�ֶα�ʾ����Ϣ����������Ϣ���ͣ� 
**
**  (2)��¼(IRecord �ӿ�)
**	   һ����¼���ɶ���ֶ�(Field)����;ÿ���ֶζ���һ�������ͱ�ǩ(TAG)����ʶ;
**     �ж����ֶ�:һ������ͨ�ֶΣ�һ�����ظ����ֶΣ�
**    
**     һ����ͨ�ֶ��ɱ�־���ֶεı�ǩ(TAG)�͸��ֶε�ȡֵ(Value)  {TAG, VALUE}�����;
**     һ���ظ����ֶ��ɱ�־���ظ���ı�ǩ(TAG)��һ��IGroup�ӿ�  {TAG, IGroup}�����;
**     
**  (3)�ظ���(IGroup�ӿ�)
**     �ظ�����Ϊһ�ָ������͵��ֶ�(Field)�����ڼ�¼(IRecord)�У�
**     һ���ظ�����һ���������¼�����,�൱����һ����¼��(DataSet);
**     ����ظ������������¼����ÿ����¼���ֶ�˳�����һ��;
**     �ظ���ļ�¼�л����԰����ظ����ֶΣ����ظ������Ƕ��;
**     
**     �ظ����TAG (FieldTag):
**         ��FIX/STEPЭ����Ϊ�ظ���ļ����ֶε�TAG;
**         ����ظ�����û�м�¼�����ظ����TAG�ֶ�Ҳ��������������շ��͵���Ϣ�С�
**     �ظ����¼�еķָ��ֶ�TAG (DelimTag):
**         FIXЭ��涨�ظ����¼�еĵ�һ���ֶα�����֣�����Ϊ������¼��ķָ���ʶ��
**
**     ע�⣺������Ҫ�����������Ӧ�����ݴʵ�,���²������ظ���!!
**						 
**     ��Ҫ:
**         ��ʵ����,��Ϣ���̲߳���ȫ��,�κζ��߳�ʹ��ͬһ��Ϣ,���ᵼ�²���Ԥ���ĺ��!!!!!!!!
**         �����³�����,���߳�ʹ���ǰ�ȫ��:
**             ����߳̾�ʹ�õ�����Ϣ��const�ӿ�,����ͬʱû���κ��߳�ʹ�÷�const�ӿ�
**				���Ǽ������ֳ�����,Ҳ�������û����߳�ʹ��

�޸ļ�¼��
	// 2015-08-24	zhouwh	��Ϣģ�����������ļ�����ʵ��fix��Ϣ�ĺϷ���У��
****************************************************************************/

#ifndef __HSFIX20_MESSAGE_INTERFACE__H__
#define __HSFIX20_MESSAGE_INTERFACE__H__
#include "hsfix2_base_interface.h"

struct IGroup;

///	��¼����ӿڣ���ʾһ����¼
/**��¼�е�ÿ���ֶΰ���TAG�Լ��͸�TAG��Ӧ��VALUE;	
 * һ����ͨ�ֶ�,����һ��{TAG, VALUE}��
 * һ���ظ�����Ϊһ��������ֶγ����ڼ�¼��;�ظ������һ��{TAG, IGroup}��
 * ��ͨ�ֶκ��ظ����ֶ��ɲ�ͬ�Ľӿڲ���
 */
struct  IRecord : public IFIXBase
{
	virtual ~IRecord(){};
	/// ����Tag�����ֶ�ֵ�������ֶβ������򷵻�NULL
	/**
	*@param int   iFieldTag	��Ҫ����ֶ�ֵ��TAG
	*@return ��TAG��Ӧ���ֶ�ֵ,�粻����,����NULL
	*/
	virtual const char* FUNCTION_CALL_MODE GetFieldValue( int iFieldTag ) const = 0;
	/// ����Tag�����ֶ�ֵ,�����ֶβ������򷵻�ȱʡֵ
	/**
	*@param int   iFieldTag			��Ҫ����ֶε�TAG
	*@param char* lpDefaultValue	��iFieldTag ������,�򷵻�ȱʡֵ
	*@return ��TAG��Ӧ���ֶ�ֵ,�ڷ���ȱʡֵ�������,���صľ���lpDefaultValueָ�뱾��
	*/
	virtual const char* FUNCTION_CALL_MODE GetFieldValueDefault( int iFieldTag, const char* lpDefaultValue) const = 0;
	/// ����Tag�����ֶ�ֵ�������ֶβ������򷵻�NULL,����ָ�� ilpRawlenָ���ֵ Ϊ0
	/**
	*@param int   iFieldTag	��Ҫ����ֶ�ֵ��TAG
	*@param int*  ilpRawlen	���ֶ����ݳ���
	*@return ��TAG��Ӧ���ֶ�ֵ,�粻����,����NULL
	*/
	virtual const void* FUNCTION_CALL_MODE GetFieldRawValue( int iFieldTag,  int* ilpRawlen ) const = 0 ;
	/// ȡָ�����ظ���
	/**
	 *@param  int iGroupTag �ظ���Tag
	 *@return IGroup �ṹָ��; ������ظ��鲻���ڣ�����NULL;
	 */
	virtual IGroup * FUNCTION_CALL_MODE GetGroup( int iGroupTag ) const = 0;
	///	�ظ������ͨ�ֶ�ͨ�õĽӿ�
	/// ����¼���Ƿ����iFieldTag�ֶΣ����ظ��飩
	/**
	*@param int   iFieldTag	������TAG
	*@return true ����	false ������
	*/
	virtual bool FUNCTION_CALL_MODE IsSetField( int iFieldTag ) const = 0;


	///	������ͨ�ֶεĽӿ�
	/// ���øü�¼��ָ���ֶε�ֵ
	/** ������ֶ��Ѵ��ڣ���ᱻ�滻
	*@param int   iFieldTag		�ֶε�TAG
	*@param const char* lpFieldValue	�ֶε�value
	*@return 0�ɹ�	����:ʧ��
	*/
	virtual int FUNCTION_CALL_MODE SetFieldValue( int iFieldTag, const char * lpFieldValue ) = 0;
	/// ���øü�¼��ָ���ֶε�ֵ(rawdata�����ֶΣ�
	/** ������ֶ��Ѵ��ڣ���ᱻ�滻
	*@param int   iFieldTag		�ֶε�TAG
	*@param const char* lpFieldRaw	�ֶε�value
	*@param int iRawlen	�ֶε�value�ĳ���
	*@return 0�ɹ�	����:ʧ��
	*/
	virtual int FUNCTION_CALL_MODE SetFieldRawValue( int iFieldTag, const void * lpFieldRaw, int iRawlen ) = 0;
	///	�����ظ���Ľӿ�
	/// ����¼������һ���ظ��飬����ظ����Ѵ��ڣ��򷵻ص����Ѵ����ظ���ӿڡ�
	/**
	*@param int iGroupTag	�ظ���(������)Tag(��������Ϊһ���ظ����TAG����)
							����Ψһ�ı�־��GROUP
	*@param int iDelimTag	�ظ���ָ���TAG��һ��GROUP�ɶ�����¼��ɣ�iDelimTag
							��Ϊ��¼�ķָ����������ֶ�����¼
	*@return  IGroup �ṹָ��
	��Ϊ��	����һ���յ�IGroup�ṹ�����ÿ�IGroup�ṹ���뵽��¼�У�ͬʱ����
			�����ɵĸ�IGroup�ṹָ�롣������Ա�����ڻ�ø�ָ��󣬽���ȥ
			��IGroup�ṹ���в�����������IGroup�в��������¼��
	*/
	virtual IGroup* FUNCTION_CALL_MODE SetGroup( int iGroupTag, int iDelimTag ) = 0;
	/// �Ӽ�¼��ɾ��iFieldTag�ֶΣ������ֶβ����ڣ����޲���
	/**
	*@param int   iFieldTag	��ɾ����TAG
	*/
	virtual void FUNCTION_CALL_MODE RemoveField( int iFieldTag ) = 0;
	
	/// zhouwh 2015-06-24 Ϊ�Ϻ�֤ͨ��Ŀ���ӣ���ʵ��fix����ת��Ϊpack
	/**
		�����ֶε�TAG��ȡ�ֶ�����
	@param int iFieldTag	�ֶε�TAG
	@param char **lpDst		�û���������ڴ洢�ֶ����ƵĻ���ָ��
	@param int iDstLen		�û�����Ļ���ĳ���
	*/
	virtual void FUNCTION_CALL_MODE GetFieldNameByTag(int iFieldTag, char **lpDst, int iDstLen) const=0;
	/**
		�ж��ֶ������Ƿ��Ƕ���������
	@param	int iFieldTag	��Ҫ�жϵ��ֶε�TAG
	@return	�Ƕ��������ͣ�����TRUE ���򣬷���FALSE
	*/
	virtual bool FUNCTION_CALL_MODE IsRawDataField(int iFieldTag) const=0;
	/**
		�����ֶ����ƻ�ȡ�ֶ�TAG
	@param	const char *lpFieldName		�ֶ�����
	@return	�ֶ����ƺϷ������ض�Ӧ�ֶ�TAG�����򣬷���-1
	*/
	virtual int FUNCTION_CALL_MODE GetFieldTagByName(const char *lpFieldName) const = 0;
	/**
		�����ظ���TAG��ȡ���ظ���ķָ���TAG
	@param	int iGroupID	�ظ���TAG
	@return iGroupID�ǺϷ��ظ��飬������ָ���TAG�����򣬷���-1
	*/
	virtual int FUNCTION_CALL_MODE GetDelimByGroup(int iGroupID) const = 0;
	///////////////////////////////////////////////////////////////////////


	///	�����Ǳ�����¼�Ľӿ�	һ��Ӧ�ó��������õ����ӿ�
	/// ��ʼ����ǰ�������ȵ���BeginTraverse����������������Ԥ֪
	/// ����0,˵���ɹ�, ����ʧ��
	virtual int FUNCTION_CALL_MODE BeginTraverse () = 0;
	virtual int FUNCTION_CALL_MODE GetCurrentFieldTag () const= 0;
	/// ����0��ʾ��ͨ�ֶ�	1��ʾ�ظ���
	virtual int FUNCTION_CALL_MODE GetCurrentFieldType () const= 0;
	virtual const char* FUNCTION_CALL_MODE GetCurrentFieldValue () const= 0;
	//  ����С��0,˵����������
	virtual int FUNCTION_CALL_MODE Next () = 0;
};


///�ظ������ӿڣ���ʾ������¼��һ������(DataSet)
//	�ظ�����û�����ݴʵ�������,���Զ�λ
struct  IGroup : public IFIXBase
{
	virtual ~IGroup(){};
	/// ȡ�ظ��������Tag
	virtual int FUNCTION_CALL_MODE GetFieldTag() const = 0;
	/// ȡ�ظ���ָ���Tag (��һ�ֶ�)
	virtual int FUNCTION_CALL_MODE GetDelimTag() const = 0;
	
	///	�����ӿ�
	/// �����ظ����ڼ�¼����
	virtual int FUNCTION_CALL_MODE GetRecordCount() const = 0;
	/// ȡ�ظ����е�ĳ����¼
	/**
	 *@param int iIndex �ڼ�����¼(iIndex��0��ʼ)
	 *@return IRecord �ṹָ��
	 */
	virtual IRecord * FUNCTION_CALL_MODE GetRecord( int iIndex ) const = 0;
	/// ���ظ�����׷��һ���հ׼�¼
	/**
	 * @return IRecord ׷�ӵļ�¼ָ��
	��Ϊ:	����һ��AddRecord����,��������һ���յ�Record,���ҽ���Record����
			�ظ���,���ظմ�����Recordָ��,������Ա��ø�ָ���,������Record
			�в����ֶ�(��ͨ�ֶλ�������һ���ظ����ֶ�)
	*/
	virtual IRecord * FUNCTION_CALL_MODE AddRecord( )  = 0;
};

	
typedef IRecord HSFixHeader;
typedef IRecord HSFixTrailer;
typedef IRecord HSFixMsgBody;

///��Ϣ��������Ϣ�壬ͬʱǶ��Header��Trailer
//	������SetMessageType������Ϣ����,Ȼ����ܲ�����Ϣ��(��Ϊ������Ϣ����Ҫ�����ݴʵ��)
//	����ͬ����Ϣ����,�����ݴʵ䲻һ��
struct  IMessage : public IFIXBase
{
	virtual ~IMessage(){};
	/// �����Ϣͷ��¼
	virtual HSFixHeader* FUNCTION_CALL_MODE GetHeader() const = 0;
	/// �����Ϣβ��¼
	virtual HSFixTrailer* FUNCTION_CALL_MODE GetTrailer() const = 0;
	/// �����Ϣ���¼
	virtual HSFixMsgBody* FUNCTION_CALL_MODE GetMsgBody() const = 0;
	
	
	///	���º���,������Ϣͷ�г����ļ����ֶ�,�Լ򻯵���,ʵ����,�û�Ҳ����ͨ�����ȵ���
	///	GetHeader�����Ϣͷ��,Ȼ�������Ϣͷ�о����TAG�����ͬ��Ч��
	/// �����Ϣ����,��ȡʧ��,�򷵻�NULL
	virtual const char* FUNCTION_CALL_MODE GetMessageType() const = 0;
	/// ������Ϣ����
	virtual int FUNCTION_CALL_MODE SetMessageType(const char*) = 0;
	/// �����ϢЭ���ʶ
	virtual const char* FUNCTION_CALL_MODE GetBeginString() const = 0;
	/// ������ϢЭ���ʶ
	virtual int FUNCTION_CALL_MODE SetBeginString(const char*) = 0;
	/// ��÷����߱�ʶ
	virtual const char* FUNCTION_CALL_MODE GetSenderID() const = 0;
	/// ���÷����߱�ʶ
	virtual int FUNCTION_CALL_MODE SetSenderID(const char*) = 0;
	/// ���Ŀ���ʶ
	virtual const char* FUNCTION_CALL_MODE GetTargetID() const = 0;
	/// ����Ŀ���ʶ
	virtual int FUNCTION_CALL_MODE SetTargetID(const char*) = 0;
	///////////////////////////////////////////////////////////////////////
	
	
	///	��Header�еķ����ߺͽ����߶Ե���Ϊ����Ϣ�ķ����߽�����
	///	���ӿ�һ������FIX������Ӧ��,  һ��Ӧ�ò���Ҫ����
	virtual void FUNCTION_CALL_MODE ReverseRoute( const HSFixHeader* ) = 0;


	///	�����Ϣ���Ӧ����Ҫ�ظ�ʹ����Ϣ,����ÿ�ξ������ͷ�,����ÿ������ʹ�ø���Ϣǰ
	///	���������սӿ�,�Ա�����ϴε�ʹ�û���
	virtual int FUNCTION_CALL_MODE ClearMsg() = 0;
	//	����һ����Ϣ,����ᴴ��һ������Ϣ,���Ʊ���Ϣ�����ݷ��ظ�������
	//	�����߿��Խ��¸��Ƴ�������Ϣ�������淢��,���������ں��ʵ�ʱ���ͷ�
	virtual IMessage* FUNCTION_CALL_MODE CloneMsg() const=0;
	
	
	
	///	!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	///	����2����,��������ʹ����Ϣ����ģ���Ӧ�õ���,ֻ�дӰ����ݴʵ����Ϣ������
	///	�������Ϣ,������ȷ�ĵ�������2����.
	///	����2������Ҫ������ݴʵ������ȷ����,����Ϊ���ⲻ��ȫ�Ĳ���,����Ϣδ��
	///	���ݴʵ�������,ִ������2���ӿڷ���,������
	/**
	 *@param out ilpBuffLen ���������ĳ���
	 *@return const char*	��������
	 */
	/// �����л�,����ַ���   
	virtual const char* FUNCTION_CALL_MODE GetBuffer(int* ilpBuffLen) = 0;
	/// ���л�,�����ַ��������Ϣ�ṹ
	/**
	 *@param in const char*lpBuff	����Ķ��������׵�ַ
	 *@param in int iBuffLen		����Ķ��������ֽ���
	 *@return int	0: �ɹ�	����:	ʧ��
	 */
	virtual int FUNCTION_CALL_MODE SetBuffer(const char*lpBuff, int iBuffLen) = 0;
	///////////////////////////////////////////////////////////////
	
	
	//////////////////////////////////////////////////////////////////////////
	///	����Ϣ��Ч����صĽӿ�			�������ӿ���ʱ��ʵ��
	///	�����Ϣ����(����Ϣ���ɵ����ھ����ĺ�����)
	virtual int FUNCTION_CALL_MODE GetMsgAges() const = 0;
	///	����Ϣ������������
	virtual int FUNCTION_CALL_MODE GetMaxAges() const = 0;
	///	�پ������ٺ���,����Ϣ�ͻ�ʧЧ
	virtual int FUNCTION_CALL_MODE GetTimesToDie() const = 0;
	//////////////////////////////////////////////////////////////////////////


	///	�ͷ���Ϣ��Դ,�������Ὣ��Ϣ�黹�����ʵĳأ�������Ϣ�ڲ����ظ����¼Ҳ��黹
	/// ע�⣺ Release �� AddRef �ӿڽ����ǿպ������ã������������������ü���������������2�ӿڽ�Ϊ����delphi��
	virtual void FUNCTION_CALL_MODE FreeMsg() = 0;
	/// �� ��Ϣ���� / ���� ʧ��ʱ,��ͨ���ú��������ϸ������Ϣ(Ŀǰδʵ��,���д�������¼��־)
	virtual const char* FUNCTION_CALL_MODE GetFailureReason() = 0;

	// 2015-08-24	zhouwh	��Ϣģ�����������ļ�����ʵ��fix��Ϣ�ĺϷ���У��
	/// ���л�,�����ַ��������Ϣ�ṹ
	/**
	 *@param in const char*lpBuff	����Ķ��������׵�ַ
	 *@param in int iBuffLen		����Ķ��������ֽ���
	 *@param in IMessage* lpMsgReturn		�������Ϣ�ӿڣ��������������������lpMsgReturn
	 *@return int	0: �ɹ�
	 *				-1: ����lpMsgReturn�Ѿ�������
					-2����������Ƿ���lpMsgReturnδ���
					-3: ����fix���ĳ���lpMsgReturnδ���
					-4: �Ƿ���fix���ģ�lpMsgReturnδ���
					-5: ��֧�ֵ�fixЭ��汾��lpMsgReturnδ���
					-6: ��������lpMsgReturnδ���
	 */
	virtual int FUNCTION_CALL_MODE SetBufferWithErrReturn(const char*lpBuff, int iBuffLen, IMessage* lpMsgReturn) = 0; 
};



////	ע��:	����ӵ������FIX�Ự��Ӧ�ã���Ҫȥ������Ϣ�����������Ҫ��Ϣ��Ӧ�ô�����ĻỰ�л��
/**
	��Ϣ����������������FIX��Ϣ����û������FIX�Ự��Ӧ��ʹ��
**/
//	�Ӹ���Ϣ�����������Ϣ,���ڲ�������ݴʵ�
struct IMessageFactory : public IFIXBase
{
	virtual ~IMessageFactory(){};

	/// ֻҪ��Դ�㹻,���ܷ���һ���յ���Ϣ,ֻ����Դ����,�ŷ���NULL
	virtual IMessage* FUNCTION_CALL_MODE GetEmptyMessage()=0;
	/// �����ݷǷ�ʱ,�᷵��NULL,����,Ӧ��һ��Ҫ�жϷ���ֵ
	virtual IMessage* FUNCTION_CALL_MODE GetFixMessage(const char*lpBuff, int iBuffLen)=0;
	
	//	�������ݴʵ�����Ϣ�ĺϷ��Լ��(���������û�,����ʹ�ñ��ӿ�)
	//	0: �Ϸ�	����:�Ƿ�
	virtual int FUNCTION_CALL_MODE ValidMessage(IMessage * lpMessage)=0;

	//// 2015-08-24	zhouwh	��Ϣģ�����������ļ�����ʵ��fix��Ϣ�ĺϷ���У��
	///// �����ݷǷ�ʱ���᷵��NULL������Ӧ�ľܾ���Ϣ��lpErrMsgReturn����
	///// lpErrMsgReturn�ɵ����߸���������ͷţ�����ֻ�ڷ�������ʱ���������
	//virtual IMessage* FUNCTION_CALL_MODE GetFixMessageWithErrReturn(const char* lpBuff, int iBuffLen, IMessage* lpErrMsgReturn)=0;
};

#ifdef __cplusplus
extern "C"{
#endif

//	����һ�������ض����ݴʵ����Ϣ����,���б�������������Ϣ�����lpDataDictionaryFile��
/**
 *@param char* lpDataDictionaryFile	���ݴʵ��ļ�,��ΪNULL,�򲻺��κ����ݴʵ��
 				���ڹ�����ʹ����Ϣ��Ӧ����Ҫ�������ݴʵ�,��ʵ�ֺϷ���У��
 				������������ʹ��fix�Ự��Ӧ��,����Դ���һ�������������ݴʵ�󶨵���Ϣ����
 				��Ϊ�Ự�����Ѿ�ʵ���˺����ݴʵ�İ�
 *@return ��Ϣ����ָ��
 */
FIXENGAPI IMessageFactory* FUNCTION_CALL_MODE CreateSpecialMsgFactory(char* lpDataDictionaryFile);

// 2015-08-24	zhouwh	��Ϣģ�����������ļ�����ʵ��fix��Ϣ�ĺϷ���У��
FIXENGAPI IMessageFactory* FUNCTION_CALL_MODE CreateMsgFactory(char* lpDataDictionaryFile, char* lpCfgFile);

/**	�ݻ�һ����Ϣ����
**/
FIXENGAPI void FUNCTION_CALL_MODE DestroyMessageFactory(IMessageFactory*);
#ifdef __cplusplus
}
#endif

#endif //__HSFIX20_MESSAGE_INTERFACE__H__

