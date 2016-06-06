#include "stdafx.h"
#include "Packet.h"

namespace MDP
{
	Packet::Packet()
	{
		m_RtsSent = false;
		m_packetSize = 0;
		memset(m_szPacket, 0, MAXPACKETSIZE);
		//m_pPacketHeader = (PacketHeader* )m_szPacket;
		time(&m_createTime);
	}
/*
	Packet::Packet( const char* str, ssize_t len )
	{
		m_RtsSent = false;
		m_pPacketHeader = (PacketHeader *)str;
		m_packetSize = len;
	}
*/

	Packet::~Packet(void)
	{
	}
	/*
	void Packet::setPacket( const char* str )
	{
		m_pPacketHeader = (PacketHeader* )str;
	}

	void Packet::setPacket( const char* str, ssize_t len )
	{
		m_pPacket = (char* )str;
		m_pPacketHeader = (PacketHeader *)str;
		m_packetSize = len;
	}
	*/
	UINT32 Packet::getSeqNum()
	{
		PacketHeader* m_pPacketHeader = (PacketHeader* )m_szPacket;
		return m_pPacketHeader->mSeqNum;
	}

	int Packet::getTimeLimit()
	{
		time_t now;
		time(&now);
		return (int)(now - m_createTime);
	}

/*
	bool Packet::containsSeqNum( unsigned sn )
	{
		if ( m_pPcketHeader->mSeqNum > sn || (m_pPcketHeader->mSeqNum + m_pPcketHeader->mMsgCount - 1) < sn )
		{
			return false;
		}
		return true;
	}

	unsigned int Packet::getSeqNum()
	{
		return m_pPcketHeader->mSeqNum;
	}

	unsigned int Packet::getMsgCount()
	{
		return m_pPcketHeader->mMsgCount;
	}

	void Packet::extractMessage( Message& msg, unsigned int msgProcessCount )
	{
		unsigned int offset = sizeof( PacketHeader );
		char* p = (char* )((char* )m_pPcketHeader + offset);
		MsgHeader* pMsgHeader = (MsgHeader* )p;
		offset += pMsgHeader->MsgSize;
		while ( msgProcessCount )
		{
			p += offset;
			pMsgHeader = (MsgHeader* )p;
			offset += pMsgHeader->MsgSize;
			--msgProcessCount;
		}
		//ур╣╫
		msg.setMessage( p );
	}
*/
}