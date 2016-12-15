#include "stdafx.h"
#include "Initiator.h"
#include "TinyXML/tinystr.h"
#include "TinyXML/tinyxml.h"

namespace MDP
{
	Initiator::Initiator()
	{
		InitializeCriticalSection( &m_csData );
		InitializeCriticalSection( &m_csLog );
		m_hEventData = CreateEvent( NULL, TRUE, FALSE, NULL );
		m_hEventStop = CreateEvent( NULL, TRUE, FALSE, NULL );
		WORD version = MAKEWORD( 2, 2 );
		WSADATA data;
		WSAStartup( version, &data );
		m_timeval.tv_sec = 2;
		m_timeval.tv_usec = 0;
	}

	Initiator::~Initiator()
	{
		DeleteCriticalSection( &m_csData );
		DeleteCriticalSection( &m_csLog );
		CloseHandle(m_hEventStop);
		CloseHandle(m_hEventData);
		WSACleanup();
	}

	void Initiator::WriteLog(char* szFormat, ...)
	{
		char buffer[1024];
		SYSTEMTIME st;
		GetLocalTime(&st);
		sprintf(buffer, "%04d%02d%02d %02d:%02d:%02d:%03d - ", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
		va_list args;
		va_start (args, szFormat);
		vsprintf (buffer+strlen(buffer), szFormat, args);
		va_end (args);
		EnterCriticalSection(&m_csLog);
		m_fMDPLog << buffer << std::endl;
		LeaveCriticalSection(&m_csLog);
	}

	int Initiator::Start( ConfigStruct* configStruct, Application* application)
	{
		WriteLog("[Initiator::Start]: Called!");
		if (configStruct == NULL)
		{
			WriteLog("[Initiator::Start]: Invalid ConfigStruct*");
			return 1;
		}

		if (application == NULL)
		{
			strcpy(configStruct->errorInfo, "[Initiator::Start]: Invalid Application*");
			WriteLog("[Initiator::Start]: Invalid Application*");
			return 1;
		}

		m_configFile.assign(configStruct->configFile);
		m_localInterface.assign(configStruct->localInterface);
		m_templateFile.assign(configStruct->templateFile);
		//m_username.assign(configStruct->userName);
		//m_password.assign(configStruct->passWord);
		m_application = application;

		//打开（创建）日志文件
		char szPath[MAX_PATH] = {0};
		GetModuleFileName(NULL, szPath, MAX_PATH);
		char* pExeDir = strrchr(szPath, '\\');
		*pExeDir = '\0';
		sprintf(szPath, "%s\\Log", szPath);
		_mkdir(szPath);
		SYSTEMTIME st;
		GetSystemTime(&st);
		sprintf(szPath, "%s\\MDPEngine_%02d.log", szPath, st.wDay);
		m_fMDPLog.open( szPath, std::ofstream::app );
		if ( !m_fMDPLog.is_open() )
		{
			strcpy(configStruct->errorInfo, "[Initiator::Start]: Open MDPEngine.log failed!");
			WriteLog("[Initiator::Start]: Open log file failed!");
			return 1;
		}

		//初始化，读取XML配置文件，创建Channel
		TiXmlDocument doc( m_configFile.c_str() );
		bool loadOkay = doc.LoadFile();
		if ( !loadOkay )	//printf( "Could not load test file 'config.xml'. Error='%s'. Exiting.\n", doc.ErrorDesc() );
		{
			sprintf(configStruct->errorInfo, "[Initiator::Start]: Could not load config.xml(%s). Exiting.\n", m_configFile.c_str());
			return 1;
		}

		TiXmlHandle handleDoc(&doc);
		TiXmlElement* pElementChannel = handleDoc.FirstChildElement( "configuration" ).FirstChildElement( "channel" ).ToElement();	

		for (pElementChannel; pElementChannel; pElementChannel = pElementChannel->NextSiblingElement())
		{
			//获取channel id
			std::string strChannelID;
			if (pElementChannel->QueryStringAttribute("id", &strChannelID) != TIXML_SUCCESS)
				continue;
			m_setChannelIDs.insert(strChannelID);

			TiXmlHandle handleChannel(pElementChannel);
			TiXmlElement* pElementConnection = handleChannel.FirstChildElement("connections").FirstChildElement("connection").ToElement();

			for (pElementConnection; pElementConnection; pElementConnection = pElementConnection->NextSiblingElement())
			{
				std::string strConnectionID;
				ConnectionInfo connectionInfo;
				if (pElementConnection->QueryStringAttribute( "id", &strConnectionID ) != TIXML_SUCCESS)
					continue;

				//获取connection信息，ip，host-ip，port
				TiXmlElement* pElementIp = pElementConnection->FirstChildElement("ip");
				if (pElementIp)
				{
					connectionInfo.ip = pElementIp->GetText();
				}
				TiXmlElement* pElementHostIp = pElementConnection->FirstChildElement("host-ip");
				if (pElementHostIp)
				{
					connectionInfo.hostip = pElementHostIp->GetText();
				}
				TiXmlElement* pElementPort = pElementConnection->FirstChildElement("port");
				if (pElementPort)
				{
					connectionInfo.port = pElementPort->GetText();
				}
				m_mapConnID2Info.insert(std::pair<std::string, ConnectionInfo>(strConnectionID, connectionInfo));
			}
		}

		//channel数量为0
		if (!m_setChannelIDs.size())
		{
			strcpy(configStruct->errorInfo, "[Initiator::Start]: No Channel defined!");
			WriteLog("[Initiator::Start]: No Channel defined!");
			return 1;
		}

		//为每个channelID创建channel
		SETChannelIDs::iterator i = m_setChannelIDs.begin();
		for ( ; i != m_setChannelIDs.end(); i++ )
		{
			Channel* pChannel = new Channel( *i, this );
			m_mapChannels[*i] = pChannel;
		}

		//读取SBE解析文件 templates_FixBinary.sbeir
		if ( m_irRepo.loadFromFile( m_templateFile.c_str() ) == -1 || m_irRepoX.loadFromFile( m_templateFile.c_str() ) == -1 )
		{
			strcpy(configStruct->errorInfo, "[Initiator::Start]: Could not load IR!\n");
			return 1;
		}

		//创建接收线程
		if ( (m_hThreads[0] = (HANDLE)_beginthreadex(NULL, 0, &receiverThread, this, 0, NULL)) == 0 )
		{
			strcpy(configStruct->errorInfo, "[Initiator::Start]: Unable to spawn receiver thread!\n");
			return 1;
		}

		//创建处理线程
		if ( (m_hThreads[1] = (HANDLE)_beginthreadex(NULL, 0, &processorThread, this, 0, NULL)) == 0 )
		{
			strcpy(configStruct->errorInfo, "[Initiator::Start]: Unable to spawn processor thread!\n");
			return 1;
		}
		return 0;
	}

	void Initiator::Stop()
	{
		WriteLog("[Initiator::Stop]: Called!");
		SetEvent(m_hEventStop);

		//等待接收线程退出
		WaitForMultipleObjects(2, m_hThreads, TRUE, INFINITE);

		//断开所有socket连接，清空连接信息
		MapSocketToSocketInfo::iterator i = m_mapSocket2Info.begin();
		for ( ; i !=  m_mapSocket2Info.end(); ++i )
		{
			//断开组播连接，并delete i->second
			UDPdisconnect( i->first, i->second->multiaddr, i->second->interface );
			delete i->second;
		}
		m_mapSocket2Info.clear();

		//delete Channels
		MAPChannels::iterator j = m_mapChannels.begin();
		for ( ; j != m_mapChannels.end(); ++j )
		{
			delete j->second;
		}
		m_mapChannels.clear();
		m_fMDPLog.close();
	}

	void Initiator::ReceiveThread()
	{
		//TODO:加入组播分为两种情形，目前实现的是第2种
		//1.Pre-Opening Startup
		//2.Late Joiner Startup
		//TODO:按Channel ID订阅，具体需求还需研究
		SETChannelIDs::iterator i = m_setChannelIDs.begin();
		for ( ; i != m_setChannelIDs.end(); i++ )
		{
			WriteLog("Subscribe to channel, id=%s", (*i).c_str());
			MAPChannels::iterator j = m_mapChannels.find( *i );
			if (j != m_mapChannels.end())
			{
				j->second->subscribeIncremental();
			}
		}

		while (WaitForSingleObject(m_hEventStop, 0) != WAIT_OBJECT_0)
		{
			fd_set readSet;
			FD_ZERO( &readSet );
			BuildSet( m_readSockets, readSet );

			int result = select( FD_SETSIZE, &readSet, NULL, NULL, &m_timeval );//&writeSet, &exceptSet, &m_timeval );

			if ( result == 0 )
			{
#ifdef _DEBUG
				WriteLog("select() timeout!");
#endif
			}
			else if ( result > 0 )
			{
				ProcessReadSet( readSet );
			}
			else
			{
#ifdef _DEBUG
				WriteLog( "select SOCKET_ERROR GetLastError():%d", GetLastError() );
				Sleep(2000);
#endif
			}
		}

		WriteLog("[ReceiveThread]: Receiver thread exit");
	}

	void Initiator::BuildSet( const Sockets& sockets, fd_set& watchSet )
	{
		Sockets::const_iterator iter;
		for ( iter = sockets.begin(); iter != sockets.end(); ++iter ) {
			FD_SET( *iter, &watchSet );
		}
	}

	void Initiator::ProcessReadSet( fd_set& readSet )
	{
		for ( unsigned i = 0; i < readSet.fd_count; ++i )
		{
			int s = readSet.fd_array[ i ];
			if ( !onData( s ) )
			{
				//onDisconnect( s );
			}
		}
	}

	void Initiator::ProcessThread()
	{
		//m_fLog << "Processor thread start...\n" ;
		HANDLE lpHandles[2];
		lpHandles[0] = m_hEventStop;
		lpHandles[1] = m_hEventData;
		DWORD dwWaitResult;
		//程序没有终止并且有行情包未处理
		while ((dwWaitResult = WaitForMultipleObjects(2, lpHandles, FALSE, INFINITE)) != WAIT_OBJECT_0)
		{
			if (dwWaitResult == WAIT_OBJECT_0 + 1)
			{
				//获取数据包
				Packet packet;
				if (FrontPacket(packet) == -1)
					continue;

				WriteLog("[ProcessThread]: packet seqNum:%d", packet.getSeqNum() );

				char* pBuf = packet.getPacketPointer() + 12;//sizeof(PacketHeader);
				int len = packet.getPacketSize() - 12;
				Listener listener;
				UINT16 uMsgSize;
				while ( len > 0 )
				{
					uMsgSize = *(UINT16* )pBuf;
					//Message Header --Message Size 2Bytes
					pBuf += 2;
					len -= 2;

					//CarCallbacks carCbs(listener, &m_fMDPLog);
					CarCallbacks carCbs(listener);
					listener.dispatchMessageByHeader(m_irRepoX.header(), &m_irRepoX)
						.resetForDecode(pBuf , len)
						.subscribe(&carCbs, &carCbs, &carCbs);

					if (carCbs.getStatus())
					{
						//解析成功
						m_application->onMarketData(carCbs.getFieldMapPtr(), listener.getTemplateId());
					}
					else
					{
						WriteLog("[ProcessThread]: parse fail, left:%d", len);
						break;
					}

					pBuf += uMsgSize-2;
					len -= uMsgSize-2;
				}

				//删除已处理数据包
				PopPacket();
			}
		}
		//m_fLog << "[onProcessorStart]: left:" << m_packetQueue.size() << std::endl;
		WriteLog("[ProcessThread]: Processor thread exit...");
	}

	bool Initiator::SubscribeInstrumentDefinition( const std::string& c )
	{
		MAPChannels::iterator i = m_mapChannels.find( c );
		if (i == m_mapChannels.end())
			return false;

		Channel* pChannel = i->second;

		///Instrument Replay UDP Feed A
		std::string str = c + "NA";
		MAPConnectionIDtoInfo::iterator j = m_mapConnID2Info.find(str);
		if (j == m_mapConnID2Info.end())
			return false;
		//	throw ConfigError("can't find NA connection info");
		ConnectionInfo& cNA = j->second;
		int result = UDPconnect( cNA.ip, atoi(cNA.port.c_str()), m_localInterface );
		if ( result != -1 )
		{
			SocketInfo* pSocketInfoNA = new SocketInfo;
			pSocketInfoNA->pChannel = pChannel;
			pSocketInfoNA->iConnectionType = 2;
			pSocketInfoNA->multiaddr = cNA.ip;
			pSocketInfoNA->interface = m_localInterface;
			MapSocketToSocketInfo::iterator iter = m_mapSocket2Info.find(result);
			if ( iter != m_mapSocket2Info.end())
				delete iter->second; 
			m_mapSocket2Info[result] = pSocketInfoNA;
		}

		return true;
	}

	bool Initiator::SubscribeIncremental( const std::string& c )
	{
		MAPChannels::iterator i = m_mapChannels.find( c );
		if (i == m_mapChannels.end())
		{
			WriteLog("[SubscribeIncremental]: Can't find pChannel of channel id=%s", c.c_str());
			return false;
		}
		Channel* pChannel = i->second;
		///Incremental UDP Feed A
		std::string str = c + "IA";
		MAPConnectionIDtoInfo::iterator j = m_mapConnID2Info.find(str);
		if (j == m_mapConnID2Info.end())
		{
			WriteLog("[SubscribeIncremental]: Can't find connection info of connection id=%s", str.c_str());
			return false;
		}

		ConnectionInfo& cIA = j->second;
		int result = UDPconnect( cIA.ip, atoi(cIA.port.c_str()), m_localInterface );
		if ( result != -1)
		{
			SocketInfo* pSocketInfoIA = new SocketInfo;
			pSocketInfoIA->pChannel = pChannel;
			pSocketInfoIA->iConnectionType = 1;
			pSocketInfoIA->multiaddr = cIA.ip;
			pSocketInfoIA->interface = m_localInterface;
			MapSocketToSocketInfo::iterator iter = m_mapSocket2Info.find(result);
			if ( iter != m_mapSocket2Info.end())
				delete iter->second; 
			m_mapSocket2Info[result] = pSocketInfoIA;
		}

		///Incremental UDP Feed B
		str.clear();
		str = c + "IB";
		j = m_mapConnID2Info.find(str);
		if (j == m_mapConnID2Info.end())
		{
			WriteLog("[SubscribeIncremental]: Can't find connection info of connection id=%s", str.c_str());
			return false;
		}
		ConnectionInfo& cIB = j->second;
		result = UDPconnect( cIB.ip, atoi(cIB.port.c_str()), m_localInterface );
		if ( result != -1)
		{
			SocketInfo* pSocketInfoIB = new SocketInfo;
			pSocketInfoIB->pChannel = pChannel;
			pSocketInfoIB->iConnectionType = 1;
			pSocketInfoIB->multiaddr = cIB.ip;
			pSocketInfoIB->interface = m_localInterface;
			MapSocketToSocketInfo::iterator iter = m_mapSocket2Info.find(result);
			if ( iter != m_mapSocket2Info.end())
				delete iter->second; 
			m_mapSocket2Info[result] = pSocketInfoIB;
		}
		return true;
	}

	bool Initiator::SubscribeMarketRecovery( const std::string& c )
	{
		MAPChannels::iterator i = m_mapChannels.find( c );
		if (i == m_mapChannels.end())
			return false;

		Channel* pChannel = i->second;

		///Snapshot UDP Feed A
		std::string str = c + "SA";
		MAPConnectionIDtoInfo::iterator j = m_mapConnID2Info.find(str);
		if (j == m_mapConnID2Info.end())
			return false;
		//	throw ConfigError("can't find SA connection info");
		ConnectionInfo& cSA = j->second;
		int result = UDPconnect( cSA.ip, atoi(cSA.port.c_str()), m_localInterface );
		if (result != -1)
		{
			SocketInfo* pSocketInfoSA = new SocketInfo;
			pSocketInfoSA->pChannel = pChannel;
			pSocketInfoSA->iConnectionType = 3;
			pSocketInfoSA->multiaddr = cSA.ip;
			pSocketInfoSA->interface = m_localInterface;
			MapSocketToSocketInfo::iterator iter = m_mapSocket2Info.find(result);
			if ( iter != m_mapSocket2Info.end())
				delete iter->second; 
			m_mapSocket2Info[result] = pSocketInfoSA;
		}

		return true;
	}

	bool Initiator::Unsubscribe(const int sock)
	{
		MapSocketToSocketInfo::iterator i = m_mapSocket2Info.find(sock);
		if (i != m_mapSocket2Info.end())
		{
			SocketInfo* pSocketInfo = i->second;
			UDPdisconnect(sock, pSocketInfo->multiaddr, pSocketInfo->interface);
			delete pSocketInfo;
			m_mapSocket2Info.erase(i);
			return true;
		}
		return false;
	}
	
	int Initiator::UDPconnect(const std::string& address, const int port, const std::string& localInterface)
	{
		struct sockaddr_in local;
		struct ip_mreq imreq;
		int iResult = 0;
		BOOL bOptVal = TRUE;
		int bOptLen = sizeof( BOOL );
		int ListenSocket = socket(AF_INET, SOCK_DGRAM, 0 );
		if ( ListenSocket == SOCKET_ERROR )
		{
			WriteLog("[Initiator::UDPconnect]: socket() error");
			return -1;
		}

		iResult = setsockopt( ListenSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&bOptVal, bOptLen );
		if ( iResult == SOCKET_ERROR )
		{
			WriteLog("[Initiator::UDPconnect]: setsockopt() SO_REUSEADDR error");
			return -1;
		}

		//将sock绑定到本机某端口上。
		memset(&local, 0, sizeof(local));
		local.sin_family = AF_INET;
		local.sin_port = htons(port);
		local.sin_addr.s_addr = htonl(INADDR_ANY);

		iResult = bind( ListenSocket,(struct sockaddr*)&local, sizeof(struct sockaddr_in) );
		if( SOCKET_ERROR == iResult )
		{
			WriteLog("[Initiator::UDPconnect]: bind() error");
			return -1;
		}
		//加入多播组
		imreq.imr_interface.s_addr = htonl(INADDR_ANY);
		//imreq.imr_interface.s_addr = inet_addr(localInterface.c_str());
		imreq.imr_multiaddr.s_addr = inet_addr(address.c_str());

		iResult = setsockopt( ListenSocket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (const char *)&imreq, sizeof(imreq));
		if ( iResult == SOCKET_ERROR )
		{
			WriteLog("[Initiator::UDPconnect]: setsockopt() IP_ADD_MEMBERSHIP error");
			//g_lpfnWriteLog(LOG_DEBUG, "[UDPconnect]: setsockopt error, Error code:%d\n", WSAGetLastError() );
			return -1;
		}

		u_long opt = 1;
		::ioctlsocket( ListenSocket, FIONBIO, &opt );
		m_readSockets.insert( ListenSocket );

		return ListenSocket;
	}

	void Initiator::UDPdisconnect(int socket, const std::string& address, const std::string& localInterface)
	{
		int iResult = 0;
		struct ip_mreq imreq;
		//imreq.imr_interface.s_addr = inet_addr(localInterface.c_str());
		imreq.imr_interface.s_addr = htonl(INADDR_ANY);
		imreq.imr_multiaddr.s_addr = inet_addr(address.c_str());
		iResult = setsockopt( socket, IPPROTO_IP, IP_DROP_MEMBERSHIP, (const char *)&imreq, sizeof(imreq));
		if ( iResult == SOCKET_ERROR )
		{
			WriteLog( "[UDPdisconnect]: setsockopt error, Error code:%d\n", WSAGetLastError() );
		}
		Sockets::iterator i = m_readSockets.find( socket );

		if ( i != m_readSockets.end() )
		{
			shutdown( socket, 2 );
			closesocket( socket );
			m_readSockets.erase( socket );
		}
	}

	bool Initiator::onData( const int sock )
	{
		MapSocketToSocketInfo::iterator i = m_mapSocket2Info.find( sock );
		if ( i != m_mapSocket2Info.end() )
		{
			SocketInfo* pSocketInfo = i->second;
			return pSocketInfo->pChannel->read(sock, pSocketInfo->iConnectionType);
		}
		else
		{
			return false;
		}
	}

	void Initiator::onError( )
	{
		//g_lpfnWriteLog(LOG_WARNING, "[onError]: select() error occurred, error code:%d\n", WSAGetLastError());
		//onTimeout( connector );
	}

	void Initiator::onTimeout(  )
	{
		//g_lpfnWriteLog(LOG_DEBUG, "[onTimeout]: select() timeout...\n");
		/*
		time_t now;
		time( &now );
		//30秒以上没有收到数据，关闭引擎
		if ( (now - m_lastTimeOut) >= 15 )
		{
		if (m_bLog)
		{
		g_lpfnWriteLog(LOG_WARNING, "[onTimeout]: No packet received after 15 seconds.\n");
		m_bLog = FALSE;
		}
		}		else		{		g_lpfnWriteLog(LOG_DEBUG, "[onTimeout]: select() timeout...\n");		if (!m_bLog)		m_bLog = TRUE;		}		*/		/*
		SocketToUDPConnections::iterator i;
		for ( i= m_socketToUDPConnections.begin(); i != m_socketToUDPConnections.end(); ++i )
		{
		i->second->onTimeout();//TODO
		}
		*/
		//		m_rtsConnection->next();
	}


	unsigned int _stdcall Initiator::receiverThread( void* p )
	{
		Initiator * pInitiator = static_cast < Initiator* > ( p );
		pInitiator->ReceiveThread();
		return 0;
	}

	unsigned int _stdcall Initiator::processorThread( void* p )
	{
		Initiator * pInitiator = static_cast < Initiator* > ( p );
		pInitiator->ProcessThread();
		return 0;
	}

}