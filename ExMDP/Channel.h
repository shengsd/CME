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

		//��sock��ȡ����connectTypeΪ������
		bool read( int sock, int connectType );

		//��Ч���������
		void PushPacket( Packet& packet);
		//void pushPacket( Packet& packet, bool copy );
		//�����Ч�������
		//void clearPacketQueue();

		//����ʵʱ�����
		void processRealTimePacket( Packet& packet);
		//�����Լ���������
		void processInstDefPacket( Packet& packet, int sock);
		//������������
		void processSnapShotPacket( Packet& packet, int sock);

		//����ʵʱ���������Ź���
		void spoolRealTimePacket( Packet& packet );
		//�����Լ���������
		void spoolInstDefPacket( Packet& packet );
		//������������
		void spoolSnapShotPacket( Packet& packet );

		//���ͺ�Լ���������֮ǰ������Ҫ��ֹͣ��������Ϣ�����ﵽTag 911-TotNumReports
		void onPushInstDefPacket( Packet& packet, int sock );
		//ͬ���ģ����յ�ֹͣ��������Ϣ�����ﵽTag 911-TotNumReports������Ҫ��ȡtag 369-LastMsgSeqNumProcessed��+1����Ϊʵʱ�������͵Ŀ�ʼ���
		void onPushSnapShotPacket( Packet& packet, int sock );

		//���ʵʱ�����еĻ�����Ƿ��������
		void checkRealTimeSpoolTimer();
		//����Լ�����еĻ����
		void checkInstDefSpoolTimer(int sock);
		//�������еĻ����
		void checkSnapShotSpoolTimer(int sock);

		//���ʵʱ���������
		void clearRealTimeSpool();
		//��պ�Լ�������������
		void clearInstDefSpool();
		//��տ������������
		void clearSnapShotSpool();

		//�����鲥
		//����Incremental Feed
		void subscribeIncremental();
		//����Recovery����
		void subscribeMarketRecovery();
		//����Instrument Replay Feed
		void subscribeInstrumentDefinition();
		//�˳��鲥
		void unsubscribe(int socket);

		//Incremental״̬����
		//void setOnIncremental( bool value ) { m_bOnIncremental = value; }
		//bool isOnIncremental() { return m_bOnIncremental; }

		//Instrument Definition����״̬����
		void setOnInstrumentDefinition( bool value ) { m_bOnInstrumentDefinition = value; }
		bool isOnInstrumentDefinition() { return m_bOnInstrumentDefinition; }

		//��Լ�����ȡ״̬����
		//void setInstDefComplete( bool value ) { m_InstDefComplete = value; }
		//bool isInsDefComplete() { return m_InstDefComplete; }

		//Market Recovery����״̬����
		void setOnMarketRecovery( bool value ) { m_bOnMarketRecovery = value; }
		bool isOnMarketRecovery() { return m_bOnMarketRecovery; }


		//�������������
		void increaseRealTimeNextSeqNum() { ++m_RealTimeNextSeqNum; }
		void increaseSnapShotNextSeqNum() { ++m_SnapShotNextSeqNum; }
		void increaseInstDefNextSeqNum() { ++m_InstDefNextSeqNum; }

//		void resetIncremental();
		void resetMarketRecovery();
		void resetInstrumentDefinition();

		void onEvent( const std::string& );

		void onData( const char* , int );

		ChannelID getChannelID() { return m_ChannelID; }

		//��Ч���������
		//std::queue<Packet> m_packetQueue;

	private:
		///Application& m_application;
		
		ChannelID m_ChannelID;

//		typedef std::queue<Packet> PacketQueue;

		//Incremental��־
		bool m_bOnIncremental;

		//Instrument Definition�����־
		bool m_bOnInstrumentDefinition;

		//Market Recovery�����־
		bool m_bOnMarketRecovery;

		//���ջָ���ɱ�־
		//bool m_SnapShotComplete;

		//��Լ����ָ���ɱ�־
		//bool m_InstDefComplete;

		//��Լ����ͨ�����
		unsigned m_InstDefNextSeqNum;
		//����ͨ�����
		unsigned m_SnapShotNextSeqNum;
		//ʵʱ����ͨ�����
		unsigned m_RealTimeNextSeqNum;

		//���һ��������Ϣ�е�The tag 369-LastMsgSeqNumProcessed
		//ָʾʵʱ����ͨ�����
		unsigned m_LastMsgSeqNumProcessed;

		//��Լ����ͨ��������Ϣ����
		unsigned m_InstDefProcessedNum;
		//����ͨ��������Ϣ����
		unsigned m_SnapShotProcessedNum;

		//refresh sever��ʹ���������������ʱ������
		const int m_poolTimeLimit;

		//���������
		typedef std::map< unsigned int, Packet > PacketSpool;
		PacketSpool m_RealTimePacketSpool;
		PacketSpool m_InstDefPacketSpool;
		PacketSpool m_SnapShotPacketSpool;

		Initiator* m_initiator;

		//��socket��ȡ������ֱ�Ӵ浽Packet��
		char m_buffer[2048];
		size_t m_len;

		std::ofstream m_fPackets;
		std::ofstream m_fEvents;

		std::string m_packetsFileName;
		std::string m_eventsFileName;

		Mutex m_mutex;
	};

}