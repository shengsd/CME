#include "stdafx.h"
#include "Connector.h"
#include "Exceptions.h"
#include "ExMDP.h"

namespace MDP
{
	Connector::Connector()
	{
		socket_init();
		m_timeval.tv_sec = 2;
		m_timeval.tv_usec = 0;
		//////////////////////////////////////////////////////////////////////////
		
		//std::pair<int, int> sockets = socket_createpair();
		//m_signal = sockets.first;
		//m_interrupt = sockets.second;
		//socket_setnonblock( m_signal );
		//socket_setnonblock( m_interrupt );
		//m_readSockets.insert( m_interrupt );

		//////////////////////////////////////////////////////////////////////////
		//m_timeval.tv_sec = 0;
		//m_timeval.tv_usec = 0;
// #ifndef SELECT_DECREMENTS_TIME
// 		m_ticks = clock();
// #endif
	}

	Connector::~Connector()
	{
		Sockets::iterator i;
		for ( i = m_readSockets.begin(); i != m_readSockets.end(); ++i ) {
			socket_close( *i );
		}

		//socket_close( m_signal );
		socket_term();
	}

// 	void Connector::setLogFile()
// 	{
// 
// 	}

	int Connector::UDPconnect(const std::string& address, const int port, const std::string& localInterface)
	{
		//m_fLog << "Connect to "<< address << ":" << port << std::endl;
		//g_lpfnWriteLog(LOG_DEBUG, "Connect to %s:%d\n", address.c_str(), port);
		struct sockaddr_in local;
		struct ip_mreq imreq;
		int iResult = 0;
		BOOL bOptVal = TRUE;
		int bOptLen = sizeof( BOOL );
		int ListenSocket = socket(AF_INET, SOCK_DGRAM, 0 );
		if ( ListenSocket == SOCKET_ERROR )
		{
			//g_lpfnWriteLog(LOG_DEBUG, "[UDPconnect]: UDPconnect socket error\n" );
			return -1;
		}

		iResult = setsockopt( ListenSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&bOptVal, bOptLen );
		if ( iResult == SOCKET_ERROR )
		{
			//g_lpfnWriteLog(LOG_DEBUG, "[UDPconnect]: set sock reuse addr error, Error code:%d\n", WSAGetLastError() );
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
			//g_lpfnWriteLog(LOG_DEBUG, "[UDPconnect]: UDPconnect bind error, Error code:%d\n", WSAGetLastError() );
			return -1;
		}
		//加入多播组
		imreq.imr_interface.s_addr = htonl(INADDR_ANY);
		//imreq.imr_interface.s_addr = inet_addr("10.249.43.131");//local interface
		//imreq.imr_interface.s_addr = inet_addr("172.17.120.92");
		//imreq.imr_interface.s_addr = inet_addr(localInterface.c_str());
		imreq.imr_multiaddr.s_addr = inet_addr(address.c_str());

		iResult = setsockopt( ListenSocket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (const char *)&imreq, sizeof(imreq));
		if ( iResult == SOCKET_ERROR )
		{
			//g_lpfnWriteLog(LOG_DEBUG, "[UDPconnect]: setsockopt error, Error code:%d\n", WSAGetLastError() );
			return -1;
		}

		addRead( ListenSocket );
		//addConnect( ListenSocket );
		return ListenSocket;
	}
	
	void Connector::UDPdisconnect(int socket, const std::string& address, const std::string& localInterface)
	{
		//g_lpfnWriteLog(LOG_DEBUG, "UDPdisconnect %s\n", address.c_str());
		int iResult = 0;
		struct ip_mreq imreq;
		//imreq.imr_interface.s_addr = inet_addr(localInterface.c_str());
		imreq.imr_interface.s_addr = htonl(INADDR_ANY);
		imreq.imr_multiaddr.s_addr = inet_addr(address.c_str());
		iResult = setsockopt( socket, IPPROTO_IP, IP_DROP_MEMBERSHIP, (const char *)&imreq, sizeof(imreq));
		if ( iResult == SOCKET_ERROR )
		{
			//g_lpfnWriteLog(LOG_DEBUG, "[UDPdisconnect]: setsockopt error, Error code:%d\n", WSAGetLastError() );
		}

		drop(socket);
	}

	int Connector::TCPconnect(const std::string& address, int port)
	{
		int sock = socket_createConnector();
		if (sock == -1)
			return -1;
		
		addConnect(sock);
		socket_connect(sock, address.c_str(), port);
		
		return sock;
	}


// 	void Connector::signal( int socket )
// 	{
// 		socket_send( m_signal, (char*)&socket, sizeof(socket) );
// 	}

// 	void Connector::unsignal( int s )
// 	{
// 		Sockets::iterator i = m_writeSockets.find( s );
// 		if( i == m_writeSockets.end() ) return;
// 
// 		m_writeSockets.erase( s );
// 	}

	void Connector::block( Strategy& strategy, bool poll, long timeout /* = 0 */ )
	{
// 		while ( m_dropped.size() )
// 		{
// 			strategy.onError( *this, m_dropped.front() );
// 			m_dropped.pop();
// 			if ( m_dropped.size() == 0 )
// 				return ;
// 		}

// 		timeval timeVal;
// 		timeVal.tv_sec = timeout;
// 		timeVal.tv_usec = 0;

		fd_set readSet;
		FD_ZERO( &readSet );
		buildSet( m_readSockets, readSet );
		fd_set writeSet;
		FD_ZERO( &writeSet );
		buildSet( m_connectSockets, writeSet );
		buildSet( m_writeSockets, writeSet );
		fd_set exceptSet;
		FD_ZERO( &exceptSet );
		buildSet( m_connectSockets, exceptSet );

// 		if ( sleepIfEmpty(poll) )
// 		{
// 			strategy.onTimeout( *this );
// 			return;
// 		}

		int result = select( FD_SETSIZE, &readSet, NULL, NULL, &m_timeval);//&writeSet, &exceptSet, &m_timeval );

		//g_lpfnWriteLog(LOG_DEBUG, "select result:%d", result);

		if ( result == 0 )
		{
			//strategy.onTimeout( *this );
			return;
		}
		else if ( result > 0 )
		{
			//processExceptSet( strategy, exceptSet );
			//processWriteSet( strategy, writeSet );
			processReadSet( strategy, readSet );
		}
		else
		{
			strategy.onError( *this );
		}
	}

	void Connector::processReadSet( Strategy& strategy, fd_set& readSet )
	{
		for ( unsigned i = 0; i < readSet.fd_count; ++i )
		{
			int s = readSet.fd_array[ i ];
// 			if ( s == m_interrupt )
// 			{
// 				int socket = 0;
// 				recv( s, (char*)&socket, sizeof(socket), 0 );
// 				addWrite( socket );
// 				continue;
// 			}
			if ( !strategy.onData( *this, s ) )
			{
				strategy.onDisconnect( *this, s );
			}
		}
	}
	void Connector::processWriteSet( Strategy& strategy, fd_set& writeSet )
	{
		for ( unsigned i = 0; i < writeSet.fd_count; ++i )
		{
			int s = writeSet.fd_array[ i ];
			if( m_connectSockets.find(s) != m_connectSockets.end() )
			{
				m_connectSockets.erase( s );
				m_readSockets.insert( s );
				strategy.onConnect( *this, s );
			}
			else
			{
				strategy.onWrite( *this, s );
			}
		}
	}

	void Connector::processExceptSet( Strategy& strategy, fd_set& exceptSet )
	{
		for ( unsigned i = 0; i < exceptSet.fd_count; ++i )
		{
			int s = exceptSet.fd_array[ i ];
			//g_lpfnWriteLog(LOG_DEBUG, "[processExceptSet]: exceptSet:%d\n", s);
			strategy.onError( *this, s );
		}
	}


// 	inline timeval* Connector::getTimeval( bool poll, long timeout )
// 	{
// 		if ( poll )
// 		{
// 			m_timeval.tv_sec = 0;
// 			m_timeval.tv_usec = 0;
// 			return &m_timeval;
// 		}
// 
// 		timeout = m_timeout;
// 
// 		if ( !timeout )
// 			return 0;
// #ifdef SELECT_MODIFIES_TIMEVAL
// 		if ( !m_timeval.tv_sec && !m_timeval.tv_usec && timeout )
// 			m_timeval.tv_sec = timeout;
// 		return &m_timeval;
// #else
// 		long elapsed = ( clock() - m_ticks ) / CLOCKS_PER_SEC;
// 		if ( elapsed >= timeout || elapsed == 0.0 )
// 		{
// 			m_ticks = clock();
// 			m_timeval.tv_sec = 0;
// 			m_timeval.tv_usec = timeout * 1000000;
// 		}
// 		else
// 		{
// 			m_timeval.tv_sec = 0;
// 			m_timeval.tv_usec = ( timeout - elapsed ) * 1000000 ;
// 		}
// 		return &m_timeval;
// #endif
// 	}

	bool Connector::addConnect( int socket )
	{
		socket_setnonblock( socket );
		Sockets::iterator i = m_connectSockets.find( socket );
		if ( i != m_connectSockets.end() )
		{
			return false;
		}
		m_connectSockets.insert( socket );
		return true;
	}


	bool Connector::addRead( int s )
	{
		socket_setnonblock( s );
		Sockets::iterator i = m_readSockets.find( s );
		if( i != m_readSockets.end() ) return false;

		m_readSockets.insert( s );
		return true;
	}
	bool Connector::addWrite( int s )
	{
		if( m_readSockets.find(s) == m_readSockets.end() )
			return false;

		socket_setnonblock( s );
		Sockets::iterator i = m_writeSockets.find( s );
		if( i != m_writeSockets.end() ) return false;

		m_writeSockets.insert( s );
		return true;
	}

	bool Connector::drop( int s )
	{
		Sockets::iterator i = m_readSockets.find( s );
		Sockets::iterator j = m_writeSockets.find( s );
		Sockets::iterator k = m_connectSockets.find( s );

		if ( i != m_readSockets.end() || 
			j != m_writeSockets.end() ||
			k != m_connectSockets.end() )
		{
			socket_close( s );
			m_readSockets.erase( s );
			m_writeSockets.erase( s );
			m_connectSockets.erase( s );
			//m_dropped.push( s );
			return true;
		}
		return false;
	}


	void Connector::buildSet( const Sockets& sockets, fd_set& watchSet )
	{
		Sockets::const_iterator iter;
		for ( iter = sockets.begin(); iter != sockets.end(); ++iter ) {
			FD_SET( *iter, &watchSet );
		}
	}

// 	bool Connector::sleepIfEmpty( bool poll )
// 	{
// 		if( poll )
// 			return false;
// 
// 		if ( m_readSockets.empty() && 
// 			m_writeSockets.empty() &&
// 			m_connectSockets.empty() )
// 		{
// 			process_sleep( m_timeout );
// 			return true;
// 		}
// 		else
// 			return false;
// 	}
}

