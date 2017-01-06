#include "stdafx.h"
#include "Channel.h"


namespace MDP
{
	Channel::Channel( const std::string& channelID, Initiator* pInitiator)
		:m_ChannelID( channelID ),
		m_initiator( pInitiator ),
		m_poolTimeLimit( 5 ),
		m_IncrementalNextSeqNum( 1 ),
		m_InstrumentDefNextSeqNum( 1 ),
		m_MarketRecoveryNextSeqNum( 1 ),
		m_LastMsgSeqNumProcessed( 1 ),
		m_bOnInstrumentDef( false ),
		m_bOnMarketRecovery( false ),
		m_InstDefComplete( false ),
		m_InstrumentDefProcessedNum( 0 ),
		m_MarketRecoveryProcessedNum( 0 )
	{
		InitializeCriticalSection(&m_csChannel);
		char szModulePath[MAX_PATH] = {0};
		GetModuleFileName(NULL, szModulePath, MAX_PATH);
		char* pExeDir = strrchr(szModulePath, '\\');
		*pExeDir = '\0';
		std::string s = szModulePath;
		s.append("\\MDPEngineLog\\"+channelID);
		_mkdir(s.c_str());
		m_fPackets.open( s + "\\packets.log", std::ios::trunc );
		m_fChannel.open(s +"\\channel.log", std::ios::trunc );
	}

	Channel::~Channel(void)
	{
		m_fPackets.close();
		m_fChannel.close();
		unsubscribeAll();
		DeleteCriticalSection(&m_csChannel);
	}

	void Channel::ProcessData(PER_SOCKET_DATA *pCompKey, PER_IO_DATA *pOverlapped)
	{
		EnterCriticalSection(&m_csChannel);
		Packet packet(pOverlapped->buffer, pOverlapped->overlapped.InternalHigh);
		switch (pCompKey->nDataType)
		{
		case INCREMENTAL_A:
		case INCREMENTAL_B:
			processIncrementalPacket(packet, pCompKey->s);
			break;
		case SNAPSHOT_A:
		case SNAPSHOT_B:
			processMarketRecoveryPacket(packet, pCompKey->s);
			break;
		case INSTRUMENT_RPLAY_A:
		case INSTRUMENT_RPLAY_B:
			processInstrumentDefPacket(packet, pCompKey->s);
			break;
		default:
			break;
		}
		LeaveCriticalSection(&m_csChannel);
	}

	void Channel::processIncrementalPacket(Packet& packet, int socket)
	{
		if (packet.getSeqNum() == m_IncrementalNextSeqNum)
		{
			PushPacket(packet.getPacketPointer(), packet.getPacketSize());
			increaseIncrementalNextSeqNum();
			m_fChannel << "[processIncrementalPacket]: sequence num=" << packet.getSeqNum() << "    Action: Push this packet\n";
		}
		else if (packet.getSeqNum() > m_IncrementalNextSeqNum)
		{
			spoolIncrementalPacket(packet);
			m_fChannel << "[processIncrementalPacket]: sequence num=" << packet.getSeqNum() << "     Action: Spool this packet\n";
		}
		else
		{
			//discard duplicate packet
			m_fChannel << "[processIncrementalPacket]: sequence num=" << packet.getSeqNum() << "     Action: Discard this packet\n";
		}
		checkIncrementalSpoolTimer(socket);
		/*if (!m_mapSpoolPacket.empty())
		{
		MAPSpoolPacket::iterator iter = m_mapSpoolPacket.begin();
		for ( ; iter != m_mapSpoolPacket.end(); iter++ )
		{
		m_fChannel << "[TEST checkIncrementalSpoolTimer]: SequenceNum=" << iter->first << std::endl; 
		}
		}*/
	}

	//�ĵ��н��鿪ʼ�����ǰ����Ϊ1�����������Ǵ�����Ϣ���ﵽTag 911-TotNumReports
	void Channel::processInstrumentDefPacket(Packet& packet, const int sock)
	{
		//��Ҫ��������
		if (packet.getSeqNum() == m_InstrumentDefNextSeqNum)
		{
			onPushInstrumentDefPacket(packet, sock);
			PushPacket(packet.getPacketPointer(), packet.getPacketSize());
			increaseInstrumentDefNextSeqNum();
			m_fChannel << "[processInstrumentDefPacket]: sequence num=" << packet.getSeqNum() << "     Action: Push this packet\n";
		}
		else
		{
			//�����������á�������������������һ����  �������µ�һ��ѭ��
			resetInstrumentDef();
			//������µ�һ��ѭ������Ҫ�ٴ�����
			if (packet.getSeqNum() == m_InstrumentDefNextSeqNum)
			{
				onPushInstrumentDefPacket(packet, sock);
				PushPacket(packet.getPacketPointer(), packet.getPacketSize());
				increaseInstrumentDefNextSeqNum();
				m_fChannel << "[processInstrumentDefPacket]: sequence num=" << packet.getSeqNum() << "     Action: Push this packet\n";
			}
			else
			{
				m_fChannel << "[processInstrumentDefPacket]: sequence num=" << packet.getSeqNum() << "     Action: Discard this packet\n";
			}
		}

		if (!m_bOnInstrumentDef)
		{
			resetInstrumentDef();
			m_fChannel << "[processInstrumentDefPacket]: Instrument Recovery completed, reset Instrument Recovery Feed..." << std::endl;
		}
	}

	void Channel::processMarketRecoveryPacket( Packet& packet, const int sock )
	{
		//��1�Ű���ʼ���δ���
		if (packet.getSeqNum() == m_MarketRecoveryNextSeqNum)
		{
			onPushMarketRecoveryPacket( packet, sock );
			PushPacket(packet.getPacketPointer(), packet.getPacketSize());
			increaseMarketRecoveryNextSeqNum();
			m_fChannel << "[processMarketRecoveryPacket]: sequence num=" << packet.getSeqNum() << "     Action: Push this packet\n";
		}
		else
		{
			//������������������һ����  �������µ�һ��ѭ��
			resetMarketRecovery();
			if (packet.getSeqNum() == m_MarketRecoveryNextSeqNum)
			{
				onPushMarketRecoveryPacket( packet, sock );
				PushPacket(packet.getPacketPointer(), packet.getPacketSize());
				increaseMarketRecoveryNextSeqNum();
				m_fChannel << "[processMarketRecoveryPacket]: sequence num=" << packet.getSeqNum() << "     Action: Push this packet\n";
			}
			else
			{
				m_fChannel << "[processMarketRecoveryPacket]: sequence num=" << packet.getSeqNum() << "     Action: Discard this packet\n";
			}
		}

		if (!m_bOnMarketRecovery)
		{
			resetMarketRecovery();
			m_fChannel << "[processMarketRecoveryPacket]: completed, reset Market Recovery Feed..." << std::endl;
		}
	}

	void Channel::PushPacket( char* str, int size )
	{
		m_initiator->PushPacket( str, size );
	}

	//Incremental Feed Arbitration
	void Channel::spoolIncrementalPacket( Packet& packet )
	{
		m_fChannel << "[spoolIncrementalPacket]: before:" << packet.getSeqNum();
		SpoolPacket spoolPacket(packet.getPacketPointer(), packet.getPacketSize());
		m_fChannel << " after:" << spoolPacket.getSeqNum() << std::endl;
		MAPSpoolPacket::iterator i = m_mapSpoolPacket.find(spoolPacket.getSeqNum());
		if (i == m_mapSpoolPacket.end())
		{
			m_mapSpoolPacket[spoolPacket.getSeqNum()] = spoolPacket;
		}
	}

	void Channel::checkIncrementalSpoolTimer(const int sock)
	{
		while (!m_mapSpoolPacket.empty())
		{
			MAPSpoolPacket::iterator i = m_mapSpoolPacket.begin();
			SpoolPacket& packet = i->second;
			unsigned seqNum = packet.getSeqNum();
			// If the current packet sequence number is larger than expected,
			// there's a gap
			if (seqNum > m_IncrementalNextSeqNum)
			{
				m_fChannel << "[checkRealTimeSpoolTimer]: still have a gap, current SeqNum=" << seqNum << ", need SeqNum=" << m_IncrementalNextSeqNum << std::endl;
				/*
				//time limit hasn't been reached, so it's still not an unrecoverable gap. Return and wait..
				//No retransmission request sent for this message before
				if ( !packet.isRetransRequested() ) 
				{
				if (m_initiator->GenerateRetransRequest( m_ChannelID, m_RealTimeNextSeqNum, seqNum - 1))
				{
				packet.setRetransRequested( true );
				}
				}
				*/
				//The RetransRequest failed or took too long and the gap wasn't
				//filled by the other feed - The packets have been permanently
				//missed. Recover the lost data from Market Recovery Server.
				if (seqNum - m_IncrementalNextSeqNum >= 5 || packet.getTimeLimit() >= m_poolTimeLimit) 
				{
					m_fChannel << "[checkRealTimeSpoolTimer]: The packets have been permanently missed." << std::endl;

					//�Ȳ����պ�ԼҲ���ڻָ�����
					if (!m_bOnInstrumentDef && !m_bOnMarketRecovery)
					{
						//��Լֻ��һ��
						if (m_InstDefComplete)//�Ѿ��չ���Լ
							subscribeMarketRecovery();//ֱ�Ӷ��Ŀ���
						else
							subscribeInstrumentDef();//�ȶ��ĺ�Լ
					}
					else
					{
						m_fChannel << "[checkRealTimeSpoolTimer]: Already Started Market Recovery." << std::endl;
					}
				}
				return;
			}
			else if (seqNum == m_IncrementalNextSeqNum)
			{
				// The packet contains the next expected sequence number, so process it
				PushPacket(packet.getPacketPointer(), packet.getPacketSize());
				increaseIncrementalNextSeqNum();
				m_mapSpoolPacket.erase(i);
				m_fChannel << "[checkRealTimeSpoolTimer]: push spooled packet, num:" << seqNum << std::endl;
			}
			else
			{
				//�Ѿ���������ӻ�����ɾ��
				m_fChannel << "[checkRealTimeSpoolTimer]: already processed packet, num:" << seqNum << " i->first:" << i->first << std::endl;
				m_mapSpoolPacket.erase(i);//�������
			}
		}
	}

	void Channel::onPushInstrumentDefPacket( Packet& packet, const int sock )
	{
		//Packet Header
		//	--Packet Sequence Num 4Bytes
		//	--Sending Time (in nanoseconds) 8Bytes
		char* pBuf = packet.getPacketPointer() + 12;
		int len = packet.getPacketSize() - 12;
		UINT16 uMsgSize;
		Listener listener;
		while ( len > 0 )
		{
			//Message Header
			//	--Message Size 2Bytes
			uMsgSize = *(UINT16* )pBuf;
			pBuf += 2;
			len -= 2;
			CarCallbacks carCbs(listener);
			EnterCriticalSection(&m_initiator->m_csirRepo);
			listener.dispatchMessageByHeader(m_initiator->m_irRepo.header(), &m_initiator->m_irRepo)
				.resetForDecode(pBuf , len)
				.subscribe(&carCbs, &carCbs, &carCbs);
			LeaveCriticalSection(&m_initiator->m_csirRepo);
			if (carCbs.getStatus())
			{
				++m_InstrumentDefProcessedNum;
				//�����ɹ�
				FieldMap& fieldMap = carCbs.getFieldMapRef();
				Field* pField = NULL;

				//911-TotNumReports
				if ( pField = fieldMap.getField(911) )
				{
					if (m_InstrumentDefProcessedNum == pField->getUInt() )
					{
						m_fChannel << "[onPushInstrumentDefPacket]: Completed, received " << m_InstrumentDefProcessedNum << " messages." << std::endl;
						unsubscribe(sock);
						m_fChannel << "[onPushInstrumentDefPacket]: unsubscribe InstrumentDef" << std::endl;
						m_bOnInstrumentDef = false;
						m_InstDefComplete = true;
						//�����Լ�󣬶��Ŀ���
						subscribeMarketRecovery();
					}
					else
					{
						m_fChannel << "[onPushInstrumentDefPacket]: received " << m_InstrumentDefProcessedNum << " messages."  << " Total: " << pField->getUInt() << std::endl;
					}
				}
			}
			pBuf += uMsgSize-2;
			len -= uMsgSize-2;
		}
	}

	//SnapShotֹͣ����Ҫ��ȡʵʱ������һ��Ҫ����İ����
	void Channel::onPushMarketRecoveryPacket( Packet& packet, const int sock )
	{
		//Packet Header
		//	--Packet Sequence Num 4Bytes
		//	--Sending Time (in nanoseconds) 8Bytes
		char* pBuf = packet.getPacketPointer() + 12;
		int len = packet.getPacketSize() - 12;
		UINT16 uMsgSize;
		Listener listener;
		while ( len > 0 )
		{
			//Message Header
			//	--Message Size 2Bytes
			uMsgSize = *(UINT16* )pBuf;
			pBuf += 2;
			len -= 2;
			CarCallbacks carCbs(listener);
			EnterCriticalSection(&m_initiator->m_csirRepo);
			listener.dispatchMessageByHeader(m_initiator->m_irRepo.header(), &m_initiator->m_irRepo)
				.resetForDecode(pBuf , len)
				.subscribe(&carCbs, &carCbs, &carCbs);
			LeaveCriticalSection(&m_initiator->m_csirRepo);
			if (carCbs.getStatus())
			{
				//�����ɹ�
				++m_MarketRecoveryProcessedNum;
				FieldMap& fieldMap = carCbs.getFieldMapRef();
				Field* pField = NULL;

				//ȡһ��ѭ���ĵ�һ�����е�LastMsgSeqNumProcessed������ͬ��Incremental Feed
				if ( pField = fieldMap.getField(369) )
				{
					if (packet.getSeqNum() == 1)
					{
						m_LastMsgSeqNumProcessed = pField->getUInt();
					}
				}

				//TotNumReport
				if ( pField = fieldMap.getField(911) )
				{
					if (m_MarketRecoveryProcessedNum == pField->getUInt())
					{
						m_IncrementalNextSeqNum = m_LastMsgSeqNumProcessed + 1;
						m_fChannel << "[processSnapShotPacket]SnapShot Completed, received " << m_MarketRecoveryProcessedNum << " messages, m_RealTimeNextSeqNum : " << m_IncrementalNextSeqNum << std::endl;
						unsubscribe(sock);
						m_bOnMarketRecovery = false;
					}
				}
			}
			pBuf += uMsgSize-2;
			len -= uMsgSize-2;
		}
	}

	void Channel::subscribeIncremental()
	{
		if ( m_initiator->Subscribe( this, INCREMENTAL_A) )
		{
			m_fChannel << "Subscribed Incremental A" << std::endl;
		}

		if ( m_initiator->Subscribe( this, INCREMENTAL_B) )
		{
			m_fChannel << "Subscribed Incremental B" << std::endl;
		}
	}

	void Channel::subscribeMarketRecovery()
	{
		if ( m_initiator->Subscribe(this, SNAPSHOT_A) )
		{
			m_bOnMarketRecovery = true;
			m_fChannel << "Subscribed Market Recovery A" << std::endl;
		}
		else
		{
			if (m_initiator->Subscribe(this, SNAPSHOT_B))//���õ�ַ
			{
				m_bOnMarketRecovery = true;
				m_fChannel << "Subscribed Market Recovery B" << std::endl;
			}
		}
	}

	void Channel::subscribeInstrumentDef()
	{
		if ( m_initiator->Subscribe(this, INSTRUMENT_RPLAY_A) )
		{
			m_bOnInstrumentDef = true;
			m_fChannel << "Subscribed Instrument Definition A" << std::endl;
		}
		else
		{
			if (m_initiator->Subscribe(this, INSTRUMENT_RPLAY_B))//���õ�ַ
			{
				m_bOnInstrumentDef = true;
				m_fChannel << "Subscribed Instrument Definition B" << std::endl;
			}
		}
	}

	void Channel::unsubscribe(const int sock)
	{
		shutdown(sock, SD_BOTH);
		closesocket(sock);
		m_setSockets.erase(sock);
	}

	void Channel::unsubscribeAll()
	{
		while (!m_setSockets.empty())
		{
			Sockets::iterator i = m_setSockets.begin();
			unsubscribe(*i);
		}
	}

	void Channel::resetInstrumentDef()
	{
		m_InstrumentDefNextSeqNum = 1;
		m_InstrumentDefProcessedNum = 0;
	}

	void Channel::resetMarketRecovery()
	{
		m_MarketRecoveryNextSeqNum = 1;
		m_MarketRecoveryProcessedNum = 0;
	}

	void Channel::clearIncrementalSpool()
	{
		MAPSpoolPacket::iterator i = m_mapSpoolPacket.begin();
		for ( ; i != m_mapSpoolPacket.end(); i++ )
		{
			m_mapSpoolPacket.erase(i);
		}
	}
}

