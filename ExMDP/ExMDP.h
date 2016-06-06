#ifndef _EXMDP_H_
#define _EXMDP_H_

// ���� ifdef ���Ǵ���ʹ�� DLL �������򵥵�
// ��ı�׼�������� DLL �е������ļ��������������϶���� EXMDP_EXPORTS
// ���ű���ġ���ʹ�ô� DLL ��
// �κ�������Ŀ�ϲ�Ӧ����˷��š�������Դ�ļ��а������ļ����κ�������Ŀ���Ὣ
// EXMDP_API ������Ϊ�Ǵ� DLL ����ģ����� DLL ���ô˺궨���
// ������Ϊ�Ǳ������ġ�
#ifdef EXMDP_EXPORTS
#define EXMDP_API __declspec(dllexport)
#else
#define EXMDP_API __declspec(dllimport)
#endif

///MDP����ص��ӿڣ������ʹ����ʵ�ִ˽ӿڣ�������Ӧ���淢���ĸ����¼�	
/**
 * �����ʹ����ʵ�ִ˽ӿڣ����ڳ�ʼ��ʱ������ע��˽ӿڣ�
 */
//#include <stdint.h>
typedef __int64 int64_t;
typedef unsigned __int64 uint64_t;

typedef struct tagConfigStruct
{
	char configFile[128];	//CME�ṩ�������ļ�
	char localInterface[16]; //����interface
	char templateFile[128];	//CME���鱨�Ľ���ģ��
	char userName[64];		//�ط������û���
	char passWord[64];		//�ط���������
	char errorInfo[128];	//����ʧ�ܷ��صĴ�����Ϣ
}ConfigStruct;

//һ���ֶΣ�һ��Ҫ����ģ���ļ����type����ѡ���ȡֵ�ĺ���
class MDPField
{
public:
	///�����ֶ�ֵ�����͵��ö�Ӧ�ĺ���
	/**
	*@param  const int index	�ֶ��е�index��ֵ��Ĭ��-1��ʾ�׸���һ������²���Ҫ��ֵ
	*@return ��Ӧ���͵�ֵ
	*/
	virtual int64_t getInt(const int index = -1) const = 0;

	virtual uint64_t getUInt(const int index  = -1) const = 0;

	virtual double getDouble(const int index = -1) const =0;

	///�ֶ�ֵ�ĳ���
	/**
	*@param  const int index = -1		�ֶ��е�index��ֵ
	*@return int ֵ�ĳ���
	*/
	virtual int length(const int index = -1) const = 0;

	///��ȡ�ֶ�ֵ������Ϊ�ַ�����
	/**
	*@param  const int index	�ֶ��е�index��ֵ��һ�������Ϊ0
	*@param  char *dst			��Ҫ��������Ŀ���ַ
	*@param  const int offset	Դ�ַ������, һ�������Ϊ0
	*@param  const int length	�ַ������ȣ�һ������¿���ͨ��length()��ȡ
	*/
	virtual void getArray(const int index, char *dst, const int offset, const int length) const = 0;
};

//һ����¼���������ظ��Ķ���ֶΣ�
class MDPFieldMap
{
public:
	///��ȡ��ʾ�ظ����ֶα�ǩ��Ӧ�ļ�¼����
	/**
	*@param  int tag	�ظ����ֶα�ǩ
	*@return int		��Ӧ�ļ�¼����
	*/
	virtual int getFieldMapNumInGroup(int tag) = 0;

	///��ȡ�ظ����ֶα�ǩ��Ӧ�ĵ�index����¼
	/**
	*@param  int tag		�ظ����ֶα�ǩ
	*@return MDPFieldMap*	��Ӧ�ļ�¼ָ��
	*/
	virtual MDPFieldMap* getFieldMapPtrInGroup(int tag, int index) = 0;

	///��ȡ��ǰ��¼���ֶα�ǩ��Ӧ���ֶ�
	/**
	*@param  int tag		�ֶα�ǩ
	*@return MDPField*		��Ӧ���ֶ�ָ��
	*/
	virtual MDPField* getField(int tag) = 0;
};

//ʹ���߱��붨�������࣬��ʵ�����еĻص�����
class Application
{
public:
	///���鱨�Ļص�����
	/**
	*@param  MDPFieldMap* pMDPFieldMap		���ļ�һ����¼
	*@return const int templateID			���ĵ�ģ��ID
	*/
	virtual void onMarketData( MDPFieldMap* pMDPFieldMap, const int templateID) = 0;
};

#ifdef __cplusplus
extern "C"{
#endif

/////����������������
/**
*@param  ConfigStruct* configStruct		������Ϣ
*@param  Application* application		����ص���
*@param  LPFN_WRITELOG lpfnWriteLog		��־����ָ��
*@return int	0 - �ɹ�  else - ʧ��
*/
EXMDP_API int StartEngine( ConfigStruct* configStruct, Application* application );

///��������رպ���
/**
*@return int 0 - �ɹ�  else - ʧ��
*/
EXMDP_API int StopEngine();

#ifdef __cplusplus
}
#endif

#endif

