/****************************************************************************
  源程序名称:hsfix2_messageinterface.h
  软件著作权:恒生电子股份有限公司
  系统名称:  HSFIX20
  模块名称:  接口文件
  功能说明:  
			恒生FIX引擎消息组织和解析接口
				
  作    者:  maoyj
  开发日期:  20140415
  备    注:  
**恒生FIX引擎消息对象结构：
**	(1)消息(IMessage 接口)
**     每条消息由消息头、消息体、消息尾三条记录(IRecord 接口)组成;
**     消息头中的BeginString字段表示该消息属于哪种协议版本；FIX4.2   FIX4.4  etc
**     消息头中的MessageType字段表示该消息属于哪种消息类型； 
**
**  (2)记录(IRecord 接口)
**	   一条记录则由多个字段(Field)构成;每种字段都有一个数字型标签(TAG)来标识;
**     有二类字段:一类是普通字段，一类是重复组字段；
**    
**     一个普通字段由标志该字段的标签(TAG)和该字段的取值(Value)  {TAG, VALUE}对组成;
**     一个重复组字段由标志该重复组的标签(TAG)和一个IGroup接口  {TAG, IGroup}对组成;
**     
**  (3)重复组(IGroup接口)
**     重复组作为一种复杂类型的字段(Field)出现在记录(IRecord)中；
**     一个重复组由一条或多条记录所组成,相当于是一个记录集(DataSet);
**     如果重复组包含多条记录，则每条记录的字段顺序必须一致;
**     重复组的记录中还可以包含重复组字段，即重复组可以嵌套;
**     
**     重复组的TAG (FieldTag):
**         在FIX/STEP协议中为重复组的计数字段的TAG;
**         如果重复组内没有记录，则重复组的TAG字段也将不会出现在最终发送的消息中。
**     重复组记录中的分隔字段TAG (DelimTag):
**         FIX协议规定重复组记录中的第一个字段必须出现，并作为二条记录间的分隔标识。
**
**     注意：本引擎要求必须配置相应的数据词典,哪怕不存在重复组!!
**						 
**     重要:
**         本实现中,消息是线程不安全的,任何多线程使用同一消息,均会导致不可预估的后果!!!!!!!!
**         在如下场景下,多线程使用是安全的:
**             多个线程均使用的是消息的const接口,在这同时没有任何线程使用非const接口
**				但是即令这种场景下,也不建议用户多线程使用

修改记录：
	// 2015-08-24	zhouwh	消息模块增加配置文件，以实现fix消息的合法性校验
****************************************************************************/

#ifndef __HSFIX20_MESSAGE_INTERFACE__H__
#define __HSFIX20_MESSAGE_INTERFACE__H__
#include "hsfix2_base_interface.h"

struct IGroup;

///	记录对象接口，表示一条记录
/**记录中的每个字段包含TAG以及和该TAG对应的VALUE;	
 * 一个普通字段,就是一个{TAG, VALUE}对
 * 一个重复组作为一个特殊的字段出现在记录中;重复组就是一个{TAG, IGroup}对
 * 普通字段和重复组字段由不同的接口操作
 */
struct  IRecord : public IFIXBase
{
	virtual ~IRecord(){};
	/// 根据Tag返回字段值，若该字段不存在则返回NULL
	/**
	*@param int   iFieldTag	需要获得字段值的TAG
	*@return 和TAG对应的字段值,如不存在,返回NULL
	*/
	virtual const char* FUNCTION_CALL_MODE GetFieldValue( int iFieldTag ) const = 0;
	/// 根据Tag返回字段值,若该字段不存在则返回缺省值
	/**
	*@param int   iFieldTag			需要获得字段的TAG
	*@param char* lpDefaultValue	当iFieldTag 不存在,则返回缺省值
	*@return 和TAG对应的字段值,在返回缺省值的情况下,返回的就是lpDefaultValue指针本身
	*/
	virtual const char* FUNCTION_CALL_MODE GetFieldValueDefault( int iFieldTag, const char* lpDefaultValue) const = 0;
	/// 根据Tag返回字段值，若该字段不存在则返回NULL,且置指针 ilpRawlen指向的值 为0
	/**
	*@param int   iFieldTag	需要获得字段值的TAG
	*@param int*  ilpRawlen	该字段数据长度
	*@return 和TAG对应的字段值,如不存在,返回NULL
	*/
	virtual const void* FUNCTION_CALL_MODE GetFieldRawValue( int iFieldTag,  int* ilpRawlen ) const = 0 ;
	/// 取指定的重复组
	/**
	 *@param  int iGroupTag 重复组Tag
	 *@return IGroup 结构指针; 如果该重复组不存在，返回NULL;
	 */
	virtual IGroup * FUNCTION_CALL_MODE GetGroup( int iGroupTag ) const = 0;
	///	重复组和普通字段通用的接口
	/// 检查记录中是否存在iFieldTag字段（或重复组）
	/**
	*@param int   iFieldTag	待检查的TAG
	*@return true 存在	false 不存在
	*/
	virtual bool FUNCTION_CALL_MODE IsSetField( int iFieldTag ) const = 0;


	///	操作普通字段的接口
	/// 设置该记录中指定字段的值
	/** 如果该字段已存在，则会被替换
	*@param int   iFieldTag		字段的TAG
	*@param const char* lpFieldValue	字段的value
	*@return 0成功	其他:失败
	*/
	virtual int FUNCTION_CALL_MODE SetFieldValue( int iFieldTag, const char * lpFieldValue ) = 0;
	/// 设置该记录中指定字段的值(rawdata类型字段）
	/** 如果该字段已存在，则会被替换
	*@param int   iFieldTag		字段的TAG
	*@param const char* lpFieldRaw	字段的value
	*@param int iRawlen	字段的value的长度
	*@return 0成功	其他:失败
	*/
	virtual int FUNCTION_CALL_MODE SetFieldRawValue( int iFieldTag, const void * lpFieldRaw, int iRawlen ) = 0;
	///	操作重复组的接口
	/// 往记录中增加一个重复组，如该重复组已存在，则返回的是已存在重复组接口。
	/**
	*@param int iGroupTag	重复组(计数域)Tag(计数域作为一个重复组的TAG处理)
							可以唯一的标志该GROUP
	*@param int iDelimTag	重复组分隔域TAG，一个GROUP由多条记录组成，iDelimTag
							作为记录的分隔符，以区分多条记录
	*@return  IGroup 结构指针
	行为：	生成一个空的IGroup结构，将该空IGroup结构插入到记录中，同时返回
			刚生成的该IGroup结构指针。开发人员可以在获得该指针后，接下去
			对IGroup结构进行操作，可以在IGroup中插入多条记录。
	*/
	virtual IGroup* FUNCTION_CALL_MODE SetGroup( int iGroupTag, int iDelimTag ) = 0;
	/// 从记录中删除iFieldTag字段，若该字段不存在，则无操作
	/**
	*@param int   iFieldTag	待删除的TAG
	*/
	virtual void FUNCTION_CALL_MODE RemoveField( int iFieldTag ) = 0;
	
	/// zhouwh 2015-06-24 为上海证通项目增加，以实现fix报文转换为pack
	/**
		根据字段的TAG获取字段名称
	@param int iFieldTag	字段的TAG
	@param char **lpDst		用户传入的用于存储字段名称的缓存指针
	@param int iDstLen		用户传入的缓存的长度
	*/
	virtual void FUNCTION_CALL_MODE GetFieldNameByTag(int iFieldTag, char **lpDst, int iDstLen) const=0;
	/**
		判断字段类型是否是二进制类型
	@param	int iFieldTag	需要判断的字段的TAG
	@return	是二进制类型，返回TRUE 否则，返回FALSE
	*/
	virtual bool FUNCTION_CALL_MODE IsRawDataField(int iFieldTag) const=0;
	/**
		根据字段名称获取字段TAG
	@param	const char *lpFieldName		字段名称
	@return	字段名称合法，返回对应字段TAG；否则，返回-1
	*/
	virtual int FUNCTION_CALL_MODE GetFieldTagByName(const char *lpFieldName) const = 0;
	/**
		根据重复组TAG获取该重复组的分隔域TAG
	@param	int iGroupID	重复组TAG
	@return iGroupID是合法重复组，返回其分隔域TAG；否则，返回-1
	*/
	virtual int FUNCTION_CALL_MODE GetDelimByGroup(int iGroupID) const = 0;
	///////////////////////////////////////////////////////////////////////


	///	以下是遍历记录的接口	一般应用程序无序用到本接口
	/// 开始遍历前。必须先调用BeginTraverse，否则遍历结果不可预知
	/// 返回0,说明成功, 否则失败
	virtual int FUNCTION_CALL_MODE BeginTraverse () = 0;
	virtual int FUNCTION_CALL_MODE GetCurrentFieldTag () const= 0;
	/// 返回0表示普通字段	1表示重复组
	virtual int FUNCTION_CALL_MODE GetCurrentFieldType () const= 0;
	virtual const char* FUNCTION_CALL_MODE GetCurrentFieldValue () const= 0;
	//  返回小于0,说明遍历结束
	virtual int FUNCTION_CALL_MODE Next () = 0;
};


///重复组对象接口，表示多条记录的一个集合(DataSet)
//	重复组在没有数据词典的情况下,难以定位
struct  IGroup : public IFIXBase
{
	virtual ~IGroup(){};
	/// 取重复组计数域Tag
	virtual int FUNCTION_CALL_MODE GetFieldTag() const = 0;
	/// 取重复组分隔域Tag (第一字段)
	virtual int FUNCTION_CALL_MODE GetDelimTag() const = 0;
	
	///	遍历接口
	/// 返回重复组内记录个数
	virtual int FUNCTION_CALL_MODE GetRecordCount() const = 0;
	/// 取重复组中的某条记录
	/**
	 *@param int iIndex 第几条记录(iIndex从0开始)
	 *@return IRecord 结构指针
	 */
	virtual IRecord * FUNCTION_CALL_MODE GetRecord( int iIndex ) const = 0;
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
//	必须先SetMessageType设置消息类型,然后才能操作消息体(因为操作消息体需要和数据词典绑定)
//	而不同的消息类型,其数据词典不一致
struct  IMessage : public IFIXBase
{
	virtual ~IMessage(){};
	/// 获得消息头记录
	virtual HSFixHeader* FUNCTION_CALL_MODE GetHeader() const = 0;
	/// 获得消息尾记录
	virtual HSFixTrailer* FUNCTION_CALL_MODE GetTrailer() const = 0;
	/// 获得消息体记录
	virtual HSFixMsgBody* FUNCTION_CALL_MODE GetMsgBody() const = 0;
	
	
	///	以下函数,操作消息头中常见的几个字段,以简化调用,实际上,用户也可以通过首先调用
	///	GetHeader获得消息头部,然后操作消息头中具体的TAG来获得同等效果
	/// 获得消息类型,获取失败,则返回NULL
	virtual const char* FUNCTION_CALL_MODE GetMessageType() const = 0;
	/// 设置消息类型
	virtual int FUNCTION_CALL_MODE SetMessageType(const char*) = 0;
	/// 获得消息协议标识
	virtual const char* FUNCTION_CALL_MODE GetBeginString() const = 0;
	/// 设置消息协议标识
	virtual int FUNCTION_CALL_MODE SetBeginString(const char*) = 0;
	/// 获得发送者标识
	virtual const char* FUNCTION_CALL_MODE GetSenderID() const = 0;
	/// 设置发送者标识
	virtual int FUNCTION_CALL_MODE SetSenderID(const char*) = 0;
	/// 获得目标标识
	virtual const char* FUNCTION_CALL_MODE GetTargetID() const = 0;
	/// 设置目标标识
	virtual int FUNCTION_CALL_MODE SetTargetID(const char*) = 0;
	///////////////////////////////////////////////////////////////////////
	
	
	///	将Header中的发送者和接收者对调成为本消息的发送者接收者
	///	本接口一般用以FIX网关类应用,  一般应用不需要调用
	virtual void FUNCTION_CALL_MODE ReverseRoute( const HSFixHeader* ) = 0;


	///	清空消息如果应用需要重复使用消息,不想每次均申请释放,则在每次重新使用该消息前
	///	必须调用清空接口,以避免和上次的使用混淆
	virtual int FUNCTION_CALL_MODE ClearMsg() = 0;
	//	复制一条消息,引擎会创建一条新消息,复制本消息的内容返回给调用者
	//	调用者可以将新复制出来的消息交给引擎发送,或者自行在合适的时候释放
	virtual IMessage* FUNCTION_CALL_MODE CloneMsg() const=0;
	
	
	
	///	!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	///	以下2方法,仅供独立使用消息解析模块的应用调用,只有从绑定数据词典的消息工厂中
	///	分配的消息,才能正确的调用以下2方法.
	///	以下2函数需要配合数据词典才能正确操作,所以为避免不安全的操作,在消息未绑定
	///	数据词典的情况下,执行下面2个接口方法,将报错
	/**
	 *@param out ilpBuffLen 二进制流的长度
	 *@return const char*	二进制流
	 */
	/// 反序列化,获得字符串   
	virtual const char* FUNCTION_CALL_MODE GetBuffer(int* ilpBuffLen) = 0;
	/// 序列化,根据字符串获得消息结构
	/**
	 *@param in const char*lpBuff	传入的二进制流首地址
	 *@param in int iBuffLen		传入的二进制流字节数
	 *@return int	0: 成功	其他:	失败
	 */
	virtual int FUNCTION_CALL_MODE SetBuffer(const char*lpBuff, int iBuffLen) = 0;
	///////////////////////////////////////////////////////////////
	
	
	//////////////////////////////////////////////////////////////////////////
	///	跟消息有效期相关的接口			以下三接口暂时不实现
	///	获得消息年龄(从消息生成到现在经过的毫秒数)
	virtual int FUNCTION_CALL_MODE GetMsgAges() const = 0;
	///	该消息允许的最大年龄
	virtual int FUNCTION_CALL_MODE GetMaxAges() const = 0;
	///	再经过多少毫秒,本消息就会失效
	virtual int FUNCTION_CALL_MODE GetTimesToDie() const = 0;
	//////////////////////////////////////////////////////////////////////////


	///	释放消息资源,本操作会将消息归还到合适的池，并且消息内部的重复组记录也会归还
	/// 注意： Release 和 AddRef 接口仅仅是空函数调用，并不会有真正的引用计数操作（保留这2接口仅为兼容delphi）
	virtual void FUNCTION_CALL_MODE FreeMsg() = 0;
	/// 当 消息流化 / 解析 失败时,可通过该函数获得详细错误信息(目前未实现,所有错误均会记录日志)
	virtual const char* FUNCTION_CALL_MODE GetFailureReason() = 0;

	// 2015-08-24	zhouwh	消息模块增加配置文件，以实现fix消息的合法性校验
	/// 序列化,根据字符串获得消息结构
	/**
	 *@param in const char*lpBuff	传入的二进制流首地址
	 *@param in int iBuffLen		传入的二进制流字节数
	 *@param in IMessage* lpMsgReturn		传入的消息接口，如果发生错误，引擎会填充lpMsgReturn
	 *@return int	0: 成功
	 *				-1: 错误，lpMsgReturn已经填充完毕
					-2：输入参数非法，lpMsgReturn未填充
					-3: 解析fix报文出错，lpMsgReturn未填充
					-4: 非法的fix报文，lpMsgReturn未填充
					-5: 不支持的fix协议版本，lpMsgReturn未填充
					-6: 其他错误，lpMsgReturn未填充
	 */
	virtual int FUNCTION_CALL_MODE SetBufferWithErrReturn(const char*lpBuff, int iBuffLen, IMessage* lpMsgReturn) = 0; 
};



////	注意:	对于拥有完整FIX会话的应用，不要去创建消息工厂，如果需要消息，应该从自身的会话中获得
/**
	消息工厂仅给仅仅处理FIX消息，而没有完整FIX会话的应用使用
**/
//	从该消息工厂分配的消息,在内部会绑定数据词典
struct IMessageFactory : public IFIXBase
{
	virtual ~IMessageFactory(){};

	/// 只要资源足够,总能返回一个空的消息,只有资源不足,才返回NULL
	virtual IMessage* FUNCTION_CALL_MODE GetEmptyMessage()=0;
	/// 当数据非法时,会返回NULL,所以,应用一定要判断返回值
	virtual IMessage* FUNCTION_CALL_MODE GetFixMessage(const char*lpBuff, int iBuffLen)=0;
	
	//	根据数据词典做消息的合法性检测(对于引擎用户,无须使用本接口)
	//	0: 合法	其他:非法
	virtual int FUNCTION_CALL_MODE ValidMessage(IMessage * lpMessage)=0;

	//// 2015-08-24	zhouwh	消息模块增加配置文件，以实现fix消息的合法性校验
	///// 当数据非法时，会返回NULL，且相应的拒绝消息由lpErrMsgReturn返回
	///// lpErrMsgReturn由调用者负责申请和释放，引擎只在发生错误时，负责填充
	//virtual IMessage* FUNCTION_CALL_MODE GetFixMessageWithErrReturn(const char* lpBuff, int iBuffLen, IMessage* lpErrMsgReturn)=0;
};

#ifdef __cplusplus
extern "C"{
#endif

//	创建一个符合特定数据词典的消息工厂,所有本工厂创建的消息均会和lpDataDictionaryFile绑定
/**
 *@param char* lpDataDictionaryFile	数据词典文件,若为NULL,则不和任何数据词典绑定
 				对于孤立的使用消息的应用需要传入数据词典,以实现合法性校验
 				而对于完整的使用fix会话的应用,则可以创建一个不和任意数据词典绑定的消息工厂
 				因为会话本身已经实现了和数据词典的绑定
 *@return 消息工厂指针
 */
FIXENGAPI IMessageFactory* FUNCTION_CALL_MODE CreateSpecialMsgFactory(char* lpDataDictionaryFile);

// 2015-08-24	zhouwh	消息模块增加配置文件，以实现fix消息的合法性校验
FIXENGAPI IMessageFactory* FUNCTION_CALL_MODE CreateMsgFactory(char* lpDataDictionaryFile, char* lpCfgFile);

/**	摧毁一个消息工厂
**/
FIXENGAPI void FUNCTION_CALL_MODE DestroyMessageFactory(IMessageFactory*);
#ifdef __cplusplus
}
#endif

#endif //__HSFIX20_MESSAGE_INTERFACE__H__

