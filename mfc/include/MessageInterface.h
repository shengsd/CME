///恒生FIX引擎消息组织和解析接口
/****************************************************************************
**恒生FIX引擎消息对象结构：
**	(1)消息(IMessage 接口)
**     每条消息由消息头、消息体、消息尾三条记录(IRecord 接口)组成;
**     消息头中的BeginString字段表示该消息属于哪种协议；
**     消息头中的MessageType字段表示该消息属于哪种消息；
**
**  (2)记录(IRecord 接口)
**	   一条记录则由多个字段(Field)构成;每种字段都有一个数字型标签(TAG)来标识;
**     有二类字段:一类是普通字段，一类是重复组字段；
**    
**     一个普通字段由标志该字段的标签(TAG)和该字段的取值(Value)对组成;
**     一个重复组字段由标志该重复组的标签(TAG)和一个IGroup接口对组成;
**     
**  (3)重复组(IGroup接口)
**     重复组作为一种复杂类型的字段(Field)出现在记录(IRecord)中；
**     一个重复组由一条或多条记录所组成,相当于是一个记录集(DataSet);
**     如果重复组包含多条记录，则每条记录的字段顺序必须一致;
**     重复组的记录中还可以包含重复组字段，即重复组可以嵌套;
**     
**     重复组的TAG (FieldTag):
**         在FIX/STEP协议中为重复组的计数字段的TAG;
**         在IFTS协议中为重复组本身标签的TAG;
**         如果重复组内没有记录，则重复组的TAG字段也将不会出现在最终发送的消息中。
**     重复组记录中的分隔字段TAG (DelimTag):
**         FIX协议规定重复组记录中的第一个字段必须出现，并作为二条记录间的分隔标识。
**         IFTS协议无此要求，但本接口仍要求提供第一个字段的TAG ，以保持接口统一。
**
**     注意：当收到的消息中包含有重复组时，本引擎要求必须配置相应的数据词典，
**           否则不能解析消息中的重复组。
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
///标准接口查询,参照com标准
struct IKnownV1
{
	///接口查询
	/**
	 *@param const int iid 接口全局唯一标识
	 *@param void **ppv 返回iid对应的接口指针（接口参照com标准）
	 *@return I_OK 成功，I_NONE 未查到iid 相应接口
	 */
    virtual unsigned long  FUNCTION_CALL_MODE QueryInterface(const int & iid, IKnownV1 **ppv ) = 0;
    virtual unsigned long  FUNCTION_CALL_MODE AddRef() = 0;
    virtual unsigned long  FUNCTION_CALL_MODE Release() =  0;
};
#endif

struct IGroup;

///	记录对象接口，表示一条记录
/**记录中的每个字段包含TAG以及和该TAG对应的VALUE;
 * 一个重复组作为一个特殊的字段出现在记录中;
 */
struct  IRecord :public IKnownV1
{
	///设置该记录中指定字段的值
	/**如果该字段已存在，则对字段值进行替换，可通过GetFieldValue()获取指定字段的值。
	*@param int   iFieldTag		字段的TAG
	*@param char* szFieldValue	字段的value
	*@return 0成功
	*/
	virtual int FUNCTION_CALL_MODE SetFieldValue( int iFieldTag, char * szFieldValue ) = 0;

	/// 根据Tag返回字段值，若该字段不存在则返回NULL
	/**
	*@param int   iFieldTag	需要获得字段值的TAG
	*@return 和TAG对应的字段值
	*/
	virtual char* FUNCTION_CALL_MODE GetFieldValue( int iFieldTag ) const = 0;

	/// 根据Tag返回字段值,若该字段不存在则返回缺省值
	/**
	*@param int   iFieldTag			需要获得字段的TAG
	*@param char* lpDefaultValue	缺省值
	*@return 和TAG对应的字段值
	*/
	virtual char* FUNCTION_CALL_MODE GetFieldValueDefault( int iFieldTag, char* lpDefaultValue) const = 0;


	/// 检查记录中是否存在iFieldTag字段（或重复组）
	/**
	*@param int   iFieldTag	待检查的TAG
	*@return true 存在	false 不存在
	*/
	virtual bool FUNCTION_CALL_MODE IsSetField( int iFieldTag ) const = 0;

	/// 从记录中删除iFieldTag字段，若该字段不存在，则无操作
	/**
	*@param int   iFieldTag	待删除的TAG
	*/
	virtual void FUNCTION_CALL_MODE RemoveField( int iFieldTag ) = 0;

	/// 往记录中增加一个重复组，如该重复组已存，则返回的是已存在重复组接口。
	/**
	*@param int iGroupTag	重复组(计数域)Tag(计数域作为一个重复组的TAG处理)
							可以唯一的标志该GROUP
	*@param int iDelimTag	重复组分隔域TAG，一个GROUP由多条记录组成，iDelimTag
							作为记录的分隔符，以区分多条记录
	*@return  IGroup 结构指针

	行为：	生成一个空的Group结构，将该空Group结构插入到记录中，同时返回
			刚生成的该Group结构指针。开发人员可以在获得该指针后，接下去
			对Group结构进行操作，可以在Group中插入多条记录。
	*/
	virtual IGroup* FUNCTION_CALL_MODE SetGroup( int iGroupTag, int iDelimTag ) = 0;

	/// 取指定的重复组
	/**
	 *@param int iGroupTag 重复组Tag，FIX协议中为计算域Tag，在IFTS中为第一个字段的Tag
	 *@return IGroup 结构指针; 如果该重复组不存在，返回NULL;
	 */
	virtual IGroup * FUNCTION_CALL_MODE GetGroup( int iGroupTag ) const = 0;
	
	///	取记录中字段的个数
	virtual int FUNCTION_CALL_MODE GetFieldCount() const = 0;

	/// 取记录中第n个字段的TAG
	/**
	 *@param int n 字段序号 (n从0开始)
	 *@return TAG
	 */
	virtual int	FUNCTION_CALL_MODE GetFieldTag(int n) const = 0;

	/// 取记录中第n个字段类型
	/**
	 *@param int n 字段序号 (n从0开始)
	 *@return 0:普通	1:重复组
	 */
	virtual int	FUNCTION_CALL_MODE GetFieldType(int n) const = 0;
};


///重复组对象接口，表示多条记录的一个集合(DataSet)
struct  IGroup :public IKnownV1
{
	///取重复组计数域Tag，IFTS协议中为重复组中第一字段的Tag
	virtual int FUNCTION_CALL_MODE GetFieldTag() const = 0;
	///取重复组分隔域Tag (第一字段)
	virtual int FUNCTION_CALL_MODE GetDelimTag() const = 0;
	
	/// 返回重复组内记录个数
	virtual int FUNCTION_CALL_MODE GetRecordCount() const = 0;

	/// 取重复组中的某条记录
	/**
	 *@param int num 第几条记录(num从0开始)
	 *@return IRecord 结构指针
	 */
	virtual IRecord * FUNCTION_CALL_MODE GetRecord( int num ) const = 0;

	/// 在重复组中追加一条空白记录
	/**
	 * @return IRecord 追加的记录指针
	行为:	调用一次AddRecord操作,函数创建一个空的Record,并且将该Record插入
			重复组,返回刚创建的Record指针,开发人员获得该指针后,可以往Record
			中插入字段(普通字段或者又是一个重复组字段)
	*/
	virtual IRecord * FUNCTION_CALL_MODE AddRecord( )  = 0;
};

	
typedef IRecord HSFixHeader;
typedef IRecord HSFixTrailer;
typedef IRecord HSFixMsgBody;

///消息，包含消息体，同时嵌套Header，Trailer
struct  IMessage :public IKnownV1
{
	///	将Header中的发送者和接收者对调称为本消息的发送者接收者
	virtual void FUNCTION_CALL_MODE ReverseRoute( const HSFixHeader* ) = 0;
	/// 获得消息头记录
	virtual HSFixHeader* FUNCTION_CALL_MODE GetHeader() const = 0;
	/// 获得消息尾记录
	virtual HSFixTrailer* FUNCTION_CALL_MODE GetTrailer() const = 0;
	/// 获得消息体记录
	virtual HSFixMsgBody* FUNCTION_CALL_MODE GetMsgBody() const = 0;
	/// 获得消息类型
	virtual const char* FUNCTION_CALL_MODE GetMessageType() const = 0;
	/// 获得消息协议标识
	virtual const char* FUNCTION_CALL_MODE GetBeginString() const = 0;

	/// 将消息转化成字串
	virtual std::string FUNCTION_CALL_MODE ToString() const = 0;
};



#endif //HSFIX_MESSAGE
