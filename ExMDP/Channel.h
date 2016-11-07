#pragma once
#include "Packet.h"
#include "Initiator.h"
namespace MDP
{
	typedef int ChannelID;
	class Initiator;
	class Channel
	{
	public:
		Channel( const std::string&, Initiator* );
		~Channel();

		Initiator* m_initiator;

		//从sock读取包，connectType为包类型
		bool read( int sock, int connectType );

		//有效行情包推送
		void PushPacket( Packet& packet);

		//处理实时行情包
		void processIncrementalPacket( Packet& packet);
		//处理合约定义行情包
		void processInstrumentDefPacket( Packet& packet, int sock);
		//处理快照行情包
		void processMarketRecoveryPacket( Packet& packet, int sock);

		//缓存实时行情包（序号过大）
		void spoolIncrementalPacket( Packet& packet );
		//缓存合约定义行情包
		void spoolInstrumentDefPacket( Packet& packet );
		//缓存快照行情包
		void spoolMarketRecoveryPacket( Packet& packet );

		//推送合约定义行情包之前，还需要找停止条件：消息数量达到Tag 911-TotNumReports
		void onPushInstrumentDefPacket( Packet& packet, int sock );
		//同样的，快照的停止条件：消息数量达到Tag 911-TotNumReports，还需要获取tag 369-LastMsgSeqNumProcessed，+1后作为实时行情推送的开始序号
		void onPushMarketRecoveryPacket( Packet& packet, int sock );

		//检查实时行情中的缓存包是否可以推送
		void checkIncrementalSpoolTimer();
		//检查合约定义中的缓存包
		void checkInstrumentDefSpoolTimer(int sock);
		//检查快照中的缓存包
		void checkMarketRecoverySpoolTimer(int sock);

		//清空实时行情包缓存
		void clearIncrementalSpool();
		//清空合约定义行情包缓存
		void clearInstrumentDefSpool();
		//清空快照行情包缓存
		void clearMarketRecoverySpool();

		//订阅组播
		//订阅Incremental Feed
		void subscribeIncremental();
		//订阅Market Recovery Feed
		void subscribeMarketRecovery();
		//订阅Instrument Replay Feed
		void subscribeInstrumentDef();
		//退出组播
		void unsubscribe(int socket);

		//Incremental状态设置
		//void setOnIncremental( bool value ) { m_bOnIncremental = value; }
		//bool isOnIncremental() { return m_bOnIncremental; }

		//Instrument Definition服务状态设置
		void setOnInstrumentDef( bool value ) { m_bOnInstrumentDef = value; }
		bool isOnInstrumentDef() { return m_bOnInstrumentDef; }

		//合约定义获取状态设置
		//void setInstDefComplete( bool value ) { m_InstDefComplete = value; }
		//bool isInsDefComplete() { return m_InstDefComplete; }

		//Market Recovery服务状态设置
		void setOnMarketRecovery( bool value ) { m_bOnMarketRecovery = value; }
		bool isOnMarketRecovery() { return m_bOnMarketRecovery; }


		//包处理序号自增
		void increaseIncrementalNextSeqNum() { ++m_IncrementalNextSeqNum; }
		void increaseMarketRecoveryNextSeqNum() { ++m_MarketRecoveryNextSeqNum; }
		void increaseInstrumentDefNextSeqNum() { ++m_InstrumentDefNextSeqNum; }

//		void resetIncremental();
		void resetMarketRecovery();
		void resetInstrumentDef();

		void onEvent( const std::string& );

		void onData( const char* , int );

		//有效行情包队列
		//std::queue<Packet> m_packetQueue;

	private:
		///Application& m_application;
		
		std::string m_ChannelID;

//		typedef std::queue<Packet> PacketQueue;

		//Incremental标志
		bool m_bOnIncremental;

		//Instrument Definition服务标志
		bool m_bOnInstrumentDef;

		//Market Recovery服务标志
		bool m_bOnMarketRecovery;

		//快照恢复完成标志
		//bool m_SnapShotComplete;

		//合约定义恢复完成标志
		//bool m_InstDefComplete;

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

		//refresh sever的使用条件，超过这个时长（）
		const int m_poolTimeLimit;

		//缓存行情包
		typedef std::map< unsigned int, Packet > PacketSpool;
		PacketSpool m_IncrementalPacketSpool;
		PacketSpool m_InstrumentDefPacketSpool;
		PacketSpool m_MarketRecoveryPacketSpool;

		//从socket读取的数据直接存到Packet中
		//char m_buffer[2048];
		//size_t m_len;

		std::ofstream m_fPackets;
		std::ofstream m_fEvents;
	};

}