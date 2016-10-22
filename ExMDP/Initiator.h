#pragma once

#include <queue>
#include "ExMDP.h"
#include "SBEdecoder.h"
#include "Channel.h"

namespace MDP
{
	class Channel;
	typedef std::set< std::string > SETChannelIDs;

	typedef struct __ConnectionInfo
	{
		std::string ip;
		std::string hostip;
		std::string port;
	}ConnectionInfo;

	typedef std::map< std::string, ConnectionInfo > MAPConnectionIDtoInfo;

	typedef std::map< std::string, Channel* > MAPChannels;

	typedef struct __SocketInfo
	{
		Channel* pChannel;
		int iConnectionType;//1:Incremental Feed	2:Instrument Recovery	3:Market Recovery  4:Retransmission
		std::string multiaddr;//�鲥��ַ
		std::string interface;//��������
	}SocketInfo;

	typedef std::map < int, SocketInfo*> MapSocketToSocketInfo;

	typedef std::set < int > Sockets;

	typedef std::queue < int > Queue;

	class Initiator
	{
	public:
		Initiator();
		~Initiator();

		//��������
		int Start( ConfigStruct* configStruct, Application* application );

		//����ֹͣ
		int Stop();

		//�����Լ�����鲥
		bool SubscribeInstrumentDefinition( const std::string& );

		//����ʵʱ�����鲥
		bool SubscribeIncremental( const std::string& );

		//�����г������鲥
		bool SubscribeMarketRecovery( const std::string& );

		//�Ͽ�UDP�鲥
		bool Unsubscribe( const int sock );

		//SBE����ģ���ļ�,���̰߳�ȫ
		//�����߳�ʹ��
		IrRepo m_irRepo;

		//�����߳�ʹ��
		IrRepo m_irRepoX;

		//���ݰ������
		void PushPacket(Packet& packet)
		{
			EnterCriticalSection(&m_csData);
			//���Զ��л�ѹ
			//m_fLog << "[PushPacket]: size:" << m_packetQueue.size() << std::endl;
			m_packetQueue.push(packet);
			SetEvent(m_hEventData);
			LeaveCriticalSection(&m_csData);
		}

	private:
		//������־�ļ� debugģʽ�¿ɰ���Ч���������־
		std::ofstream m_fLog;
		//��־����
		void WriteLog(char* szFormat, ...);

		//Channel �����ļ�
		std::string m_configFile;

		//����ip��ַ
		std::string m_localInterface;

		//SBE����ģ��
		std::string m_templateFile;

		//Ӧ�ûص���
		Application* m_application;

		//����ChannelID����������Ϣ: connection id -> connection info.
		MAPConnectionIDtoInfo m_mapConnID2Info;

		//ChannelID��
		SETChannelIDs m_setChannelIDs;

		//����ChannelID����Channel��ChannelID -> Channel*
		MAPChannels m_mapChannels;

		//��ֹ�ظ�����
		BOOL m_bStopped;

		//����ر��¼�
		HANDLE m_hEventStop;

		//�������ݰ�ָʾ
		HANDLE m_hEventData;

		//�������ݶ�д��
		CRITICAL_SECTION m_csData;

		//�������߳̾��
		HANDLE m_hThreads[2];

		//�������������߳�
		static unsigned int _stdcall receiverThread( void* p );
		void ReceiveThread();

		//socket����
		int UDPconnect( const std::string& address, const int port, const std::string& localInterface);

		//socket�Ͽ�
		void UDPdisconnect(int socket, const std::string& address, const std::string& localInterface);

		//����Socket���Ҷ�ӦChannel����������
		MapSocketToSocketInfo m_mapSocket2Info;
		
		void Block( bool poll = 0, long timeout = 0 );
		bool addRead( int socket );
		bool drop( int socket );
		void ProcessReadSet( fd_set& );
		void BuildSet( const Sockets&, fd_set& );
		Sockets m_readSockets;
		timeval m_timeval;

		bool onData( int );

		void onError( int );
		void onError();

		void onTimeout();

		//�������������߳�
		static unsigned int _stdcall processorThread( void* p );
		void ProcessThread();

		//��Ч���������
		std::queue<Packet> m_packetQueue;

		///ȡ��������е����ݰ�
		/**
		*@param  Packet& packet		���ݰ�����
		*@return int 0 �ɹ� -1 ʧ��
		*/
		int FrontPacket(Packet& packet)
		{
			//Locker l(m_mutex);
			EnterCriticalSection(&m_csData);
			//���Զ��л�ѹ
			//m_fLog << "[FrontPacket]: size:" << m_packetQueue.size() << std::endl;
			if (!m_packetQueue.empty())
			{
				packet = m_packetQueue.front();
				LeaveCriticalSection(&m_csData);
				return 0;
			}
			LeaveCriticalSection(&m_csData);
			return -1;
		}

		//ɾ����������е����ݰ�
		void PopPacket()
		{
			EnterCriticalSection(&m_csData);
			if (!m_packetQueue.empty())
				m_packetQueue.pop();
			if (m_packetQueue.empty())
				ResetEvent(m_hEventData);
			LeaveCriticalSection(&m_csData);
		}

//--------------�ط�����-----------------
		//�Ự
		//Session* m_session;
		//����״̬
		//bool m_onRetransmission;
		//�û���
		//std::string m_username;
		//����
		//std::string m_password;
		//��һ�ν������ݳ�ʱ
		//time_t m_lastTimeOut;
		//�������
		//int m_reconnectInterval;
		//��������
		//int m_connectTimes;
		//�ط�����
		//bool GenerateRetransRequest(ChannelID , unsigned, unsigned);
	};

}

