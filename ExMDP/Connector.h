#pragma once
#ifdef _MSC_VER
#pragma warning( disable : 4503 4355 4786 4290 )
#endif
#ifdef _MSC_VER
#include <Winsock2.h>
typedef int socklen_t;
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#include <set>
#include <queue>
#include <time.h>
#include <fstream>

namespace MDP
{
	/// Connects sockets to remote ports and addresses.
	class Connector
	{
	public:
		class Strategy;

		Connector();
		~Connector();

		//void setLogFile();

		int UDPconnect( const std::string& address, const int port, const std::string& localInterface);
		int TCPconnect( const std::string& address, const int port);

		void UDPdisconnect(int socket, const std::string& address, const std::string& localInterface);

		bool addConnect( int socket );
		bool addRead( int socket );
		bool addWrite( int socket );
		bool drop( int socket );
// 		void signal( int socket );
// 		void unsignal( int socket );
		void block( Strategy& strategy, bool poll = 0, long timeout = 0 );

		size_t numSockets() 
		{ return m_readSockets.size() - 1; }

	private:
		typedef std::set < int > Sockets;
		typedef std::queue < int > Queue;

		//bool listen();
		void buildSet( const Sockets&, fd_set& );
		//inline timeval* getTimeval( bool poll, long timeout );
		//inline bool sleepIfEmpty( bool poll );

		void processReadSet( Strategy&, fd_set& );
		void processWriteSet( Strategy&, fd_set& );
		void processExceptSet( Strategy&, fd_set& );

		int m_timeout;
		timeval m_timeval;
// #ifndef SELECT_DECREMENTS_TIME
// 		clock_t m_ticks;
// #endif

		//int m_signal;
		//int m_interrupt;
		Sockets m_connectSockets;

		Sockets m_readSockets;
		Sockets m_writeSockets;
		//Queue m_dropped;

	public:

		class Strategy
		{
		public:
			virtual ~Strategy() {}
//			virtual void onEvent( Connector&, int socket ) = 0;
			virtual void onConnect( Connector&, int socket ) = 0;
			virtual void onWrite( Connector&, int socket ) = 0;
			virtual bool onData(  Connector&, int ) = 0;
			virtual void onDisconnect( Connector&, int ) = 0;
			virtual void onError( Connector&, int socket ) = 0;
			virtual void onError( Connector& ) = 0;
			virtual void onTimeout( Connector& ) = 0;
		};
	};
}
