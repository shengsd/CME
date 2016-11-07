#pragma once
#include <time.h>
#define MAXPACKETSIZE 4096

namespace MDP
{
	typedef struct XdpPacketHeader
	{
		UINT32 mSeqNum;
		UINT64 mSendTime;
	}PacketHeader;

	class Packet
	{
	public:
		Packet();
		//Packet( const char* str, ssize_t len );

		~Packet();

		void setPacketSize( int nSize ) { m_packetSize = nSize; } 

		//�������ݰ�ָ��
		//		void setPacket( const char* str );
		//		void setPacket( const char* str, ssize_t len );

		//�Ƿ��Ѿ��������ط�
		bool isRetransRequested() { return m_RtsSent; }

		//�ط�����
		void setRetransRequested( bool value ) { m_RtsSent = value; }

		//��ȡ���ݰ����
		UINT32 getSeqNum();

		//��ȡ���ݰ�ָ��
		char* getPacketPointer() { return m_szPacket; }

		//��ȡ���ݰ���С
		int getPacketSize() { return m_packetSize; }

		//���ص�ǰʱ���봴��ʱ��Ĳ�ֵ����λ���룩
		int getTimeLimit();

	private:
		char m_szPacket[MAXPACKETSIZE];

		int m_packetSize;

		bool m_RtsSent;

		time_t m_createTime;
	};
}
