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
		std::string multiaddr;//组播地址
		std::string interface;//本地网卡
	}SocketInfo;

	typedef std::map < int, SocketInfo*> MapSocketToSocketInfo;

	typedef std::set < int > Sockets;

	typedef std::queue < int > Queue;

	class Initiator
	{
	public:
		Initiator();
		~Initiator();

		//引擎启动
		int Start( ConfigStruct* configStruct, Application* application );

		//引擎停止
		int Stop();

		//接入合约定义组播
		bool SubscribeInstrumentDefinition( const std::string& );

		//接入实时行情组播
		bool SubscribeIncremental( const std::string& );

		//接入市场快照组播
		bool SubscribeMarketRecovery( const std::string& );

		//断开UDP组播
		bool Unsubscribe( const int sock );

		//SBE解析模板文件,非线程安全
		//接收线程使用
		IrRepo m_irRepo;

		//处理线程使用
		IrRepo m_irRepoX;

		//数据包入队列
		void PushPacket(Packet& packet)
		{
			EnterCriticalSection(&m_csData);
			//测试队列积压
			//m_fLog << "[PushPacket]: size:" << m_packetQueue.size() << std::endl;
			m_packetQueue.push(packet);
			SetEvent(m_hEventData);
			LeaveCriticalSection(&m_csData);
		}

	private:
		//引擎日志文件 debug模式下可包含效行情解析日志
		std::ofstream m_fLog;
		//日志函数
		void WriteLog(char* szFormat, ...);

		//Channel 配置文件
		std::string m_configFile;

		//本机ip地址
		std::string m_localInterface;

		//SBE解码模板
		std::string m_templateFile;

		//应用回调类
		Application* m_application;

		//根据ChannelID查找连接信息: connection id -> connection info.
		MAPConnectionIDtoInfo m_mapConnID2Info;

		//ChannelID集
		SETChannelIDs m_setChannelIDs;

		//根据ChannelID查找Channel：ChannelID -> Channel*
		MAPChannels m_mapChannels;

		//禁止重复启动
		BOOL m_bStopped;

		//引擎关闭事件
		HANDLE m_hEventStop;

		//行情数据包指示
		HANDLE m_hEventData;

		//行情数据读写锁
		CRITICAL_SECTION m_csData;

		//创建的线程句柄
		HANDLE m_hThreads[2];

		//启动接收数据线程
		static unsigned int _stdcall receiverThread( void* p );
		void ReceiveThread();

		//socket连接
		int UDPconnect( const std::string& address, const int port, const std::string& localInterface);

		//socket断开
		void UDPdisconnect(int socket, const std::string& address, const std::string& localInterface);

		//根据Socket查找对应Channel和数据类型
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

		//启动处理数据线程
		static unsigned int _stdcall processorThread( void* p );
		void ProcessThread();

		//有效行情包队列
		std::queue<Packet> m_packetQueue;

		///取最早入队列的数据包
		/**
		*@param  Packet& packet		数据包引用
		*@return int 0 成功 -1 失败
		*/
		int FrontPacket(Packet& packet)
		{
			//Locker l(m_mutex);
			EnterCriticalSection(&m_csData);
			//测试队列积压
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

		//删除最早入队列的数据包
		void PopPacket()
		{
			EnterCriticalSection(&m_csData);
			if (!m_packetQueue.empty())
				m_packetQueue.pop();
			if (m_packetQueue.empty())
				ResetEvent(m_hEventData);
			LeaveCriticalSection(&m_csData);
		}

//--------------重发服务-----------------
		//会话
		//Session* m_session;
		//连接状态
		//bool m_onRetransmission;
		//用户名
		//std::string m_username;
		//密码
		//std::string m_password;
		//上一次接收数据超时
		//time_t m_lastTimeOut;
		//重连间隔
		//int m_reconnectInterval;
		//重连次数
		//int m_connectTimes;
		//重发请求
		//bool GenerateRetransRequest(ChannelID , unsigned, unsigned);
	};

}

