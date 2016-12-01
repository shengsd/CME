/****************************************************************************
  源程序名称:hsfix2_error.h
  软件著作权:恒生电子股份有限公司
  系统名称:  HSFIX20
  模块名称:  接口文件
  功能说明:  
			错误代码定义
				
  作    者:  maoyj
  开发日期:  20140415
  备    注:  
  
**/
#ifndef __HSFIX20_ERROR_DEF__H__
#define __HSFIX20_ERROR_DEF__H__

#define Succeed							0
#define ERR_FieldNotFound				1	/// Field not found inside a message
#define ERR_FieldConvertError			2	/// Unable to convert field into its native format
#define ERR_MessageParseError			3	/// Unable to parse message
#define ERR_InvalidMessage				4	/// Not a recognizable message
#define ERR_ConfigError					5	/// Application is not configured correctly
#define ERR_RuntimeError				6	/// Application encountered serious error during runtime
#define ERR_InvalidTagNumber			7	/// Tag number does not exist in specification
#define ERR_RequiredTagMissing			8	/// Required field is not in message
#define ERR_TagNotDefinedForMessage		9	/// Field does not belong to message
#define ERR_NoTagValue					10	/// Field exists in message without a value
#define ERR_IncorrectTagValue			11	/// Field has a value that is out of range
#define ERR_IncorrectDataFormat			12	/// Field has a badly formatted value
#define ERR_IncorrectMessageStructure	13	/// Message is not structured correctly
#define ERR_DuplicateFieldNumber		14	/// Field shows up twice in the message
#define ERR_InvalidMessageType			15	/// Not a known message type
#define ERR_UnsupportedMessageType		16	/// Message type not supported by application
#define ERR_UnsupportedVersion			17	/// Version of %FIX is not supported
#define ERR_TagOutOfOrder				18	/// Tag is not in the correct order
#define ERR_RepeatedTag					19	/// Repeated tag not part of repeating group
#define ERR_RepeatingGroupCountMismatch	20	/// Repeated group count not equal to actual count
#define ERR_DoNotSend					21	/// Indicates user does not want to send a message
#define ERR_RejectLogon					22	/// User wants to reject permission to logon
#define ERR_SessionNotFound				23	/// Session cannot be found for specified action
#define ERR_IOError						24	/// IO Error
#define ERR_SocketError					25	/// SocketError
#define ERR_SocketSendFailed			26	/// Socket send operation failed
#define ERR_SocketRecvFailed			27	/// Socket recv operation failed
#define ERR_SocketCloseFailed			28	/// Socket close operation failed
#define ERR_NotEnoughMemory				29	/// Not Enough Memory
#define ERR_CreateConfigObj				30	/// ERR_CreateConfigObj
#define ERR_LoadConfig					31	/// ERR_LoadConfig
#define ERR_Inited						32	/// ERR_Inited
#define ERR_CreateMsgBody				33	/// ERR_CreateMsgBody
#define ERR_NotWorkingTime				34	/// ERR_CreateMsgBody
#define ERR_NotSupport					35	/// ERR_NotSupport

// zhouwh 2015-06-25		增加会话未登陆错误码
#define	ERR_NotLoggedOn					36	/// ERR_NotLoggedOn

#define ERR_UNKnown						36	/// UNKnown

const char szErrInfo[100][64]={
	"Field not found inside a message",
	"Unable to convert field into its native format",
    "Unable to parse message",
    "Not a recognizable message",
    "Application is not configured correctly",
    "Application encountered serious error during runtime",
    "Tag number does not exist in specification",
    "Required field is not in message",
    "Field does not belong to message",
    "Field exists in message without a value",
    "Field has a value that is out of range",
    "Field has a badly formatted value",
    "Message is not structured correctly",
    "Field shows up twice in the message",
    "Not a known message type",
    "Message type not supported by application",
    "Version of %FIX is not supported",
    "Tag is not in the correct order",
    "Repeated tag not part of repeating group",
    "Repeated group count not equal to actual count",
    "Indicates user does not want to send a message",
    "User wants to reject permission to logon",
    "Session cannot be found for specified action",
    "IO Error",
    "SocketError",
    "Socket send operation failed",
    "Socket recv operation failed",
    "Socket close operation failed",
    "Not Enough Memory",
	"Can Not Create Config Obj",
	"Can Not Load the Config File",
	"Already Inited",
	"Can Not Create MsgBody",
	"Sending Not Woring Time",
	"Not Support",
	"UNKnown"
};
#endif //	__HSFIX20_ERROR_DEF__H__
