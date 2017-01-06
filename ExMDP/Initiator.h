#pragma once

#include <queue>
#include "ExMDP.h"
#include "SBEdecoder.h"
#include "Channel.h"

namespace MDP
{
#define INCREMENTAL_A 0
#define INCREMENTAL_B 1
#define INSTRUMENT_RPLAY_A 2
#define INSTRUMENT_RPLAY_B 3
#define SNAPSHOT_A 4
#define SNAPSHOT_B 5
#define HISTORICAL_REPLAY 6

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

	class Initiator
	{
	public:

		Initiator();

		~Initiator();

		//��������
		int Start( ConfigStruct* configStruct, Application* application );

		//����ֹͣ
		void Stop();

		//�����鲥
		BOOL Subscribe(Channel* pChannel, int nDataType);

		//���Ͱ�
		void PushPacket( char* str, int size );

		//SBE����ģ���ļ�,���̰߳�ȫ
		IrRepo m_irRepo;

		CRITICAL_SECTION m_csirRepo;

	private:

		//�����߳�
		static unsigned int _stdcall WorkThreadStartAddr( void* p );
		void WorkThread();

		HANDLE* m_pThreadHandles;

		//�����鲥
		SOCKET UDPJoin( const std::string& address, const int port, const std::string& localInterface);//TODO:localInterface?

		//������־����
		void WriteLog(char* szFormat, ...);

		//��ɶ˿�
		HANDLE m_hIOCP;

		//������־�ļ� debugģʽ�¿ɰ���Ч���������־
		std::ofstream m_fMDPLog;

		//д��־��
		CRITICAL_SECTION m_csLog;

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

