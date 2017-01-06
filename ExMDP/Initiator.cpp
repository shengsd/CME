#include "stdafx.h"
#include "Initiator.h"
#include "TinyXML/tinystr.h"
#include "TinyXML/tinyxml.h"

namespace MDP
{
	Initiator::Initiator()
	{
		m_hIOCP = NULL;
		InitializeCriticalSection( &m_csirRepo );
		InitializeCriticalSection( &m_csLog );
		WORD version = MAKEWORD( 2, 2 );
		WSADATA data;
		WSAStartup( version, &data );
	}

	Initiator::~Initiator()
	{
		DeleteCriticalSection( &m_csirRepo );
		DeleteCriticalSection( &m_csLog );
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

		//打开（创建）引擎日志文件
		char szPath[MAX_PATH] = {0};
		GetModuleFileName(NULL, szPath, MAX_PATH);
		char* pExeDir = strrchr(szPath, '\\');
		*pExeDir = '\0';
		sprintf(szPath, "%s\\MDPEngineLog", szPath);
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
			WriteLog("[Initiator::Start]: Could not load config.xml(%s). Exiting.\n", m_configFile.c_str());
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

		//读取SBE解析文件 templates_FixBinary.sbeir
		if ( m_irRepo.loadFromFile( m_templateFile.c_str() ) == -1 )
		{
			strcpy(configStruct->errorInfo, "[Initiator::Start]: Could not load IR!\n");
			WriteLog("[Initiator::Start]: Could not load IR!\n");
			return 1;
		}

		//创建完成端口
		m_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
		if (!m_hIOCP)
		{
			strcpy(configStruct->errorInfo, "[Initiator::Start]: CreateIoCompletionPort failed!\n");
			WriteLog("[Initiator::Start]: CreateIoCompletionPort failed!\n");
			return 1;
		}

		//为每个channelID创建channel
		SETChannelIDs::iterator i = m_setChannelIDs.begin();
		for ( ; i != m_setChannelIDs.end(); i++ )
		{
			Channel* pChannel = new Channel( *i, this );
			m_mapChannels[*i] = pChannel;
			pChannel->subscribeIncremental();
		}

		//创建数据处理线程
		SYSTEM_INFO systemInfo;
		GetSystemInfo(&systemInfo);
		m_pThreadHandles = new HANDLE[systemInfo.dwNumberOfProcessors];
		for (DWORD i = 0; i < systemInfo.dwNumberOfProcessors; i++)
		{
			m_pThreadHandles[i] = (HANDLE)_beginthreadex(NULL, 0, &WorkThreadStartAddr, this, 0, NULL);
			if ( !m_pThreadHandles[i] )
			{
				WriteLog("[Initiator::Start]: Unable to spawn work thread! thread num:%d\n", i);
				strcpy(configStruct->errorInfo, "[Initiator::Start]: Unable to spawn work thread!\n");
				return 1;
			}
		}
		
		return 0;
	}

	void Initiator::Stop()
	{
		WriteLog("[Initiator::Stop]: Called!");
		CloseHandle(m_hIOCP);
		SYSTEM_INFO systemInfo;
		GetSystemInfo(&systemInfo);
		DWORD dwRet = WaitForMultipleObjects(systemInfo.dwNumberOfProcessors, m_pThreadHandles, TRUE, INFINITE);
		if (dwRet == WAIT_OBJECT_0)
		{
			WriteLog("[Initiator::Stop]: All working threads exited!");
		}
		else
		{
			DWORD dwError = GetLastError();
			WriteLog("[Initiator::Stop]: WaitForMultipleObjects code=%d, lastError=%d", dwRet, dwError);
		}
		delete [] m_pThreadHandles;

		//delete Channels
		MAPChannels::iterator i = m_mapChannels.begin();
		for ( ; i != m_mapChannels.end(); ++i )
		{
			delete i->second;
		}

		m_mapChannels.clear();
		m_mapConnID2Info.clear();
		m_fMDPLog.close();
	}

	BOOL Initiator::Subscribe(Channel* pChannel, int nDataType)
	{
		//找到连接信息
		std::string str = pChannel->m_ChannelID;
		switch (nDataType)
		{
		case INCREMENTAL_A: str += "IA"; break;
		case INCREMENTAL_B: str += "IB"; break;
		case INSTRUMENT_RPLAY_A: str += "NA"; break;
		case INSTRUMENT_RPLAY_B: str += "NB"; break;
		case SNAPSHOT_A: str += "SA"; break;
		case SNAPSHOT_B: str += "SB"; break;
		default: break;
		}

		MAPConnectionIDtoInfo::iterator j = m_mapConnID2Info.find(str);
		if (j == m_mapConnID2Info.end())
		{
			WriteLog("[Initiator::Subscribe]: Can't find connection info of connection id=%s", str.c_str());
			return FALSE;
		}
		ConnectionInfo& cIA = j->second;

		//加入组播
		SOCKET s = UDPJoin( cIA.ip, atoi(cIA.port.c_str()), m_localInterface );
		if (s == SOCKET_ERROR)
			return FALSE;
		else
			pChannel->m_setSockets.insert(s);

		PER_SOCKET_DATA *pCompKey = &pChannel->m_CompKey[nDataType];
		ZeroMemory(pCompKey, sizeof(PER_SOCKET_DATA));
		pCompKey->s = s;
		pCompKey->nDataType = nDataType;
		pCompKey->pChannel = (Channel *)pChannel;

		//关联完成端口
		if (NULL == CreateIoCompletionPort(HANDLE(s), m_hIOCP,  (ULONG_PTR)pCompKey, 0) )
		{
			WriteLog("[Initiator::Subscribe]: Associate SOCKET failed! GetLastError()=%d", GetLastError());
			return FALSE;
		}

		//发起一次接收
		PER_IO_DATA *pOverlapped = &pChannel->m_Overlapped[nDataType];
		ZeroMemory(pOverlapped, sizeof(PER_IO_DATA));
		pOverlapped->dataBuf.buf = pOverlapped->buffer;
		pOverlapped->dataBuf.len = BUFFER_SIZE;
		pOverlapped->fromAddrLen = sizeof(pOverlapped->fromAddr);
		int iRet = WSARecvFrom(pCompKey->s, &(pOverlapped->dataBuf), 1, &(pOverlapped->recvBytes), &(pOverlapped->flags), &(pOverlapped->fromAddr), &(pOverlapped->fromAddrLen), &(pOverlapped->overlapped), NULL);
		if (iRet == 0)//测试
		{
			WriteLog("[Initiator::Subscribe]: receive operation has completed immediately");
		}
		else
		{
			int nError = WSAGetLastError();
			if ( nError  == WSA_IO_PENDING)
			{
				WriteLog("[Initiator::Subscribe]: WSARecvFrom the overlapped operation has been successfully initiated and that completion will be indicated at a later time");
			}
			else if ( nError  == WSA_OPERATION_ABORTED)
			{
				WriteLog("[Initiator::Subscribe]: WSARecvFrom An overlapped operation was canceled due to the closure of the socket");
			}
			else
			{
				WriteLog("[Initiator::Subscribe]: WSARecvFrom error=%d", nError);
			}
		}

		return TRUE;
	}

	void Initiator::PushPacket( char* str, int size )
	{
		char* pBuf = str + 12;
		int nSize = size - 12;
		Listener listener;
		UINT16 uMsgSize;
		while ( nSize > 0 )
		{
			uMsgSize = *(UINT16* )pBuf;
			//Message Header --Message Size 2Bytes
			pBuf += 2;
			nSize -= 2;

			//CarCallbacks carCbs(listener, &m_fMDPLog);
			CarCallbacks carCbs(listener);
			EnterCriticalSection(&m_csirRepo);
			listener.dispatchMessageByHeader(m_irRepo.header(), &m_irRepo)
				.resetForDecode(pBuf , nSize)
				.subscribe(&carCbs, &carCbs, &carCbs);
			LeaveCriticalSection(&m_csirRepo);
			if (carCbs.getStatus())
			{
				//解析成功
				m_application->onMarketData(carCbs.getFieldMapPtr(), listener.getTemplateId());
			}
			else
			{
				WriteLog("[ProcessThread]: parse fail, left:%d", nSize);
				break;
			}

			pBuf += uMsgSize-2;
			nSize -= uMsgSize-2;
		}
	}

	void Initiator::WorkThread()
	{
		static int nThreadCount = 0;
		int nThreadNum = nThreadCount++;
		WriteLog("[Initiator::WorkThread %d]: Started", nThreadNum);
		PER_SOCKET_DATA *pCompKey = NULL;
		PER_IO_DATA *pOverlapped = NULL;
		DWORD dwBytesTransferred, dwLastErrorCode;
		Channel* pChannel = NULL;
		//Session* pSession = NULL;
		while (true)
		{
			if (!GetQueuedCompletionStatus(m_hIOCP, &dwBytesTransferred, (PULONG_PTR)&pCompKey, (LPOVERLAPPED* )&pOverlapped, INFINITE))
			{
				dwLastErrorCode = GetLastError();
				if (pOverlapped == NULL)
				{
					WriteLog("[Initiator::WorkThread %d]: GetQueuedCompletionStatus failed! pOverlapped is NULL, GetLastError()=%d", nThreadNum, dwLastErrorCode);
					if (dwLastErrorCode == ERROR_ABANDONED_WAIT_0)
					{
						break;
					}
				}
				else
				{
					WriteLog("[Initiator::WorkThread %d]: GetQueuedCompletionStatus failed! GetLastError()=%d", nThreadNum, dwLastErrorCode);
				}
				continue;//？
			}

			WriteLog("[Initiator::WorkThread %d]: GetQueuedCompletionStatus succeed! lpNumberOfBytes=%d", nThreadNum, dwBytesTransferred);
			
			if (pCompKey && pOverlapped)
			{
				pChannel = (Channel *)pCompKey->pChannel;
				if (pChannel)
					pChannel->ProcessData(pCompKey, pOverlapped);//处理数据
				
				//继续接收
				int iRet = WSARecvFrom(pCompKey->s, &(pOverlapped->dataBuf), 1, &(pOverlapped->recvBytes), &(pOverlapped->flags), &(pOverlapped->fromAddr), &(pOverlapped->fromAddrLen), &(pOverlapped->overlapped), NULL);
				if (iRet == 0)//测试
				{
					WriteLog("[Initiator::WorkThread %d]: WSARecvFrom receive operation has completed immediately", nThreadNum);
				}
				else
				{
					int nError = WSAGetLastError();
					if ( nError  == WSA_IO_PENDING)
					{
						WriteLog("[Initiator::WorkThread %d]: WSARecvFrom the overlapped operation has been successfully initiated and that completion will be indicated at a later time", nThreadNum);
					}
					else if ( nError  == WSA_OPERATION_ABORTED)
					{
						WriteLog("[Initiator::WorkThread %d]: WSARecvFrom An overlapped operation was canceled due to the closure of the socket", nThreadNum);
					}
					else
					{
						WriteLog("[Initiator::WorkThread %d]: WSARecvFrom error=%d", nThreadNum, nError);
					}
					
				}
			}
		}
		WriteLog("[Initiator::WorkThread %d]: Exited", nThreadNum);
	}


	SOCKET Initiator::UDPJoin(const std::string& address, const int port, const std::string& localInterface)
	{
		struct sockaddr_in local;
		struct ip_mreq imreq;
		int iResult = 0;
		BOOL bOptVal = TRUE;
		int bOptLen = sizeof( BOOL );

		SOCKET ListenSocket = socket(AF_INET, SOCK_DGRAM, 0 );
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
		//u_long opt = 1;
		//::ioctlsocket( ListenSocket, FIONBIO, &opt );
		//m_readSockets.insert( ListenSocket );

		return ListenSocket;
	}
	
	unsigned int _stdcall Initiator::WorkThreadStartAddr( void* p )
	{
		Initiator * pInitiator = static_cast < Initiator* > ( p );
		pInitiator->WorkThread();
		return 0;
	}

}