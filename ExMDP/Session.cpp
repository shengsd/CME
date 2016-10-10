#include "stdafx.h"
#include "Session.h"
//#define USEQUICKFIX

#ifdef USEQUICKFIX
#pragma comment(lib, "quickfix.lib")
#include "include/quickfix/Message.h"
#endif

namespace MDP
{
	Session::Session(Channel* pChannel, 
					 int s, Connector* pConnector, 
					 std::string& username, 
					 std::string& password, 
					 int beginSeqNum, 
					 int endSeqNum)
	:m_pChannel(pChannel), m_socket(s),
	 m_username(username), m_password(password),
	 m_beginSeqNum(beginSeqNum), m_endSeqNum(endSeqNum),
	 m_sentLogout( false ), m_sentLogon( false ),
	 m_enabled( true ), m_receivedLogon( false ),
	 m_logonTimeout( 10 ), m_logoutTimeout( 2 ),
	 m_connector(pConnector), m_sentRequest(false),
	 m_sendLength(0)
	{
		m_channelID = pChannel->getChannelID();
		m_lastReceivedTime = time(NULL);
		FD_ZERO(&m_fds);
		FD_SET(m_socket, &m_fds);
	}


	Session::~Session(void)
	{
	}

	bool Session::read( Connector& connector)
	{
		memset(m_buffer, 0, sizeof(m_buffer));
		m_len = recv(m_socket, m_buffer, sizeof(m_buffer), 0);
		if (m_len <= 0)
			return false;
		else
			return true;
		//TODO:识别消息类型，处理系统消息，转发应用消息
		//m_pChannel->onData(m_buffer, m_len);
	}

	void Session::next()
	{
		//没收到登录确认
		if ( !receivedLogon() )
		{
			//没发送过登录请求
			if ( !sentLogon())
			{
				generateLogon();
				m_pChannel->onEvent( "Initiated logon request" );
			}
			else if ( logonTimedOut() )
			{
				m_pChannel->onEvent( "Timed out waiting for logon response" );
				disconnect();
			}
			return ;
		}
		//没发送过请求
		else if ( !sentRequest())
		{
			GenerateResendRequest();
		}
		//一定时间内没收到消息，会话超时
		if ( timedOut() )
		{
			m_pChannel->onEvent( "Timed out waiting for message" );
			disconnect();
		}
	}

	void Session::next( const std::string& str)
	{
		
	}

	void Session::generateLogon()
	{
#ifdef QUICKFIX
		FIX::Message logon;
		logon.setField( FIX::MsgType( "A" ) );
		logon.setField( FIX::Username( m_username ) );
		logon.setField( FIX::Password( m_password ) );

		UtcTimeStamp now;
		lastReceivedTime( now );
		sentLogon( true );

		std::string messageString;
		logon.toString( messageString );
		send( messageString );
#else
		std::string strLogon;

#endif
		
	}

	void Session::GenerateResendRequest()
	{
#ifdef QUICKFIX
		FIX::Message resendRequest;
		resendRequest.setField( FIX::MsgType( "V" ) );
		std::string s;
		std::stringstream ss;
		ss << m_channelID;
		ss >> s;
		resendRequest.setField( FIX::ApplID( s ) );
		UtcTimeStamp now;
		lastReceivedTime( now );
		ss << now.getSecond();
		ss >> s;
		resendRequest.setField( FIX::MDReqID( s ) );
		resendRequest.setField( FIX::ApplBegSeqNum( m_beginSeqNum ) );
		resendRequest.setField( FIX::ApplEndSeqNum( m_endSeqNum ) );

		std::string messageString;
		resendRequest.toString(messageString);
		sentRequest( true );
		send(messageString);	
#else

#endif
	}

	void Session::generateLogout()
	{
#ifdef QUICKFIX
		FIX::Message logout;
		logout.setField( FIX::MsgType( "5" ) );
		logout.setField( FIX::Text( " " ) );
		UtcTimeStamp now;
		lastReceivedTime(now);
		std::string messageString;
		logout.toString(messageString);
		sentLogout(true);
		send(messageString);
#else

#endif
	}

	bool Session::send( std::string& str )
	{
		Locker l( m_mutex );
		m_sendQueue.push_back( str );
		processQueue();
		//signal();
		return true;
	}

	bool Session::processQueue()
	{
		Locker l( m_mutex );

		if( !m_sendQueue.size() ) return true;

		struct timeval timeout = { 0, 0 };
		fd_set writeset = m_fds;
		if( select( 1 + m_socket, 0, &writeset, 0, &timeout ) <= 0 )
			return false;

		const std::string& msg = m_sendQueue.front();

		size_t result = socket_send( m_socket, msg.c_str() + m_sendLength, msg.length() - m_sendLength );

		if( result > 0 )
			m_sendLength += result;

		if( m_sendLength == msg.length() )
		{
			m_sendLength = 0;
			m_sendQueue.pop_front();
		}

		return !m_sendQueue.size();
	}



	void Session::disconnect()
	{
		m_pChannel->onEvent( "Disconnecting" );
		m_connector->drop(m_socket);
	}

}
