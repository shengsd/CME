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

	Packet::~Packet(void)
	{
	}

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

}