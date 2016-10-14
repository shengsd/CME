#pragma once
#include "Packet.h"
#include "Initiator.h"
#include <fstream>
#include "Mutex.h"

namespace MDP
{
	class Initiator;
	typedef int ChannelID;
//	class Packet;
	
	class Channel
	{
	public:
		Channel( const ChannelID&, Initiator* );
		
		~Channel();

		//从sock读取包，connectType为包类型
		bool read( int sock, int connectType );

		//有效行情包推送
		void PushPacket( Packet& packet);
		//void pushPacket( Packet& packet, bool copy );
		//清空有效行情队列
		//void clearPacketQueue();

		//处理实时行情包
		void processRealTimePacket( Packet& packet);
		//处理合约定义行情包
		void processInstDefPacket( Packet& packet, int sock);
		//处理快照行情包
		void processSnapShotPacket( Packet& packet, int sock);

		//缓存实时行情包（序号过大）
		void spoolRealTimePacket( Packet& packet );
		//缓存合约定义行情包
		void spoolInstDefPacket( Packet& packet );
		//缓存快照行情包
		void spoolSnapShotPacket( Packet& packet );

		//推送合约定义行情包之前，还需要找停止条件：消息数量达到Tag 911-TotNumReports
		void onPushInstDefPacket( Packet& packet, int sock );
		//同样的，快照的停止条件：消息数量达到Tag 911-TotNumReports，还需要获取tag 369-LastMsgSeqNumProcessed，+1后作为实时行情推送的开始序号
		void onPushSnapShotPacket( Packet& packet, int sock );

		//检查实时行情中的缓存包是否可以推送
		void checkRealTimeSpoolTimer();
		//检查合约定义中的缓存包
		void checkInstDefSpoolTimer(int sock);
		//检查快照中的缓存包
		void checkSnapShotSpoolTimer(int sock);

		//清空实时行情包缓存
		void clearRealTimeSpool();
		//清空合约定义行情包缓存
		void clearInstDefSpool();
		//清空快照行情包缓存
		void clearSnapShotSpool();

		//订阅组播
		//订阅Incremental Feed
		void subscribeIncremental();
		//订阅Recovery服务
		void subscribeMarketRecovery();
		//订阅Instrument Replay Feed
		void subscribeInstrumentDefinition();
		//退出组播
		void unsubscribe(int socket);

		//Incremental状态设置
		//void setOnIncremental( bool value ) { m_bOnIncremental = value; }
		//bool isOnIncremental() { return m_bOnIncremental; }

		//Instrument Definition服务状态设置
		void setOnInstrumentDefinition( bool value ) { m_bOnInstrumentDefinition = value; }
		bool isOnInstrumentDefinition() { return m_bOnInstrumentDefinition; }

		//合约定义获取状态设置
		//void setInstDefComplete( bool value ) { m_InstDefComplete = value; }
		//bool isInsDefComplete() { return m_InstDefComplete; }

		//Market Recovery服务状态设置
		void setOnMarketRecovery( bool value ) { m_bOnMarketRecovery = value; }
		bool isOnMarketRecovery() { return m_bOnMarketRecovery; }


		//包处理序号自增
		void increaseRealTimeNextSeqNum() { ++m_RealTimeNextSeqNum; }
		void increaseSnapShotNextSeqNum() { ++m_SnapShotNextSeqNum; }
		void increaseInstDefNextSeqNum() { ++m_InstDefNextSeqNum; }

//		void resetIncremental();
		void resetMarketRecovery();
		void resetInstrumentDefinition();

		void onEvent( const std::string& );

		void onData( const char* , int );

		ChannelID getChannelID() { return m_ChannelID; }

		//有效行情包队列
		//std::queue<Packet> m_packetQueue;

	private:
		///Application& m_application;
		
		ChannelID m_ChannelID;

//		typedef std::queue<Packet> PacketQueue;

		//Incremental标志
		bool m_bOnIncremental;

		//Instrument Definition服务标志
		bool m_bOnInstrumentDefinition;

		//Market Recovery服务标志
		bool m_bOnMarketRecovery;

		//快照恢复完成标志
		//bool m_SnapShotComplete;

		//合约定义恢复完成标志
		//bool m_InstDefComplete;

		//合约定义通道序号
		unsigned m_InstDefNextSeqNum;
		//快照通道序号
		unsigned m_SnapShotNextSeqNum;
		//实时行情通道序号
		unsigned m_RealTimeNextSeqNum;

		//最后一条快照消息中的The tag 369-LastMsgSeqNumProcessed
		//指示实时行情通道序号
		unsigned m_LastMsgSeqNumProcessed;

		//合约定义通道处理消息数量
		unsigned m_InstDefProcessedNum;
		//快照通道处理消息数量
		unsigned m_SnapShotProcessedNum;

		//refresh sever的使用条件，超过这个时长（）
		const int m_poolTimeLimit;

		//缓存行情包
		typedef std::map< unsigned int, Packet > PacketSpool;
		PacketSpool m_RealTimePacketSpool;
		PacketSpool m_InstDefPacketSpool;
		PacketSpool m_SnapShotPacketSpool;

		Initiator* m_initiator;

		//从socket读取的数据直接存到Packet中
		char m_buffer[2048];
		size_t m_len;

		std::ofstream m_fPackets;
		std::ofstream m_fEvents;

		std::string m_packetsFileName;
		std::string m_eventsFileName;

		Mutex m_mutex;
	};

}