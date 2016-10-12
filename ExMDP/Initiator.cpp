#include "stdafx.h"
#include "Initiator.h"
#include "TinyXML/tinystr.h"
#include "TinyXML/tinyxml.h"

namespace MDP
{
	Initiator::Initiator():
		m_reconnectInterval(30),
		m_lastTimeOut(0),
		m_connectTimes(0),
		m_onRetransmission(false)
	{
		InitializeCriticalSection( &m_mutex );
		m_hEventData = CreateEvent( NULL, TRUE, FALSE, NULL );
		m_hEventStop = CreateEvent( NULL, TRUE, FALSE, NULL );
		m_bStopped = TRUE;
		m_session = NULL;
		time(&m_lastTimeOut);
	}

	Initiator::~Initiator()
	{
		DeleteCriticalSection( &m_mutex );
		CloseHandle(m_hEventStop);
		CloseHandle(m_hEventData);
		m_fLog.close();
	}

	void Initiator::writeLog(char* szFormat, ...)
	{
		char buffer[256];
		SYSTEMTIME st;
		GetLocalTime(&st);
		sprintf(buffer, "%04d%02d%02d %02d:%02d:%02d:%03d - ", st.wYear, 
			st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
		va_list args;
		va_start (args, szFormat);
		vsprintf (buffer+strlen(buffer), szFormat, args);
		va_end (args);
		m_fLog << buffer << std::endl;
	}

	int Initiator::start( ConfigStruct* configStruct, Application* application)
	{
		if (configStruct == NULL)
			return 1;
		if (application == NULL)
		{
			strcpy(configStruct->errorInfo, "Invalid Param: Application* (NULL)");
			return 1;
		}

		if (!m_bStopped)
		{
			strcpy(configStruct->errorInfo, "[start]: failed! Already started!\n");
			return 1;
		}
		ResetEvent(m_hEventStop);

		m_configFile.assign(configStruct->configFile);
		m_localInterface.assign(configStruct->localInterface);
		m_templateFile.assign(configStruct->templateFile);
		m_username.assign(configStruct->userName);
		m_password.assign(configStruct->passWord);
		m_application = application;

		//打开（创建）日志文件
		char szLog[MAX_PATH] = ".\\CME";
		if (_access(szLog, 0) != 0)
			_mkdir(szLog);
		SYSTEMTIME st;
		GetSystemTime(&st);
		sprintf(szLog, "%s\\MDP_%02d.log", szLog, st.wDay);
		m_fLog.open(szLog, std::ofstream::app );
		if ( !m_fLog.is_open() )
		{
			strcpy(configStruct->errorInfo, "[start]: Open log file failed!\n");
			return 1;
		}

		//初始化，读取XML配置文件，创建Channel
		//read config.xml file
		TiXmlDocument doc( m_configFile.c_str() );

		bool loadOkay = doc.LoadFile();

		if ( !loadOkay )	//printf( "Could not load test file 'config.xml'. Error='%s'. Exiting.\n", doc.ErrorDesc() );
		{
			strcpy(configStruct->errorInfo, "[start]: Could not load config.xml 'config.xml'. Exiting.\n");
			return 1;
		}

		TiXmlHandle handleDoc(&doc);
		TiXmlElement* pElementChannel = handleDoc.FirstChildElement( "configuration" ).FirstChildElement( "channel" ).ToElement();	

		for (pElementChannel; pElementChannel; pElementChannel = pElementChannel->NextSiblingElement())
		{
			//获取channel id
			int iTemp;
			if (pElementChannel->QueryIntAttribute("id", &iTemp) != TIXML_SUCCESS)
				continue;
			m_channelIDs.insert(iTemp);

			TiXmlHandle handleChannel(pElementChannel);
			TiXmlElement* pElementConnection = handleChannel.FirstChildElement("connections").FirstChildElement("connection").ToElement();

			for (pElementConnection; pElementConnection; pElementConnection = pElementConnection->NextSiblingElement())
			{
				std::string id;
				ConnectionInfo connectionInfo;
				if (pElementConnection->QueryStringAttribute( "id", &id ) != TIXML_SUCCESS)
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
				m_connectionIDtoInfo.insert(std::pair<std::string, ConnectionInfo>(id, connectionInfo));
			}
		}

		//channel数量为0
		if (!m_channelIDs.size())
		{
			strcpy(configStruct->errorInfo, "[initialize]: No Channel defined!\n");
			return 1;
		}

		//为每个channelID创建channel
		ChannelIDs::iterator i = m_channelIDs.begin();
		for ( ; i != m_channelIDs.end(); i++ )
		{
			Channel* pChannel = new Channel( *i, this );
			m_channels[*i] = pChannel;
		}

		//读取SBE解析文件 templates_FixBinary.sbeir
		if ( m_irRepo.loadFromFile( m_templateFile.c_str() ) == -1 || m_irRepoX.loadFromFile( m_templateFile.c_str() ) == -1 )
		{
			strcpy(configStruct->errorInfo, "[initialize]: could not load IR!\n");
			return 1;
		}

		//创建接收线程
		if ( (m_hThreads[0] = (HANDLE)_beginthreadex(NULL, 0, &receiverThread, this, 0, NULL)) == 0 )
		{
			strcpy(configStruct->errorInfo, "[start]: Unable to spawn receiver thread!\n");
			return 1;
		}

		//创建处理线程
		if ( (m_hThreads[1] = (HANDLE)_beginthreadex(NULL, 0, &processorThread, this, 0, NULL)) == 0 )
		{
			strcpy(configStruct->errorInfo, "[start]: Unable to spawn processor!\n");
			return 1;
		}

		m_bStopped = FALSE;
		return 0;
	}


	int Initiator::stop()
	{
		if (m_bStopped)
			return 1;
		//m_fLog << "[Initiator]: stop called\n";
		SetEvent(m_hEventStop);

		//等待接收线程退出
		WaitForMultipleObjects(2, m_hThreads, TRUE, INFINITE);

		//断开所有socket连接，清空连接信息
		//g_lpfnWriteLog(LOG_DEBUG, "m_socketToSocketInfo.size:%d\n", m_socketToSocketInfo.size());
		MapSocketToSocketInfo::iterator i; 
		SocketInfo* p;
		while (!m_socketToSocketInfo.empty())
		{
			i = m_socketToSocketInfo.begin();
			p = i->second;
			if (p->iConnectionType == 4)
			{
				m_session->disconnect();
				m_onRetransmission = false;
				delete m_session;
				m_session = NULL;
			}
			else
			{
				//断开组播连接，并delete i->second
				//disConnectMulticast(i->first);
				m_connector.UDPdisconnect(i->first, p->multiaddr, p->interface);
			}
			delete p;
			m_socketToSocketInfo.erase(i);
		}

		//g_lpfnWriteLog(LOG_DEBUG, "m_connectionIDtoInfo.size:%d\n", m_connectionIDtoInfo.size());
		//ConnectionIDtoInfo::iterator k = m_connectionIDtoInfo.begin();
		//for ( ; k != m_connectionIDtoInfo.end(); ++k )
		//{
		//清空Channel连接信息
		//delete k->second;
		//}

		//g_lpfnWriteLog(LOG_DEBUG, "m_channels.size:%d\n", m_channels.size());
		Channels::iterator j = m_channels.begin();
		for ( ; j != m_channels.end(); ++j )
		{
			//清空各个Channel中的缓存
			Channel* p = j->second;
			// 			p->clearPacketQueue();
			// 			p->clearRealTimeSpool();
			delete p;
		}

		m_fLog.close();
		//m_fLog << "Engine Stopped..." << std::endl;
		m_bStopped = TRUE;
		return 0;
	}

	void Initiator::onReceiverStart()
	{
		//m_fLog << "Receiver thread start...\n";
		//TODO:加入组播分为两种情形，目前实现的是第2种
		//1.Pre-Opening Startup
		//2.Late Joiner Startup
		//TODO:按Channel ID订阅，具体需求还需研究
		ChannelIDs::iterator i = m_channelIDs.begin();
		for ( ; i != m_channelIDs.end(); i++ )
		{
			//g_lpfnWriteLog(LOG_INFO, "Connecting to Channel ID[%d]\n", *i);
			//doConnectToRealTime( *i );
			Channels::iterator j = m_channels.find( *i );
			if (j != m_channels.end())
			{
				j->second->subscribeInstrumentDefinition();
			}
		}

		while (WaitForSingleObject(m_hEventStop, 0) != WAIT_OBJECT_0)
		{
			static int count = 0;
			m_connector.block(*this, false, 1);
			writeLog( "[onReceiverStart]: inLoop %d",  ++count);
		}

		//g_lpfnWriteLog(LOG_DEBUG, "Receiver thread exit...\n");
		writeLog("Receiver thread exit...");
	}

	void Initiator::onProcessorStart()
	{
		//m_fLog << "Processor thread start...\n" ;
		HANDLE lpHandles[2];
		lpHandles[0] = m_hEventStop;
		lpHandles[1] = m_hEventData;
		DWORD dwWaitResult;
		//程序没有终止并且有行情包未处理
		while ((dwWaitResult = WaitForMultipleObjects(2, lpHandles, FALSE, INFINITE)) != WAIT_OBJECT_0)
		{
			//m_fDecoding << "[onProcessorStart]: left:";
			if (dwWaitResult == WAIT_OBJECT_0 + 1)
			{
				//获取数据包
				Packet packet;
				if (FrontPacket(packet) == -1)
					continue;

				//g_lpfnWriteLog(LOG_DEBUG, "[onProcessorStart]: packet seqNum:%d", packet.getSeqNum());

				m_fLog << "[onProcessorStart]: packet seqNum:" << packet.getSeqNum() << std::endl;
				writeLog("[onProcessorStart]: packet seqNum:%d", packet.getSeqNum() );

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

					CarCallbacks carCbs(listener, &m_fLog);
					listener.dispatchMessageByHeader(m_irRepoX.header(), &m_irRepoX)
						.resetForDecode(pBuf , len)
						.subscribe(&carCbs, &carCbs, &carCbs);

					if (carCbs.getStatus())
					{
						writeLog("[onProcessorStart]: parse success, message template ID:%d", listener.getTemplateId());
						//解析成功
						//g_lpfnWriteLog(LOG_DEBUG, "[onProcessorStart]: packet parse success");
						m_application->onMarketData(carCbs.getFieldMapPtr(), listener.getTemplateId());
					}
					else
					{
						writeLog("[onProcessorStart]: parse fail, left:%d", len);
						break;
					}
					//pBuf += listener.bufferOffset();
					//len -= listener.bufferOffset();
					pBuf += uMsgSize-2;
					len -= uMsgSize-2;
				}

				//删除已处理数据包
				PopPacket();
			}
		}
		//m_fLog << "[onProcessorStart]: left:" << m_packetQueue.size() << std::endl;
		writeLog("Processor thread exit...");
		//g_lpfnWriteLog(LOG_DEBUG, "Processor thread exit...\n");
		/*
		//int num = 0;
		//int packetsNo = 0;
		while (!isStopped())
		{
		Channels::iterator i = m_channels.begin();
		for ( ; i != m_channels.end(); ++i )
		{
		Channel* p = i->second;

		num = (p->m_packetQueue.size() <= 100) ? p->m_packetQueue.size() : 100;
		while ( num )
		{
		Packet& packet = p->m_packetQueue.front();
		char* data = packet.getPacketPointer();

		char* pBuf = data + 12;
		int len = packet.getPacketSize() - 12;

		Listener listener;
		while ( len > 0 )
		{
		//Message Header
		//	--Message Size 2Bytes
		pBuf += 2;
		len -= 2;
		CarCallbacks carCbs(listener);
		listener.dispatchMessageByHeader(m_irRepo.header(), &m_irRepo)
		.resetForDecode(pBuf , len)
		.subscribe(&carCbs, &carCbs, &carCbs);

		if (carCbs.getStatus())
		{
		//解析成功
		m_application->onMarketData(carCbs.getFieldMapPtr(), listener.getTemplateId());
		}
		pBuf += listener.bufferOffset();
		len -= listener.bufferOffset();
		}

		delete[] pBuf;
		p->m_packetQueue.pop();
		++packetsNo;
		--num;
		}
		}
		if (packetsNo)
		Sleep(1000);
		else
		packetsNo = 0;
		}
		*/
	}

	bool Initiator::doConnectToRealTime( const ChannelID c )
	{
		Channels::iterator i = m_channels.find( c );
		if (i == m_channels.end())
			return false;

		Channel* pChannel = i->second;

		///Incremental UDP Feed A
		std::string str;
		std::stringstream ss;
		ss << c;
		ss >> str;
		str += "IA";
		ConnectionIDtoInfo::iterator j = m_connectionIDtoInfo.find(str);
		if (j == m_connectionIDtoInfo.end())
			return false;
		//throw ConfigError("can't find IA connection info");

		ConnectionInfo& cIA = j->second;

		int result = m_connector.UDPconnect( cIA.ip, atoi(cIA.port.c_str()), m_localInterface );
		if ( result != -1)
		{
			SocketInfo* pSocketInfoIA = new SocketInfo;
			pSocketInfoIA->pChannel = pChannel;
			pSocketInfoIA->iConnectionType = 1;
			pSocketInfoIA->multiaddr = cIA.ip;
			pSocketInfoIA->interface = m_localInterface;
			MapSocketToSocketInfo::iterator iter = m_socketToSocketInfo.find(result);
			if ( iter != m_socketToSocketInfo.end())
				delete iter->second; 
			m_socketToSocketInfo[result] = pSocketInfoIA;
		}

		///Incremental UDP Feed B
		str.clear();
		ss.clear();
		ss << c;
		ss >> str;
		str += "IB";
		j = m_connectionIDtoInfo.find(str);
		if (j == m_connectionIDtoInfo.end())
			return false;
		//	throw ConfigError("can't find IB connection info");
		ConnectionInfo& cIB = j->second;
		result = m_connector.UDPconnect( cIB.ip, atoi(cIB.port.c_str()), m_localInterface );
		if ( result != -1)
		{
			SocketInfo* pSocketInfoIB = new SocketInfo;
			pSocketInfoIB->pChannel = pChannel;
			pSocketInfoIB->iConnectionType = 1;
			pSocketInfoIB->multiaddr = cIB.ip;
			pSocketInfoIB->interface = m_localInterface;
			MapSocketToSocketInfo::iterator iter = m_socketToSocketInfo.find(result);
			if ( iter != m_socketToSocketInfo.end())
				delete iter->second; 
			m_socketToSocketInfo[result] = pSocketInfoIB;
		}

		return true;
	}


	bool Initiator::GenerateRetransRequest( ChannelID channelID, unsigned beginSeqNum, unsigned endSeqNum )
	{
		//The maximum number of messages that can be requested in a given request is 2000.
		if (endSeqNum - beginSeqNum <= 1999)
		{
			if (!m_onRetransmission)
			{
				Channels::iterator i = m_channels.find(channelID);
				if (i == m_channels.end())
					return false;
				//	throw ConfigError("can't find channel");
				Channel* pChannel = i->second;
				std::string str;
				std::stringstream ss;
				ss << channelID;
				ss >> str;
				str += "H0A";
				ConnectionIDtoInfo::iterator j = m_connectionIDtoInfo.find(str);
				if (j == m_connectionIDtoInfo.end())
					return false;
				//	throw ConfigError("can't find H0A connection info");
				ConnectionInfo& cH0A = j->second;

				//pChannel->onEvent("connect to "+ pConnectonInfo->hostip + " on port " + pConnectonInfo->port);
				pChannel->onEvent("connect to "+ cH0A.hostip + " on port " + cH0A.port);

				int result = m_connector.TCPconnect( cH0A.hostip, atoi(cH0A.port.c_str()) );
				if ( result != -1 )
				{
					SocketInfo* pSocketInfoIA = new SocketInfo;
					pSocketInfoIA->pChannel = pChannel;
					pSocketInfoIA->iConnectionType = 4;
					MapSocketToSocketInfo::iterator iter = m_socketToSocketInfo.find(result);
					if ( iter != m_socketToSocketInfo.end())
						delete iter->second; 
					m_socketToSocketInfo[result] = pSocketInfoIA;
					m_session = new Session(pChannel, result, &m_connector, m_username, m_password, beginSeqNum, endSeqNum);
					//发起连接，创建会话后认为已经启动
					m_onRetransmission = true;
				}

				return true;
			}
		}
		return false;
	}

	bool Initiator::subscribeInstrumentDefinition( const ChannelID c )
	{
		Channels::iterator i = m_channels.find( c );
		if (i == m_channels.end())
			return false;

		Channel* pChannel = i->second;

		///Instrument Replay UDP Feed A
		std::string str;
		std::stringstream ss;
		ss << c;
		ss >> str;
		str += "NA";
		ConnectionIDtoInfo::iterator j = m_connectionIDtoInfo.find(str);
		if (j == m_connectionIDtoInfo.end())
			return false;
		//	throw ConfigError("can't find NA connection info");
		ConnectionInfo& cNA = j->second;
		int result = m_connector.UDPconnect( cNA.ip, atoi(cNA.port.c_str()), m_localInterface );
		if ( result != -1 )
		{
			SocketInfo* pSocketInfoNA = new SocketInfo;
			pSocketInfoNA->pChannel = pChannel;
			pSocketInfoNA->iConnectionType = 2;
			pSocketInfoNA->multiaddr = cNA.ip;
			pSocketInfoNA->interface = m_localInterface;
			MapSocketToSocketInfo::iterator iter = m_socketToSocketInfo.find(result);
			if ( iter != m_socketToSocketInfo.end())
				delete iter->second; 
			m_socketToSocketInfo[result] = pSocketInfoNA;
		}

		return true;
	}

	bool Initiator::subscribeMarketRecovery( const ChannelID c )
	{
		Channels::iterator i = m_channels.find( c );
		if (i == m_channels.end())
			return false;

		Channel* pChannel = i->second;
		//if( !session->isSessionTime(UtcTimeStamp()) ) return;
		//Log* log = channel->getLog();
		//log->onEvent( "Connecting to " + address + " on port " + IntConvertor::convert((unsigned short)port) );

		///Snapshot UDP Feed A
		std::string str;
		std::stringstream ss;
		ss << c;
		ss >> str;
		str += "SA";
		ConnectionIDtoInfo::iterator j = m_connectionIDtoInfo.find(str);
		if (j == m_connectionIDtoInfo.end())
			return false;
		//	throw ConfigError("can't find SA connection info");
		ConnectionInfo& cSA = j->second;
		int result = m_connector.UDPconnect( cSA.ip, atoi(cSA.port.c_str()), m_localInterface );
		if (result != -1)
		{
			SocketInfo* pSocketInfoSA = new SocketInfo;
			pSocketInfoSA->pChannel = pChannel;
			pSocketInfoSA->iConnectionType = 3;
			pSocketInfoSA->multiaddr = cSA.ip;
			pSocketInfoSA->interface = m_localInterface;
			MapSocketToSocketInfo::iterator iter = m_socketToSocketInfo.find(result);
			if ( iter != m_socketToSocketInfo.end())
				delete iter->second; 
			m_socketToSocketInfo[result] = pSocketInfoSA;
		}

		return true;
	}

	bool Initiator::unsubscribe(const int sock)
	{
		MapSocketToSocketInfo::iterator i = m_socketToSocketInfo.find(sock);
		if (i != m_socketToSocketInfo.end())
		{
			SocketInfo* pSocketInfo = i->second;
			m_connector.UDPdisconnect(sock, pSocketInfo->multiaddr, pSocketInfo->interface);
			delete pSocketInfo;
			m_socketToSocketInfo.erase(i);
			return true;
		}
		return false;
	}

	//	void Initiator::disconnectFromRF( const ChannelID& c )
	//	{
	// 		UDPConnection* connection = NULL;
	// 		Channels::iterator i = m_channels.find( c );
	// 		if (i == m_channels.end())
	// 		{
	// 			return;
	// 		}
	// 		Channel* channel = i->second;
	// 		///REFRESH LINE A	//		connection = channel->getRFLineAConnection();
	// 		int sock = connection->getSocket();
	// 		onDisconnect( m_connector, sock );
	// 
	// 		connection = channel->getRFLineBConnection();
	// 		sock = connection->getSocket();
	// 		onDisconnect( m_connector, sock );
	// 
	// 		channel->setOnRefresh( false );
	//	}

	//	bool Initiator::doConnectToRts()
	//	{
	// 		time_t now;
	// 		time( &now );
	// 		m_lastConnect = now;
	// 
	// 		std::pair<std::string, int> pairAddressPort;
	// 		if ( m_connectTimes < 2 )
	// 		{
	// 			pairAddressPort = m_rtsConnection->getConnectInfo(m_connectTimes);
	// 			m_connectTimes++;
	// 		}
	// 		int result = m_connector.TCPconnect( pairAddressPort.first, pairAddressPort.second );
	// 		printf("rts result:%d\n", result);
	// 		m_rtsConnection->setSocket( result );
	//	}


	// 	bool Initiator::isStartupTime()
	// 	{
	// 	LocalTimeStamp now;
	// 	LocalTimeStamp startupTime(6,0,0,0);
	// 	return (now - startupTime) > 0 ? true : false;
	// 	}


	bool Initiator::onData( Connector& connector, int sock )
	{
		time(&m_lastTimeOut);
		MapSocketToSocketInfo::iterator i = m_socketToSocketInfo.find( sock );
		if ( i != m_socketToSocketInfo.end() )
		{
			SocketInfo* pSocketInfo = i->second;
			if (pSocketInfo->iConnectionType == 4)
			{
				return m_session->read(connector);
			}
			else
			{
				return pSocketInfo->pChannel->read(sock, pSocketInfo->iConnectionType);
			}
		}
		else
		{
			return false;
		}
	}

	void Initiator::onWrite( Connector& connector, int sock )
	{
		MapSocketToSocketInfo::iterator i = m_socketToSocketInfo.find( sock );
		if ( i== m_socketToSocketInfo.end())
			return ;
		SocketInfo* pSocketInfo = i->second;
		if (pSocketInfo->iConnectionType == 4)
		{
			//if (m_session->processQueue())
			//m_session->unsignal();
		}
	}

	void Initiator::onConnect( Connector&, int sock )
	{
		MapSocketToSocketInfo::iterator i = m_socketToSocketInfo.find( sock );
		if ( i != m_socketToSocketInfo.end() )
		{
			SocketInfo* pSocketInfo = i->second;
			if (pSocketInfo->iConnectionType == 4)
			{
				m_session->next();
			}
		}
	}

	void Initiator::onDisconnect( Connector&, int s )
	{
		MapSocketToSocketInfo::iterator i = m_socketToSocketInfo.find(s);
		if (i != m_socketToSocketInfo.end())
		{
			SocketInfo* socketInfo = i->second;
			//g_lpfnWriteLog(LOG_DEBUG, "Channel ID = %d,connection type = %d, onError, onDisconnecting...\n", socketInfo->pChannel->getChannelID(), socketInfo->iConnectionType);
			if (socketInfo->iConnectionType == 4)
			{
				if (m_session)
				{
					m_session->disconnect();
					m_onRetransmission = false;
					delete m_session;
					m_session = NULL;
				}
				//delete socketInfo;
				//m_socketToSocketInfo.erase(i);
			}
		}
	}

	void Initiator::onError( Connector& connector, int socket )
	{
		onDisconnect( connector, socket );
	}

	void Initiator::onError( Connector& connector )
	{
		//g_lpfnWriteLog(LOG_WARNING, "[onError]: select() error occurred, error code:%d\n", WSAGetLastError());
		//onTimeout( connector );
	}

	void Initiator::onTimeout( Connector& )
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


	THREAD_PROC Initiator::receiverThread( void* p )
	{
		Initiator * pInitiator = static_cast < Initiator* > ( p );
		pInitiator->onReceiverStart();
		return 0;
	}

	THREAD_PROC Initiator::processorThread( void* p )
	{
		Initiator * pInitiator = static_cast < Initiator* > ( p );
		pInitiator->onProcessorStart();
		return 0;
	}

}