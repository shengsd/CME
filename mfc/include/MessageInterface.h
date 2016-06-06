///����FIX������Ϣ��֯�ͽ����ӿ�
/****************************************************************************
**����FIX������Ϣ����ṹ��
**	(1)��Ϣ(IMessage �ӿ�)
**     ÿ����Ϣ����Ϣͷ����Ϣ�塢��Ϣβ������¼(IRecord �ӿ�)���;
**     ��Ϣͷ�е�BeginString�ֶα�ʾ����Ϣ��������Э�飻
**     ��Ϣͷ�е�MessageType�ֶα�ʾ����Ϣ����������Ϣ��
**
**  (2)��¼(IRecord �ӿ�)
**	   һ����¼���ɶ���ֶ�(Field)����;ÿ���ֶζ���һ�������ͱ�ǩ(TAG)����ʶ;
**     �ж����ֶ�:һ������ͨ�ֶΣ�һ�����ظ����ֶΣ�
**    
**     һ����ͨ�ֶ��ɱ�־���ֶεı�ǩ(TAG)�͸��ֶε�ȡֵ(Value)�����;
**     һ���ظ����ֶ��ɱ�־���ظ���ı�ǩ(TAG)��һ��IGroup�ӿڶ����;
**     
**  (3)�ظ���(IGroup�ӿ�)
**     �ظ�����Ϊһ�ָ������͵��ֶ�(Field)�����ڼ�¼(IRecord)�У�
**     һ���ظ�����һ���������¼�����,�൱����һ����¼��(DataSet);
**     ����ظ������������¼����ÿ����¼���ֶ�˳�����һ��;
**     �ظ���ļ�¼�л����԰����ظ����ֶΣ����ظ������Ƕ��;
**     
**     �ظ����TAG (FieldTag):
**         ��FIX/STEPЭ����Ϊ�ظ���ļ����ֶε�TAG;
**         ��IFTSЭ����Ϊ�ظ��鱾���ǩ��TAG;
**         ����ظ�����û�м�¼�����ظ����TAG�ֶ�Ҳ��������������շ��͵���Ϣ�С�
**     �ظ����¼�еķָ��ֶ�TAG (DelimTag):
**         FIXЭ��涨�ظ����¼�еĵ�һ���ֶα�����֣�����Ϊ������¼��ķָ���ʶ��
**         IFTSЭ���޴�Ҫ�󣬵����ӿ���Ҫ���ṩ��һ���ֶε�TAG ���Ա��ֽӿ�ͳһ��
**
**     ע�⣺���յ�����Ϣ�а������ظ���ʱ��������Ҫ�����������Ӧ�����ݴʵ䣬
**           �����ܽ�����Ϣ�е��ظ��顣
****************************************************************************/

#ifndef HSFIX_MESSAGE
#define HSFIX_MESSAGE


#ifdef WIN32
	#ifdef	FIXENGAPI_EXPORTS
		#define FIXENGAPI __declspec(dllexport)
	#else
		#define FIXENGAPI __declspec(dllimport)
	#endif

	#if !defined( FUNCTION_CALL_MODE )
	#define FUNCTION_CALL_MODE		__stdcall
	#endif

#else
	#define FIXENGAPI
	#define PLUGINS_CALL
	#define FUNCTION_CALL_MODE
#endif


#ifdef _MSC_VER
	#pragma warning( disable: 4786 )
#endif

#include <vector>
#include <memory>

#include "HSFixError.h"

#ifndef HS_IKNOWN_V1
#define HS_IKNOWN_V1
///��׼�ӿڲ�ѯ,����com��׼
struct IKnownV1
{
	///�ӿڲ�ѯ
	/**
	 *@param const int iid �ӿ�ȫ��Ψһ��ʶ
	 *@param void **ppv ����iid��Ӧ�Ľӿ�ָ�루�ӿڲ���com��׼��
	 *@return I_OK �ɹ���I_NONE δ�鵽iid ��Ӧ�ӿ�
	 */
    virtual unsigned long  FUNCTION_CALL_MODE QueryInterface(const int & iid, IKnownV1 **ppv ) = 0;
    virtual unsigned long  FUNCTION_CALL_MODE AddRef() = 0;
    virtual unsigned long  FUNCTION_CALL_MODE Release() =  0;
};
#endif

struct IGroup;

///	��¼����ӿڣ���ʾһ����¼
/**��¼�е�ÿ���ֶΰ���TAG�Լ��͸�TAG��Ӧ��VALUE;
 * һ���ظ�����Ϊһ��������ֶγ����ڼ�¼��;
 */
struct  IRecord :public IKnownV1
{
	///���øü�¼��ָ���ֶε�ֵ
	/**������ֶ��Ѵ��ڣ�����ֶ�ֵ�����滻����ͨ��GetFieldValue()��ȡָ���ֶε�ֵ��
	*@param int   iFieldTag		�ֶε�TAG
	*@param char* szFieldValue	�ֶε�value
	*@return 0�ɹ�
	*/
	virtual int FUNCTION_CALL_MODE SetFieldValue( int iFieldTag, char * szFieldValue ) = 0;

	/// ����Tag�����ֶ�ֵ�������ֶβ������򷵻�NULL
	/**
	*@param int   iFieldTag	��Ҫ����ֶ�ֵ��TAG
	*@return ��TAG��Ӧ���ֶ�ֵ
	*/
	virtual char* FUNCTION_CALL_MODE GetFieldValue( int iFieldTag ) const = 0;

	/// ����Tag�����ֶ�ֵ,�����ֶβ������򷵻�ȱʡֵ
	/**
	*@param int   iFieldTag			��Ҫ����ֶε�TAG
	*@param char* lpDefaultValue	ȱʡֵ
	*@return ��TAG��Ӧ���ֶ�ֵ
	*/
	virtual char* FUNCTION_CALL_MODE GetFieldValueDefault( int iFieldTag, char* lpDefaultValue) const = 0;


	/// ����¼���Ƿ����iFieldTag�ֶΣ����ظ��飩
	/**
	*@param int   iFieldTag	������TAG
	*@return true ����	false ������
	*/
	virtual bool FUNCTION_CALL_MODE IsSetField( int iFieldTag ) const = 0;

	/// �Ӽ�¼��ɾ��iFieldTag�ֶΣ������ֶβ����ڣ����޲���
	/**
	*@param int   iFieldTag	��ɾ����TAG
	*/
	virtual void FUNCTION_CALL_MODE RemoveField( int iFieldTag ) = 0;

	/// ����¼������һ���ظ��飬����ظ����Ѵ棬�򷵻ص����Ѵ����ظ���ӿڡ�
	/**
	*@param int iGroupTag	�ظ���(������)Tag(��������Ϊһ���ظ����TAG����)
							����Ψһ�ı�־��GROUP
	*@param int iDelimTag	�ظ���ָ���TAG��һ��GROUP�ɶ�����¼��ɣ�iDelimTag
							��Ϊ��¼�ķָ����������ֶ�����¼
	*@return  IGroup �ṹָ��

	��Ϊ��	����һ���յ�Group�ṹ�����ÿ�Group�ṹ���뵽��¼�У�ͬʱ����
			�����ɵĸ�Group�ṹָ�롣������Ա�����ڻ�ø�ָ��󣬽���ȥ
			��Group�ṹ���в�����������Group�в��������¼��
	*/
	virtual IGroup* FUNCTION_CALL_MODE SetGroup( int iGroupTag, int iDelimTag ) = 0;

	/// ȡָ�����ظ���
	/**
	 *@param int iGroupTag �ظ���Tag��FIXЭ����Ϊ������Tag����IFTS��Ϊ��һ���ֶε�Tag
	 *@return IGroup �ṹָ��; ������ظ��鲻���ڣ�����NULL;
	 */
	virtual IGroup * FUNCTION_CALL_MODE GetGroup( int iGroupTag ) const = 0;
	
	///	ȡ��¼���ֶεĸ���
	virtual int FUNCTION_CALL_MODE GetFieldCount() const = 0;

	/// ȡ��¼�е�n���ֶε�TAG
	/**
	 *@param int n �ֶ���� (n��0��ʼ)
	 *@return TAG
	 */
	virtual int	FUNCTION_CALL_MODE GetFieldTag(int n) const = 0;

	/// ȡ��¼�е�n���ֶ�����
	/**
	 *@param int n �ֶ���� (n��0��ʼ)
	 *@return 0:��ͨ	1:�ظ���
	 */
	virtual int	FUNCTION_CALL_MODE GetFieldType(int n) const = 0;
};


///�ظ������ӿڣ���ʾ������¼��һ������(DataSet)
struct  IGroup :public IKnownV1
{
	///ȡ�ظ��������Tag��IFTSЭ����Ϊ�ظ����е�һ�ֶε�Tag
	virtual int FUNCTION_CALL_MODE GetFieldTag() const = 0;
	///ȡ�ظ���ָ���Tag (��һ�ֶ�)
	virtual int FUNCTION_CALL_MODE GetDelimTag() const = 0;
	
	/// �����ظ����ڼ�¼����
	virtual int FUNCTION_CALL_MODE GetRecordCount() const = 0;

	/// ȡ�ظ����е�ĳ����¼
	/**
	 *@param int num �ڼ�����¼(num��0��ʼ)
	 *@return IRecord �ṹָ��
	 */
	virtual IRecord * FUNCTION_CALL_MODE GetRecord( int num ) const = 0;

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
struct  IMessage :public IKnownV1
{
	///	��Header�еķ����ߺͽ����߶Ե���Ϊ����Ϣ�ķ����߽�����
	virtual void FUNCTION_CALL_MODE ReverseRoute( const HSFixHeader* ) = 0;
	/// �����Ϣͷ��¼
	virtual HSFixHeader* FUNCTION_CALL_MODE GetHeader() const = 0;
	/// �����Ϣβ��¼
	virtual HSFixTrailer* FUNCTION_CALL_MODE GetTrailer() const = 0;
	/// �����Ϣ���¼
	virtual HSFixMsgBody* FUNCTION_CALL_MODE GetMsgBody() const = 0;
	/// �����Ϣ����
	virtual const char* FUNCTION_CALL_MODE GetMessageType() const = 0;
	/// �����ϢЭ���ʶ
	virtual const char* FUNCTION_CALL_MODE GetBeginString() const = 0;

	/// ����Ϣת�����ִ�
	virtual std::string FUNCTION_CALL_MODE ToString() const = 0;
};



#endif //HSFIX_MESSAGE
