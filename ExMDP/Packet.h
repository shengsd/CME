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

		//��ȡ���ݰ����
		UINT32 getSeqNum()
		{
			PacketHeader* m_pPacketHeader = (PacketHeader* )m_pPacket;
			return m_pPacketHeader->SeqNum;
		}

		//��ȡ���ݰ�ָ��
		char* getPacketPointer() { return m_pPacket; }

		//��ȡ���ݰ���С
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

		//�Ƿ��Ѿ��������ط�
		bool isRetransRequested() { return m_RtsSent; }

		//�ط�����
		void setRetransRequested( bool value ) { m_RtsSent = value; }

		//���ص�ǰʱ���봴��ʱ��Ĳ�ֵ����λ���룩
		int getTimeLimit()
		{
			time_t now;
			time(&now);
			return (int)(now - m_createTime);
		}

		//��ȡ���ݰ����
		UINT32 getSeqNum()
		{
			PacketHeader* m_pPacketHeader = (PacketHeader* )m_packet;
			return m_pPacketHeader->SeqNum;
		}

		//��ȡ���ݰ�ָ��
		char* getPacketPointer() { return m_packet; }

		//��ȡ���ݰ���С
		int getPacketSize() { return m_packetSize; }

	private:

		char m_packet[MAXPACKETSIZE];

		int m_packetSize;

		bool m_RtsSent;

		time_t m_createTime;
	};
}
