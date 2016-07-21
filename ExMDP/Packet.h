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

		//重置数据包指针
		//		void setPacket( const char* str );
		//		void setPacket( const char* str, ssize_t len );

		//是否已经请求了重发
		bool isRetransRequested() { return m_RtsSent; }

		//重发设置
		void setRetransRequested( bool value ) { m_RtsSent = value; }

		//获取数据包序号
		UINT32 getSeqNum();

		//获取数据包指针
		char* getPacketPointer() { return m_szPacket; }

		//获取数据包大小
		int getPacketSize() { return m_packetSize; }

		//返回当前时间与创建时间的差值（单位：秒）
		int getTimeLimit();

		//		bool containsSeqNum(unsigned int sn);

		//		void extractMessage( Message& msg, unsigned int msgProcessCount );

	private:
		char m_szPacket[MAXPACKETSIZE];

		//PacketHeader* m_pPacketHeader;

		int m_packetSize;

		bool m_RtsSent;

		time_t m_createTime;
	};
}
