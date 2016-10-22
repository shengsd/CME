#include "stdafx.h"
#include "Channel.h"
#include <direct.h>

namespace MDP
{
	Channel::Channel( const std::string& channelID, Initiator* pInitiator)
	:m_ChannelID( channelID ),
	 m_initiator( pInitiator ),
	 m_poolTimeLimit( 10 ),
	 m_RealTimeNextSeqNum( 1 ),
	 m_InstDefNextSeqNum( 1 ),
	 m_SnapShotNextSeqNum( 1 ),
	 m_LastMsgSeqNumProcessed( 1 ),
	 m_bOnInstrumentDefinition( false ),
	 m_bOnMarketRecovery( false ),
	 m_InstDefProcessedNum( 0 ),
	 m_SnapShotProcessedNum( 0 )
	{
		char szModulePath[MAX_PATH] = {0};
		GetModuleFileName(NULL, szModulePath, MAX_PATH);
		char* pExeDir = strrchr(szModulePath, '\\');
		*pExeDir = 0;
		std::string s = szModulePath;
		s.append("\\CME\\"+channelID);
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
		m_fEvents << value << std::endl;
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
		//UtcTimeStamp now;
		//setLastReceivedTime( now );

		//Packet packet(m_buffer, m_len);

		switch (connectType)
		{
		case 1://Incremental feed

//#ifdef _DEBUG
 			m_fEvents << "  Incremental feed, sequence:" << packet.getSeqNum() << std::endl;
//  			m_fPackets << "Incremental feed, sequence:" << packet.getSeqNum() << std::endl;
//  			m_fPackets.write(packet.getPacketPointer(), packet.getPacketSize());
//  			m_fPackets << std::endl;
//#endif
			//for test
			//pushPacket(packet, true);
			processRealTimePacket(packet);
			break;
		case 2://Instrument Definition feed

// #ifdef _DEBUG
  			m_fEvents << "   Instrument Definition feed, sequence:" << packet.getSeqNum() << std::endl;
//  			m_fPackets << "Instrument Definition feed, sequence:" << packet.getSeqNum() << std::endl;
//  			m_fPackets.write(packet.getPacketPointer(), packet.getPacketSize());
//  			m_fPackets << std::endl;
// #endif
			//pushPacket(packet, true);
			processInstDefPacket(packet, sock);
			break;
		case 3://Snapshot Recovery feed

// #ifdef _DEBUG
  			m_fEvents << "   SnapShot feed, sequence:" << packet.getSeqNum() << std::endl;
//  			m_fPackets << "SnapShot feed, sequence:" << packet.getSeqNum() << std::endl;
//  			m_fPackets.write(packet.getPacketPointer(), packet.getPacketSize());
//  			m_fPackets << std::endl;
// #endif // _DEBUG
			processSnapShotPacket(packet, sock);
			break;
		default:
			return false;
		}
		return true;
	}

	void Channel::processRealTimePacket(Packet& packet)
	{
		if (packet.getSeqNum() == m_RealTimeNextSeqNum)
		{
			PushPacket(packet);
			increaseRealTimeNextSeqNum();
			m_fEvents << "Action: Push this packet\n";
		}
		else if (packet.getSeqNum() > m_RealTimeNextSeqNum)
		{
			spoolRealTimePacket(packet);
			m_fEvents << "Action: Spool this packet\n";
		}
		else
		{
			//discard duplicate packet
			m_fEvents << "Action: Discard this packet\n";
		}
		checkRealTimeSpoolTimer();
	}

	//文档中建议开始条件是包序号为1，结束条件是处理消息数达到Tag 911-TotNumReports
	void Channel::processInstDefPacket(Packet& packet, int sock)
	{
		//Recovery状态中才处理
		if ( !isOnInstrumentDefinition() )
		{
			m_fEvents << "the channel is not on Instrument Recovery, ignore this packet" << std::endl;
			return;
		}

		if (packet.getSeqNum() == m_InstDefNextSeqNum)
		{
			onPushInstDefPacket(packet, sock);
			//pushPacket(packet, true);
			PushPacket(packet);
			increaseInstDefNextSeqNum();
			m_fEvents << "Action: Push this packet\n";
		}
		else if (packet.getSeqNum() > m_InstDefNextSeqNum)
		{
			spoolInstDefPacket(packet);
			m_fEvents << "Action: Spool this packet\n";
		}
		else
		{
			//discard packet
			m_fEvents << "Action: Discard this packet\n";
		}

		checkInstDefSpoolTimer(sock);

		if (!isOnInstrumentDefinition())
		{
			resetInstrumentDefinition();
			m_fEvents << "Instrument Recovery completed, reset Instrument Recovery Feed..." << std::endl;
		}
	}

	void Channel::processSnapShotPacket( Packet& packet, int sock )
	{
		//不在恢复状态中？
		if (!isOnMarketRecovery())
		{
			m_fEvents << "the channel is not on market recovery, ignore this packet" << std::endl;
			return;
		}

		//合约定义没收完？
// 		if (!isInsDefComplete())
// 		{
// 			m_fEvents << "the Inst def is not completed, ignore this packet" << std::endl;
// 			return;
// 		}

		//从1号包开始依次处理
		if (packet.getSeqNum() == m_SnapShotNextSeqNum)
		{
			onPushSnapShotPacket( packet, sock );
			//pushPacket( packet, true );
			PushPacket(packet);
			increaseSnapShotNextSeqNum();
			m_fEvents << "Action: Push this packet\n";
		}
		else if ( packet.getSeqNum() > m_SnapShotNextSeqNum )
		{
			spoolSnapShotPacket(packet);
			m_fEvents << "Action: Spool this packet\n";
		}
		else
		{
			//discard packet
			m_fEvents << "Action: Discard this packet\n";
		}
		checkSnapShotSpoolTimer(sock);
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

	/*
	void Channel::pushPacket(Packet& packet, bool copy)
	{
		if (copy)
		{
			char* p = new char[packet.getPacketSize()];
			memcpy(p, packet.getPacketPointer(), packet.getPacketSize());
			packet.setPacket(p);
		}
		m_packetQueue.push(packet);

	}
	*/

	//Incremental Feed Arbitration
	void Channel::spoolRealTimePacket( Packet& packet )
	{
		PacketSpool::iterator i = m_RealTimePacketSpool.find(packet.getSeqNum());
		if (i == m_RealTimePacketSpool.end())
		{
// 			char* p = new char[packet.getPacketSize()];
// 			memcpy(p, packet.getPacketPointer(), packet.getPacketSize());
// 			packet.setPacket(p);
			m_RealTimePacketSpool[packet.getSeqNum()] = packet;
		}
	}

	void Channel::spoolInstDefPacket( Packet& packet )
	{
		PacketSpool::iterator i = m_InstDefPacketSpool.find(packet.getSeqNum());
		if (i == m_InstDefPacketSpool.end())
		{
// 			char* p = new char[packet.getPacketSize()];
// 			memcpy(p, packet.getPacketPointer(), packet.getPacketSize());
// 			packet.setPacket(p);
			m_InstDefPacketSpool[packet.getSeqNum()] = packet;
		}
	}

	void Channel::spoolSnapShotPacket( Packet& packet )
	{
		PacketSpool::iterator i = m_SnapShotPacketSpool.find(packet.getSeqNum());
		if (i == m_SnapShotPacketSpool.end())
		{
// 			char* p = new char[packet.getPacketSize()];
// 			memcpy(p, packet.getPacketPointer(), packet.getPacketSize());
// 			packet.setPacket(p);
			m_SnapShotPacketSpool[packet.getSeqNum()] = packet;
		}
	}

	void Channel::checkRealTimeSpoolTimer()
	{
		while (!m_RealTimePacketSpool.empty())
		{
			PacketSpool::iterator i = m_RealTimePacketSpool.begin();
			Packet& packet = i->second;
			unsigned seqNum = packet.getSeqNum();
			// If the current packet sequence number is larger than expected,
			// there's a gap
			if (seqNum > m_RealTimeNextSeqNum)
			{
				m_fEvents << "[checkRealTimeSpoolTimer]: still have a gap, cur num:" << seqNum << ", need num:" << m_RealTimeNextSeqNum << std::endl;
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
				if (seqNum - m_RealTimeNextSeqNum >= 5 || packet.getTimeLimit() > m_poolTimeLimit) 
				{
					m_fEvents << "[checkRealTimeSpoolTimer]: cur seqNum = " << seqNum <<", RealNextSeqNum = " <<m_RealTimeNextSeqNum << std::endl;
					m_fEvents << "[checkRealTimeSpoolTimer]: " << packet.getTimeLimit() << " seconds has passed" << std::endl;
					
					if (!isOnMarketRecovery())
					{
						subscribeMarketRecovery();
					}
					else
					{
						m_fEvents << "[checkRealTimeSpoolTimer]: already started recovery" << std::endl;
					}
				}
				return;
				
			}
			else if (seqNum == m_RealTimeNextSeqNum)
			{
				// The packet contains the next expected sequence number, so process it
				PushPacket(packet);
				increaseRealTimeNextSeqNum();
				m_RealTimePacketSpool.erase(i);
				m_fEvents << "[checkRealTimeSpoolTimer]: push spooled packet, num:" << seqNum << std::endl;
			}
			else
			{
				//已经处理过，从缓存中删除
				m_fEvents << "[checkRealTimeSpoolTimer]: already processed packet, num:" << seqNum << std::endl;
				m_RealTimePacketSpool.erase(i);
			}
		}
	}

	void Channel::checkInstDefSpoolTimer(int sock)
	{
		while (!m_InstDefPacketSpool.empty())
		{
			PacketSpool::iterator i = m_InstDefPacketSpool.begin();
			Packet& packet = i->second;
			unsigned seqNum = packet.getSeqNum();
			if (seqNum > m_InstDefNextSeqNum)
			{
				//缓存数据还不能处理
				m_fEvents << "[checkInstDefSpoolTimer]: still have a gap, cur num:" << seqNum << ", need num:" << m_InstDefNextSeqNum << std::endl;
				return;
			}
			else if (seqNum == m_InstDefNextSeqNum)
			{
				// The packet contains the next expected sequence number, so
				//process it
				increaseInstDefNextSeqNum();
				onPushInstDefPacket(packet, sock);
				//pushPacket(packet, false);
				PushPacket(packet);
				m_InstDefPacketSpool.erase(i);
				m_fEvents << "[checkInstDefSpoolTimer]: push spooled packet, num:" << seqNum << std::endl;
			}
			else
			{
				//已经处理过，从缓存中删除
				//delete[] packet.getPacketPointer();
				m_fEvents << "[checkInstDefSpoolTimer]: already processed packet, num:" << seqNum << std::endl;
				m_InstDefPacketSpool.erase(i);
			}
		}
	}

	void Channel::checkSnapShotSpoolTimer(int sock)
	{
		while (!m_SnapShotPacketSpool.empty())
		{
			PacketSpool::iterator i = m_SnapShotPacketSpool.begin();
			Packet& packet = i->second;
			unsigned seqNum = packet.getSeqNum();
			if (seqNum > m_SnapShotNextSeqNum)
			{
				m_fEvents << "[checkSnapShotSpoolTimer]: still have a gap, cur num:" << seqNum << ", need num:" << m_SnapShotNextSeqNum << std::endl;
				return;
			}
			else if (seqNum == m_SnapShotNextSeqNum)
			{
				onPushSnapShotPacket(packet, sock);
				//pushPacket(packet, false);
				PushPacket(packet);
				increaseSnapShotNextSeqNum();
				m_SnapShotPacketSpool.erase(i);
				m_fEvents << "[checkSnapShotSpoolTimer]: push spooled packet, num:" << seqNum << std::endl;
			}
			else
			{
				//已经处理过，从缓存中删除
				//delete[] packet.getPacketPointer();
				m_SnapShotPacketSpool.erase(i);
				m_fEvents << "[checkSnapShotSpoolTimer]: already processed packet, num:" << seqNum << std::endl;
			}
		}
	}


	void Channel::onPushInstDefPacket( Packet& packet, int sock )
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
			CarCallbacks carCbs(listener, NULL);
			listener.dispatchMessageByHeader(m_initiator->m_irRepo.header(), &m_initiator->m_irRepo)
				.resetForDecode(pBuf , len)
				.subscribe(&carCbs, &carCbs, &carCbs);

			if (carCbs.getStatus())
			{
				++m_InstDefProcessedNum;
				//解析成功
				FieldMap& fieldMap = carCbs.getFieldMapRef();
				Field* pField = NULL;

				//911-TotNumReports
				if ( pField = fieldMap.getField(911) )
				{
					if (m_InstDefProcessedNum == pField->getUInt() )
					{
						m_fEvents << "[processInstDefPacket]InstDefCompleted, received " << m_InstDefProcessedNum << " messages." << std::endl;
						//setInstDefComplete(true);
						unsubscribe(sock);
						setOnInstrumentDefinition(false);
						//收完合约后，订阅实时行情和快照
						subscribeIncremental();
						subscribeMarketRecovery();
					}
				}
			}
			pBuf += uMsgSize-2;
			len -= uMsgSize-2;
		}

	}

	//SnapShot停止后需要获取实时行情下一个要处理的包序号
	void Channel::onPushSnapShotPacket( Packet& packet, int sock )
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
			CarCallbacks carCbs(listener, NULL);
			listener.dispatchMessageByHeader(m_initiator->m_irRepo.header(), &m_initiator->m_irRepo)
				.resetForDecode(pBuf , len)
				.subscribe(&carCbs, &carCbs, &carCbs);

			if (carCbs.getStatus())
			{
				//解析成功
				++m_SnapShotProcessedNum;
				FieldMap& fieldMap = carCbs.getFieldMapRef();
				Field* pField = NULL;

				if ( pField = fieldMap.getField(369) )
				{
					m_LastMsgSeqNumProcessed = pField->getUInt();
				}

				//TotNumReport
				if ( pField = fieldMap.getField(911) )
				{
					if (m_SnapShotProcessedNum == pField->getUInt())
					{
						m_RealTimeNextSeqNum = m_LastMsgSeqNumProcessed + 1;
//#ifdef _DEBUG
						m_fEvents << "[processSnapShotPacket]SnapShot Completed, received " << m_SnapShotProcessedNum << " messages, m_RealTimeNextSeqNum : " << m_RealTimeNextSeqNum << std::endl;
//#endif
						unsubscribe(sock);
						setOnMarketRecovery(false);
						//setInstDefComplete(false);
						//subscribeIncremental();
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

	void Channel::subscribeInstrumentDefinition()
	{
		if ( m_initiator->SubscribeInstrumentDefinition( m_ChannelID ) )
		{
			setOnInstrumentDefinition( true );
			m_fEvents << "subscribe Instrument Definition!" << std::endl;
		}
	}

	void Channel::unsubscribe(int socket)
	{
		m_initiator->Unsubscribe(socket);
		m_fEvents << "Channel ID:"<< m_ChannelID << ", unsubscribe feed, socket:" << socket << std::endl;
	}

/*
	void Channel::clearPacketQueue()
	{
		int num = m_packetQueue.size();
		while (num)
		{
			Packet& packet = m_packetQueue.front();
			char* pBuf = packet.getPacketPointer();
			if (pBuf != NULL)
			{
				//std::cout << "test2" << std::endl;
				delete[] pBuf;
				pBuf = NULL;
			}
			m_packetQueue.pop();
			--num;
		}
	}
*/
	void Channel::resetInstrumentDefinition()
	{
		m_InstDefNextSeqNum = 1;
		m_InstDefProcessedNum = 0;
		clearInstDefSpool();
	}

	void Channel::resetMarketRecovery()
	{
		m_SnapShotNextSeqNum = 1;
		m_SnapShotProcessedNum = 0;
		clearSnapShotSpool();
	}


	void Channel::clearRealTimeSpool()
	{
		PacketSpool::iterator i = m_RealTimePacketSpool.begin();
		for ( ; i != m_RealTimePacketSpool.end(); i++ )
		{
// 			Packet& packet = i->second;
// 			char* p = packet.getPacketPointer();
// 			if (p != NULL)
// 			{
// 				//std::cout << "test3" << std::endl;
// 				delete[] p;
// 				p = NULL;
// 			}
			m_RealTimePacketSpool.erase(i);
		}
	}
	
	void Channel::clearInstDefSpool()
	{
		PacketSpool::iterator i = m_InstDefPacketSpool.begin();
		for ( ; i != m_InstDefPacketSpool.end(); i++ )
		{
// 			Packet& packet = i->second;
// 			char* p = packet.getPacketPointer();
// 			if (p != NULL)
// 			{
// 				delete[] p;
// 				p = NULL;
// 			}
			m_InstDefPacketSpool.erase(i);
		}
	}

	void Channel::clearSnapShotSpool()
	{
		PacketSpool::iterator i = m_SnapShotPacketSpool.begin();
		for ( ; i != m_SnapShotPacketSpool.end(); i++ )
		{
// 			Packet& packet = i->second;
// 			char* p = packet.getPacketPointer();
// 			if (p != NULL)
// 			{
// 				delete[] p;
// 				p = NULL;
// 			}
			m_SnapShotPacketSpool.erase(i);
		}
	}




}

