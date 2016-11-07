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

		//��sock��ȡ����connectTypeΪ������
		bool read( int sock, int connectType );

		//��Ч���������
		void PushPacket( Packet& packet);

		//����ʵʱ�����
		void processIncrementalPacket( Packet& packet);
		//�����Լ���������
		void processInstrumentDefPacket( Packet& packet, int sock);
		//������������
		void processMarketRecoveryPacket( Packet& packet, int sock);

		//����ʵʱ���������Ź���
		void spoolIncrementalPacket( Packet& packet );
		//�����Լ���������
		void spoolInstrumentDefPacket( Packet& packet );
		//������������
		void spoolMarketRecoveryPacket( Packet& packet );

		//���ͺ�Լ���������֮ǰ������Ҫ��ֹͣ��������Ϣ�����ﵽTag 911-TotNumReports
		void onPushInstrumentDefPacket( Packet& packet, int sock );
		//ͬ���ģ����յ�ֹͣ��������Ϣ�����ﵽTag 911-TotNumReports������Ҫ��ȡtag 369-LastMsgSeqNumProcessed��+1����Ϊʵʱ�������͵Ŀ�ʼ���
		void onPushMarketRecoveryPacket( Packet& packet, int sock );

		//���ʵʱ�����еĻ�����Ƿ��������
		void checkIncrementalSpoolTimer();
		//����Լ�����еĻ����
		void checkInstrumentDefSpoolTimer(int sock);
		//�������еĻ����
		void checkMarketRecoverySpoolTimer(int sock);

		//���ʵʱ���������
		void clearIncrementalSpool();
		//��պ�Լ�������������
		void clearInstrumentDefSpool();
		//��տ������������
		void clearMarketRecoverySpool();

		//�����鲥
		//����Incremental Feed
		void subscribeIncremental();
		//����Market Recovery Feed
		void subscribeMarketRecovery();
		//����Instrument Replay Feed
		void subscribeInstrumentDef();
		//�˳��鲥
		void unsubscribe(int socket);

		//Incremental״̬����
		//void setOnIncremental( bool value ) { m_bOnIncremental = value; }
		//bool isOnIncremental() { return m_bOnIncremental; }

		//Instrument Definition����״̬����
		void setOnInstrumentDef( bool value ) { m_bOnInstrumentDef = value; }
		bool isOnInstrumentDef() { return m_bOnInstrumentDef; }

		//��Լ�����ȡ״̬����
		//void setInstDefComplete( bool value ) { m_InstDefComplete = value; }
		//bool isInsDefComplete() { return m_InstDefComplete; }

		//Market Recovery����״̬����
		void setOnMarketRecovery( bool value ) { m_bOnMarketRecovery = value; }
		bool isOnMarketRecovery() { return m_bOnMarketRecovery; }


		//�������������
		void increaseIncrementalNextSeqNum() { ++m_IncrementalNextSeqNum; }
		void increaseMarketRecoveryNextSeqNum() { ++m_MarketRecoveryNextSeqNum; }
		void increaseInstrumentDefNextSeqNum() { ++m_InstrumentDefNextSeqNum; }

//		void resetIncremental();
		void resetMarketRecovery();
		void resetInstrumentDef();

		void onEvent( const std::string& );

		void onData( const char* , int );

		//��Ч���������
		//std::queue<Packet> m_packetQueue;

	private:
		///Application& m_application;
		
		std::string m_ChannelID;

//		typedef std::queue<Packet> PacketQueue;

		//Incremental��־
		bool m_bOnIncremental;

		//Instrument Definition�����־
		bool m_bOnInstrumentDef;

		//Market Recovery�����־
		bool m_bOnMarketRecovery;

		//���ջָ���ɱ�־
		//bool m_SnapShotComplete;

		//��Լ����ָ���ɱ�־
		//bool m_InstDefComplete;

		//��Լ����ͨ�����
		unsigned m_InstrumentDefNextSeqNum;
		//����ͨ�����
		unsigned m_MarketRecoveryNextSeqNum;
		//ʵʱ����ͨ�����
		unsigned m_IncrementalNextSeqNum;

		//���һ��������Ϣ�е�The tag 369-LastMsgSeqNumProcessed
		//ָʾʵʱ����ͨ�����
		unsigned m_LastMsgSeqNumProcessed;

		//��Լ����ͨ��������Ϣ����
		unsigned m_InstrumentDefProcessedNum;
		//����ͨ��������Ϣ����
		unsigned m_MarketRecoveryProcessedNum;

		//refresh sever��ʹ���������������ʱ������
		const int m_poolTimeLimit;

		//���������
		typedef std::map< unsigned int, Packet > PacketSpool;
		PacketSpool m_IncrementalPacketSpool;
		PacketSpool m_InstrumentDefPacketSpool;
		PacketSpool m_MarketRecoveryPacketSpool;

		//��socket��ȡ������ֱ�Ӵ浽Packet��
		//char m_buffer[2048];
		//size_t m_len;

		std::ofstream m_fPackets;
		std::ofstream m_fEvents;
	};

}