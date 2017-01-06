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

		//引擎启动
		int Start( ConfigStruct* configStruct, Application* application );

		//引擎停止
		void Stop();

		//接入组播
		BOOL Subscribe(Channel* pChannel, int nDataType);

		//推送包
		void PushPacket( char* str, int size );

		//SBE解析模板文件,非线程安全
		IrRepo m_irRepo;

		CRITICAL_SECTION m_csirRepo;

	private:

		//工作线程
		static unsigned int _stdcall WorkThreadStartAddr( void* p );
		void WorkThread();

		HANDLE* m_pThreadHandles;

		//加入组播
		SOCKET UDPJoin( const std::string& address, const int port, const std::string& localInterface);//TODO:localInterface?

		//引擎日志函数
		void WriteLog(char* szFormat, ...);

		//完成端口
		HANDLE m_hIOCP;

		//引擎日志文件 debug模式下可包含效行情解析日志
		std::ofstream m_fMDPLog;

		//写日志锁
		CRITICAL_SECTION m_csLog;

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

