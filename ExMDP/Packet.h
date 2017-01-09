#pragma once
#include <time.h>
#define MAXPACKETSIZE 4096

namespace MDP
{
	typedef struct XdpPacketHeader
	{
		UINT32 SeqNum;
		UINT64 SendTime;
	}PacketHeader;

	class Packet
	{
	public:
		Packet(char* str, int size )
		{
			m_pPacket = str;
			m_packetSize = size;
		}
		~Packet()
		{
		}

		//获取数据包序号
		UINT32 getSeqNum()
		{
			PacketHeader* m_pPacketHeader = (PacketHeader* )m_pPacket;
			return m_pPacketHeader->SeqNum;
		}

		//获取数据包指针
		char* getPacketPointer() { return m_pPacket; }

		//获取数据包大小
		int getPacketSize() { return m_packetSize; }

	private:
		char *m_pPacket;

		int m_packetSize;
	};

	class SpoolPacket
	{
	public:
		SpoolPacket()
		{
		}

		SpoolPacket(char* str, int size)
		{
			memcpy(m_packet, str, size);
			m_packetSize = size;
			time(&m_createTime);
			m_RtsSent = false;
		}

		//是否已经请求了重发
		bool isRetransRequested() { return m_RtsSent; }

		//重发设置
		void setRetransRequested( bool value ) { m_RtsSent = value; }

		//返回当前时间与创建时间的差值（单位：秒）
		int getTimeLimit()
		{
			time_t now;
			time(&now);
			return (int)(now - m_createTime);
		}

		//获取数据包序号
		UINT32 getSeqNum()
		{
			PacketHeader* m_pPacketHeader = (PacketHeader* )m_packet;
			return m_pPacketHeader->SeqNum;
		}

		//获取数据包指针
		char* getPacketPointer() { return m_packet; }

		//获取数据包大小
		int getPacketSize() { return m_packetSize; }

	private:

		char m_packet[MAXPACKETSIZE];

		int m_packetSize;

		bool m_RtsSent;

		time_t m_createTime;
	};
}
