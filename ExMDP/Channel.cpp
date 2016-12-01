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
	 m_InstrumentDefProcessedNum( 0 ),
	 m_MarketRecoveryProcessedNum( 0 )
	{
		char szModulePath[MAX_PATH] = {0};
		GetModuleFileName(NULL, szModulePath, MAX_PATH);
		char* pExeDir = strrchr(szModulePath, '\\');
		*pExeDir = '\0';
		std::string s = szModulePath;
		s.append("\\Log\\"+channelID);
		_mkdir(s.c_str());
		m_fPackets.open( s + "\\packets.log", std::ios::app );
		m_fEvents.open(s +"\\events.log", std::ios::app );
	}

	Channel::~Channel(void)
	{
		m_fPackets.close();
		m_fEvents.close();
	}

	void Channel::onEvent( const std::string& value )
	{
		//m_fEvents << value << std::endl;
	}

	void Channel::onData( const char* value, int size)
	{
		m_fPackets << "TCP:";
		m_fPackets.write(value, size);
		m_fPackets << std::endl;
	}
	
	bool Channel::read( int sock, int connectType )
	{
		//read packet from socket
		m_fEvents << "[Channel::read]: sock:" << sock << " connectType:" << connectType ;
		Packet packet;
		
		int size = sizeof(struct sockaddr);
		struct sockaddr_in local_addr;

		// Read the data on the socket
		int nLen = recvfrom(sock, packet.getPacketPointer(), MAXPACKETSIZE, 0, (struct sockaddr *) &local_addr, &size);
		if (nLen < 0)
			return false;

		packet.setPacketSize(nLen);

		switch (connectType)
		{
		case 1://Incremental feed
#ifdef _DEBUG
			m_fEvents << "  Incremental feed, sequence:" << packet.getSeqNum() << std::endl;
			m_fPackets << "Incremental feed, sequence:" << packet.getSeqNum() << std::endl;
			m_fPackets.write(packet.getPacketPointer(), packet.getPacketSize());
			m_fPackets << std::endl;
#endif
			//for test
			//pushPacket(packet, true);
			processIncrementalPacket(packet);
			break;
		case 2://Instrument Definition feed

#ifdef _DEBUG
  			m_fEvents << "   Instrument Definition feed, sequence:" << packet.getSeqNum() << std::endl;
  			m_fPackets << "Instrument Definition feed, sequence:" << packet.getSeqNum() << std::endl;
  			m_fPackets.write(packet.getPacketPointer(), packet.getPacketSize());
  			m_fPackets << std::endl;
#endif
			//pushPacket(packet, true);
			processInstrumentDefPacket(packet, sock);
			break;
		case 3://Market Recovery feed

#ifdef _DEBUG
  			m_fEvents << "   SnapShot feed, sequence:" << packet.getSeqNum() << std::endl;
  			m_fPackets << "SnapShot feed, sequence:" << packet.getSeqNum() << std::endl;
  			m_fPackets.write(packet.getPacketPointer(), packet.getPacketSize());
  			m_fPackets << std::endl;
#endif // _DEBUG
			processMarketRecoveryPacket(packet, sock);
			break;
		default:
			return false;
		}
		return true;
	}

	void Channel::processIncrementalPacket(Packet& packet)
	{
		if (packet.getSeqNum() == m_IncrementalNextSeqNum)
		{
			PushPacket(packet);
			increaseIncrementalNextSeqNum();
			m_fEvents << "Action: Push this packet\n";
		}
		else if (packet.getSeqNum() > m_IncrementalNextSeqNum)
		{
			spoolIncrementalPacket(packet);
			m_fEvents << "Action: Spool this packet\n";
		}
		else
		{
			//discard duplicate packet
			m_fEvents << "Action: Discard this packet\n";
		}
		checkIncrementalSpoolTimer();
	}

	//文档中建议开始条件是包序号为1，结束条件是处理消息数达到Tag 911-TotNumReports
	void Channel::processInstrumentDefPacket(Packet& packet, int sock)
	{
		//Recovery状态中才处理
		if ( !isOnInstrumentDef() )
		{
			m_fEvents << "the channel is not on Instrument Recovery, ignore this packet" << std::endl;
			return;
		}
		//需要连续处理
		if (packet.getSeqNum() == m_InstrumentDefNextSeqNum)
		{
			onPushInstrumentDefPacket(packet, sock);
			PushPacket(packet);
			increaseInstrumentDefNextSeqNum();
			m_fEvents << "Action: Push this packet\n";
		}
		else
		{
			//discard packet and reset
			resetInstrumentDef();
			m_fEvents << "Action: Discard this packet\n";
		}

		if (!isOnInstrumentDef())
		{
			resetInstrumentDef();
			m_fEvents << "Instrument Recovery completed, reset Instrument Recovery Feed..." << std::endl;
		}
	}

	void Channel::processMarketRecoveryPacket( Packet& packet, int sock )
	{
		//不在恢复状态中？
		if (!isOnMarketRecovery())
		{
			m_fEvents << "the channel is not on market recovery, ignore this packet" << std::endl;
			return;
		}

		//从1号包开始依次处理
		if (packet.getSeqNum() == m_MarketRecoveryNextSeqNum)
		{
			onPushMarketRecoveryPacket( packet, sock );
			PushPacket(packet);
			increaseMarketRecoveryNextSeqNum();
			m_fEvents << "Action: Push this packet\n";
		}
		else
		{
			//discard packet and reset
			resetMarketRecovery();
			m_fEvents << "Action: Discard this packet\n";
		}

		if (!isOnMarketRecovery())
		{
			resetMarketRecovery();
			m_fEvents << "Market Recovery completed, reset Market Recovery Feed..." << std::endl;
		}
	}

	void Channel::PushPacket(Packet& packet)
	{
		m_initiator->PushPacket(packet);
	}

	//Incremental Feed Arbitration
	void Channel::spoolIncrementalPacket( Packet& packet )
	{
		PacketSpool::iterator i = m_IncrementalPacketSpool.find(packet.getSeqNum());
		if (i == m_IncrementalPacketSpool.end())
		{
			m_IncrementalPacketSpool[packet.getSeqNum()] = packet;
		}
	}

	void Channel::checkIncrementalSpoolTimer()
	{
		while (!m_IncrementalPacketSpool.empty())
		{
			PacketSpool::iterator i = m_IncrementalPacketSpool.begin();
			Packet& packet = i->second;
			unsigned seqNum = packet.getSeqNum();
			// If the current packet sequence number is larger than expected,
			// there's a gap
			if (seqNum > m_IncrementalNextSeqNum)
			{
				m_fEvents << "[checkRealTimeSpoolTimer]: still have a gap, current num:" << seqNum << ", need num:" << m_IncrementalNextSeqNum << std::endl;
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
					m_fEvents << "[checkRealTimeSpoolTimer]: " << packet.getTimeLimit() << " seconds passed" << std::endl;

					//既不在收合约也不在恢复快照
					if (!isOnInstrumentDef() && !isOnMarketRecovery())
					{
						subscribeInstrumentDef();
					}
					else
					{
						m_fEvents << "[checkRealTimeSpoolTimer]: already started recovery" << std::endl;
					}
				}
				return;
			}
			else if (seqNum == m_IncrementalNextSeqNum)
			{
				// The packet contains the next expected sequence number, so process it
				PushPacket(packet);
				increaseIncrementalNextSeqNum();
				m_IncrementalPacketSpool.erase(i);
				m_fEvents << "[checkRealTimeSpoolTimer]: push spooled packet, num:" << seqNum << std::endl;
			}
			else
			{
				//已经处理过，从缓存中删除
				m_fEvents << "[checkRealTimeSpoolTimer]: already processed packet, num:" << seqNum << std::endl;
				m_IncrementalPacketSpool.erase(i);
			}
		}

		//检查缓存数量，如果快照无法恢复，应该清空数据
		if (m_IncrementalPacketSpool.size() > 100)
		{
			m_IncrementalPacketSpool.clear();
		}
	}

	void Channel::onPushInstrumentDefPacket( Packet& packet, int sock )
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
			listener.dispatchMessageByHeader(m_initiator->m_irRepo.header(), &m_initiator->m_irRepo)
				.resetForDecode(pBuf , len)
				.subscribe(&carCbs, &carCbs, &carCbs);

			if (carCbs.getStatus())
			{
				++m_InstrumentDefProcessedNum;
				//解析成功
				FieldMap& fieldMap = carCbs.getFieldMapRef();
				Field* pField = NULL;

				//911-TotNumReports
				if ( pField = fieldMap.getField(911) )
				{
					if (m_InstrumentDefProcessedNum == pField->getUInt() )
					{
						m_fEvents << "[onPushInstrumentDefPacket]: Completed, received " << m_InstrumentDefProcessedNum << " messages." << std::endl;
						unsubscribe(sock);
						setOnInstrumentDef(false);
						//收完合约后，订阅快照
						subscribeMarketRecovery();
					}
				}
			}
			pBuf += uMsgSize-2;
			len -= uMsgSize-2;
		}
	}

	//SnapShot停止后需要获取实时行情下一个要处理的包序号
	void Channel::onPushMarketRecoveryPacket( Packet& packet, int sock )
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
			listener.dispatchMessageByHeader(m_initiator->m_irRepo.header(), &m_initiator->m_irRepo)
				.resetForDecode(pBuf , len)
				.subscribe(&carCbs, &carCbs, &carCbs);

			if (carCbs.getStatus())
			{
				//解析成功
				++m_MarketRecoveryProcessedNum;
				FieldMap& fieldMap = carCbs.getFieldMapRef();
				Field* pField = NULL;

				//取一次循环的第一个包中的LastMsgSeqNumProcessed，用于同步Incremental Feed
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
						m_fEvents << "[processSnapShotPacket]SnapShot Completed, received " << m_MarketRecoveryProcessedNum << " messages, m_RealTimeNextSeqNum : " << m_IncrementalNextSeqNum << std::endl;
						unsubscribe(sock);
						setOnMarketRecovery(false);
					}
				}
			}
			pBuf += uMsgSize-2;
			len -= uMsgSize-2;
		}
	}

	void Channel::subscribeIncremental()
	{
		if ( m_initiator->SubscribeIncremental( m_ChannelID ) )
		{
			m_fEvents << "subscribe Incremental" << std::endl;
		}
	}

	void Channel::subscribeMarketRecovery()
	{
		if (m_initiator->SubscribeMarketRecovery( m_ChannelID ))
		{
			setOnMarketRecovery(true);
			m_fEvents << "subscribe Market Recovery" << std::endl;
		}
	}

	void Channel::subscribeInstrumentDef()
	{
		if ( m_initiator->SubscribeInstrumentDefinition( m_ChannelID ) )
		{
			setOnInstrumentDef( true );
			m_fEvents << "subscribe Instrument Definition!" << std::endl;
		}
	}

	void Channel::unsubscribe(int socket)
	{
		m_initiator->Unsubscribe(socket);
		m_fEvents << "Channel ID:"<< m_ChannelID << ", unsubscribe feed, socket:" << socket << std::endl;
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
		PacketSpool::iterator i = m_IncrementalPacketSpool.begin();
		for ( ; i != m_IncrementalPacketSpool.end(); i++ )
		{
			m_IncrementalPacketSpool.erase(i);
		}
	}
	
}

