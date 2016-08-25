#pragma once

#include "Exceptions.h"
#include "Connector.h"
#include "Session.h"
#include "ExMDP.h"
#include "SBEdecoder.h"

namespace MDP
{
	typedef int ChannelID;
	typedef std::set< ChannelID > ChannelIDs;
	typedef struct __ConnectionInfo
	{
		std::string ip;
		std::string hostip;
		std::string port;
	}ConnectionInfo;
	typedef std::map< std::string, ConnectionInfo > ConnectionIDtoInfo;

	class Channel;
	typedef std::map< ChannelID, Channel* > Channels;


	typedef struct __SocketInfo
	{
		Channel* pChannel;
		int iConnectionType;//1:Incremental Feed	2:Instrument Recovery	3:Market Recovery  4:Retransmission
		std::string multiaddr;//�鲥��ַ
		std::string interface;//��������
	}SocketInfo;
	typedef std::map < int, SocketInfo*> MapSocketToSocketInfo;

	class Session;

	class Initiator : public Connector::Strategy
	{
	public:
		Initiator();

		~Initiator();

		//����
		void start( ConfigStruct* configStruct, Application* application ) throw ( ConfigError, RuntimeError);

		//ֹͣ
		void stop();

		//�����Լ�����鲥
		bool doConnectToInstDef( const ChannelID );

		//����ʵʱ�����鲥
		bool doConnectToRealTime( const ChannelID );

		//�����г������鲥
		bool doConnectToSnapShot( const ChannelID );

		//�Ͽ�UDP�鲥
		bool disConnectMulticast( const int sock );

		//�ط�����
		bool GenerateRetransRequest(ChannelID , unsigned, unsigned);

		//SBE����
		//����ģ���ļ�,����Channelʹ��
		IrRepo m_irRepo;

		IrRepo m_irRepoX;

		//���ݰ������
		void PushPacket(Packet& packet)
		{
			//Locker l(m_mutex);
			EnterCriticalSection(&m_mutex);
			//���Զ��л�ѹ
			//m_fLog << "[PushPacket]: size:" << m_packetQueue.size() << std::endl;
			m_packetQueue.push(packet);
			SetEvent(m_hEventData);
			LeaveCriticalSection(&m_mutex);
		}

		///ȡ��������е����ݰ�
		/**
		*@param  Packet& packet		���ݰ�����
		*@return int 0 �ɹ� -1 ʧ��
		*/
		int FrontPacket(Packet& packet)
		{
			//Locker l(m_mutex);
			EnterCriticalSection(&m_mutex);
			//���Զ��л�ѹ
			//m_fLog << "[FrontPacket]: size:" << m_packetQueue.size() << std::endl;
			if (!m_packetQueue.empty())
			{
				packet = m_packetQueue.front();
				LeaveCriticalSection(&m_mutex);
				return 0;
			}
			LeaveCriticalSection(&m_mutex);
			return -1;
		}

		/*
		Packet& FrontPacket()
		{
			Locker l(m_mutex);
			return m_packetQueue.front();
		}
		*/
		
		//ɾ����������е����ݰ�
		void PopPacket()
		{
			//Locker l(m_mutex);
			EnterCriticalSection(&m_mutex);
			if (!m_packetQueue.empty())
				m_packetQueue.pop();
			if (m_packetQueue.empty())
				ResetEvent(m_hEventData);
			LeaveCriticalSection(&m_mutex);
		}

		//��Ч���������
		std::queue<Packet> m_packetQueue;
		

	private:
		//��ʼ������ȡ�����ļ�������Channel
		void initialize() throw (ConfigError);

		//���������߳�������
		void onReceiverStart();

		//���������߳�ֹͣ��
		//void onReceiverStop();

		void onProcessorStart();

		//void onProcessorStop();

		//�������ӣ������鲥
		void connectMulticast();

		//����״̬
		//bool isStopped() { return m_stop; }

		
		void onConnect(Connector&, int);

		void onWrite(Connector&, int);
		
		bool onData(Connector&, int);
		
		void onDisconnect(Connector&, int);
		
		void onError(Connector&, int);
		
		void onError(Connector&);
		
		void onTimeout(Connector&);

		//������־�������ֹһֱ��ʱд��־
		BOOL m_bLog;

		//��һ�ν������ݳ�ʱ
		time_t m_lastTimeOut;
		
		int m_reconnectInterval;

		int m_connectTimes;
		
		//Channel �����ļ�
		std::string m_configFile;

		//����ip��ַ
		std::string m_localInterface;

		//SBE����ģ��
		std::string m_templateFile;

		//Ӧ�ûص���
		Application* m_application;

		//socket���ӹ�����
		Connector m_connector;

		// map: connection id -> connection info.
		//	connection id = ChannelID + "H0A" or "IA" or "IB" or ...
		ConnectionIDtoInfo m_connectionIDtoInfo;

		//set: ChannelID
		ChannelIDs m_channelIDs;

		//map: ChannelID -> Channel*
		Channels m_channels;

		//map: Socket -> pChannel �� connectionType
		MapSocketToSocketInfo m_socketToSocketInfo;

		//�������������߳�
		static THREAD_PROC receiverThread( void* p );

		//���������߳�ID
		thread_id m_tReceiverID;

		//�������������߳�
		static THREAD_PROC processorThread( void* p );

		//������־�ļ�
		//std::ofstream m_fLog;
		//��Ч���������־�ļ�
		std::ofstream m_fDecoding;
		
		//���������߳�ID
		thread_id m_tProcessorID;

		//TCP�Ự
		Session* m_session;
		
		//�û���
		std::string m_username;
		
		//����
		std::string m_password;
		
		//�ط���������״̬
		bool m_onRetransmission;

		//����ر��¼�
		//bool m_stop;
		HANDLE m_hEventStop;

		//�������ݰ�ָʾ
		HANDLE m_hEventData;

		//��
		//Mutex m_mutex;

		CRITICAL_SECTION m_mutex;
	};

}

