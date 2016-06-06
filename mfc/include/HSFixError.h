/****************************************************************************
**	´íÎó´úÂë¶¨Òå
****************************************************************************/


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
#define ERR_SocketError					25	/// IO Error
#define ERR_SocketSendFailed			26	/// Socket send operation failed
#define ERR_SocketRecvFailed			27	/// Socket recv operation failed
#define ERR_SocketCloseFailed			28	/// Socket close operation failed
