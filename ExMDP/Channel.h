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

		///���ݴ���
		void ProcessData(PER_SOCKET_DATA *pCompKey, PER_IO_DATA *pOverlapped);
		//����ʵʱ�����
		void processIncrementalPacket(Packet& packet, int socket);
		//�����Լ���������
		void processInstrumentDefPacket(Packet& packet, int socket);
		//������������
		void processMarketRecoveryPacket(Packet& packet, int socket);

		//��Ч���������
		void PushPacket( char* str, int size );

		//����ʵʱ���������Ź���
		void spoolIncrementalPacket( Packet& packet );

		//���ͺ�Լ���������֮ǰ������Ҫ��ֹͣ��������Ϣ�����ﵽTag 911-TotNumReports
		void onPushInstrumentDefPacket( Packet& packet, const int sock );
		//ͬ���ģ����յ�ֹͣ��������Ϣ�����ﵽTag 911-TotNumReports������Ҫ��ȡtag 369-LastMsgSeqNumProcessed��+1����Ϊʵʱ�������͵Ŀ�ʼ���
		void onPushMarketRecoveryPacket( Packet& packet, const int sock );

		//���ʵʱ�����еĻ�����Ƿ��������
		void checkIncrementalSpoolTimer(const int sock);

		//���ʵʱ���������
		void clearIncrementalSpool();

		//�����鲥
		//����Incremental Feed
		void subscribeIncremental();
		//����Market Recovery Feed
		void subscribeMarketRecovery();
		//����Instrument Replay Feed
		void subscribeInstrumentDef();
		//�ر�socket
		void unsubscribe(const int socket);
		void unsubscribeAll();

		//�������������
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

		//�Ƿ��Ѿ�����Instrument Definition
		bool m_bOnInstrumentDef;

		//�Ƿ��Ѿ�����Market Recovery
		bool m_bOnMarketRecovery;

		//���ջָ���ɱ�־
		//bool m_SnapShotComplete;

		//��Լ����ָ���ɱ�־
		bool m_InstDefComplete;

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

		//refresh sever��ʹ���������������ʱ��
		const int m_poolTimeLimit;

		//���������
		typedef std::map< unsigned int, SpoolPacket > MAPSpoolPacket;
		MAPSpoolPacket m_mapSpoolPacket;

		//����channel�е���Ϣ����������̰߳�ȫ��
		CRITICAL_SECTION m_csChannel;

		std::ofstream m_fPackets;
		std::ofstream m_fChannel;
	};

}