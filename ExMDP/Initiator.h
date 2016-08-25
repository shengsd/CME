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
		std::string multiaddr;//组播地址
		std::string interface;//本地网卡
	}SocketInfo;
	typedef std::map < int, SocketInfo*> MapSocketToSocketInfo;

	class Session;

	class Initiator : public Connector::Strategy
	{
	public:
		Initiator();

		~Initiator();

		//启动
		void start( ConfigStruct* configStruct, Application* application ) throw ( ConfigError, RuntimeError);

		//停止
		void stop();

		//接入合约定义组播
		bool doConnectToInstDef( const ChannelID );

		//接入实时行情组播
		bool doConnectToRealTime( const ChannelID );

		//接入市场快照组播
		bool doConnectToSnapShot( const ChannelID );

		//断开UDP组播
		bool disConnectMulticast( const int sock );

		//重发请求
		bool GenerateRetransRequest(ChannelID , unsigned, unsigned);

		//SBE解码
		//解析模板文件,允许Channel使用
		IrRepo m_irRepo;

		IrRepo m_irRepoX;

		//数据包入队列
		void PushPacket(Packet& packet)
		{
			//Locker l(m_mutex);
			EnterCriticalSection(&m_mutex);
			//测试队列积压
			//m_fLog << "[PushPacket]: size:" << m_packetQueue.size() << std::endl;
			m_packetQueue.push(packet);
			SetEvent(m_hEventData);
			LeaveCriticalSection(&m_mutex);
		}

		///取最早入队列的数据包
		/**
		*@param  Packet& packet		数据包引用
		*@return int 0 成功 -1 失败
		*/
		int FrontPacket(Packet& packet)
		{
			//Locker l(m_mutex);
			EnterCriticalSection(&m_mutex);
			//测试队列积压
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
		
		//删除最早入队列的数据包
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

		//有效行情包队列
		std::queue<Packet> m_packetQueue;
		

	private:
		//初始化，读取配置文件，创建Channel
		void initialize() throw (ConfigError);

		//接收数据线程运行中
		void onReceiverStart();

		//接收数据线程停止中
		//void onReceiverStop();

		void onProcessorStart();

		//void onProcessorStop();

		//发起连接，加入组播
		void connectMulticast();

		//运行状态
		//bool isStopped() { return m_stop; }

		
		void onConnect(Connector&, int);

		void onWrite(Connector&, int);
		
		bool onData(Connector&, int);
		
		void onDisconnect(Connector&, int);
		
		void onError(Connector&, int);
		
		void onError(Connector&);
		
		void onTimeout(Connector&);

		//控制日志输出，防止一直超时写日志
		BOOL m_bLog;

		//上一次接收数据超时
		time_t m_lastTimeOut;
		
		int m_reconnectInterval;

		int m_connectTimes;
		
		//Channel 配置文件
		std::string m_configFile;

		//本机ip地址
		std::string m_localInterface;

		//SBE解码模板
		std::string m_templateFile;

		//应用回调类
		Application* m_application;

		//socket连接管理类
		Connector m_connector;

		// map: connection id -> connection info.
		//	connection id = ChannelID + "H0A" or "IA" or "IB" or ...
		ConnectionIDtoInfo m_connectionIDtoInfo;

		//set: ChannelID
		ChannelIDs m_channelIDs;

		//map: ChannelID -> Channel*
		Channels m_channels;

		//map: Socket -> pChannel 和 connectionType
		MapSocketToSocketInfo m_socketToSocketInfo;

		//启动接收数据线程
		static THREAD_PROC receiverThread( void* p );

		//接收数据线程ID
		thread_id m_tReceiverID;

		//启动处理数据线程
		static THREAD_PROC processorThread( void* p );

		//引擎日志文件
		//std::ofstream m_fLog;
		//有效行情解析日志文件
		std::ofstream m_fDecoding;
		
		//处理数据线程ID
		thread_id m_tProcessorID;

		//TCP会话
		Session* m_session;
		
		//用户名
		std::string m_username;
		
		//密码
		std::string m_password;
		
		//重发服务连接状态
		bool m_onRetransmission;

		//引擎关闭事件
		//bool m_stop;
		HANDLE m_hEventStop;

		//行情数据包指示
		HANDLE m_hEventData;

		//锁
		//Mutex m_mutex;

		CRITICAL_SECTION m_mutex;
	};

}

