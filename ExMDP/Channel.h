#pragma once
#include "Packet.h"
#include "Initiator.h"

namespace MDP
{
	typedef std::set < SOCKET > Sockets;
	typedef struct _PER_SOCKET_DATA
	{
		SOCKET  s;
		int nDataType;
		void *pChannel;
	}PER_SOCKET_DATA;

#define BUFFER_SIZE 4096
	typedef struct _PER_IO_DATA
	{
		OVERLAPPED  overlapped;
		WSABUF dataBuf;
		CHAR   buffer[BUFFER_SIZE];
		DWORD recvBytes;
		DWORD sendBytes;
		DWORD flags;
		SOCKADDR  fromAddr;
		int fromAddrLen;
	}PER_IO_DATA;

	typedef int ChannelID;
	class Initiator;
	class Channel
	{
	public:
		Channel( const std::string&, Initiator* );
		~Channel();

		///数据处理
		void ProcessData(PER_SOCKET_DATA *pCompKey, PER_IO_DATA *pOverlapped);
		//处理实时行情包
		void processIncrementalPacket(Packet& packet, int socket);
		//处理合约定义行情包
		void processInstrumentDefPacket(Packet& packet, int socket);
		//处理快照行情包
		void processMarketRecoveryPacket(Packet& packet, int socket);

		//有效行情包推送
		void PushPacket( char* str, int size );

		//缓存实时行情包（序号过大）
		void spoolIncrementalPacket( Packet& packet );

		//推送合约定义行情包之前，还需要找停止条件：消息数量达到Tag 911-TotNumReports
		void onPushInstrumentDefPacket( Packet& packet, const int sock );
		//同样的，快照的停止条件：消息数量达到Tag 911-TotNumReports，还需要获取tag 369-LastMsgSeqNumProcessed，+1后作为实时行情推送的开始序号
		void onPushMarketRecoveryPacket( Packet& packet, const int sock );

		//检查实时行情中的缓存包是否可以推送
		void checkIncrementalSpoolTimer(const int sock);

		//清空实时行情包缓存
		void clearIncrementalSpool();

		//订阅组播
		//订阅Incremental Feed
		void subscribeIncremental();
		//订阅Market Recovery Feed
		void subscribeMarketRecovery();
		//订阅Instrument Replay Feed
		void subscribeInstrumentDef();
		//关闭socket
		void unsubscribe(const int socket);
		void unsubscribeAll();

		//包处理序号自增
		void increaseIncrementalNextSeqNum() { ++m_IncrementalNextSeqNum; }
		void increaseMarketRecoveryNextSeqNum() { ++m_MarketRecoveryNextSeqNum; }
		void increaseInstrumentDefNextSeqNum() { ++m_InstrumentDefNextSeqNum; }

		void resetMarketRecovery();
		void resetInstrumentDef();

		Initiator* m_initiator;

		std::string m_ChannelID;

		PER_SOCKET_DATA m_CompKey[6];

		PER_IO_DATA m_Overlapped[6];

		Sockets m_setSockets;
	private:

		//是否已经订阅Instrument Definition
		bool m_bOnInstrumentDef;

		//是否已经订阅Market Recovery
		bool m_bOnMarketRecovery;

		//快照恢复完成标志
		//bool m_SnapShotComplete;

		//合约定义恢复完成标志
		bool m_InstDefComplete;

		//合约定义通道序号
		unsigned m_InstrumentDefNextSeqNum;
		//快照通道序号
		unsigned m_MarketRecoveryNextSeqNum;
		//实时行情通道序号
		unsigned m_IncrementalNextSeqNum;

		//最后一条快照消息中的The tag 369-LastMsgSeqNumProcessed
		//指示实时行情通道序号
		unsigned m_LastMsgSeqNumProcessed;

		//合约定义通道处理消息数量
		unsigned m_InstrumentDefProcessedNum;
		//快照通道处理消息数量
		unsigned m_MarketRecoveryProcessedNum;

		//refresh sever的使用条件，超过这个时长
		const int m_poolTimeLimit;

		//缓存行情包
		typedef std::map< unsigned int, SpoolPacket > MAPSpoolPacket;
		MAPSpoolPacket m_mapSpoolPacket;

		//单个channel中的消息处理必须是线程安全的
		CRITICAL_SECTION m_csChannel;

		std::ofstream m_fPackets;
		std::ofstream m_fChannel;
	};

}