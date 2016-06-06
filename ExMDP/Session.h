#pragma once
#include "Channel.h"
#include "Connector.h"
#include "Utility.h"
#include "Packet.h"
#include "Mutex.h"


namespace MDP
{
	class Channel;

	class Session
	{
	public:
		Session( Channel* pChannel,int s, Connector* pConnector, 
				 std::string& username, 
				 std::string& password, 
				 int beginSeqNum, 
				 int endSeqNum );

		~Session(void);

		/************************************************************************/
		/*会话过程                                                              */

		void next();

		void next( const std::string& );

		void disconnect();

		/************************************************************************/
		/*发送，回应消息														*/

		//发送登录消息
		void generateLogon();
		//发送请求消息
		void GenerateResendRequest();
		//回应登出消息
		void generateLogout();

		bool send( std::string& );
		bool processQueue();
// 		void signal()
// 		{
// 			Locker l( m_mutex );
// 			if( m_sendQueue.size() == 1 )
// 				m_connector->signal( m_socket );
// 		}
// 		void unsignal()
// 		{
// 			Locker l( m_mutex );
// 			if( m_sendQueue.size() == 0 )
// 				m_connector->unsignal( m_socket );
// 		}
		/************************************************************************/
		/*接收，处理消息                                                        */
		
		bool read( Connector& );
		void readFromSocket() throw( SocketRecvFailed );
		void readMessages( Connector& );
		bool readMessage( std::string& );
//		void processRtsPacket();

		/************************************************************************/
		/*会话状态                                                              */


		//登录消息是否已发送
		bool sentLogon() const { return m_sentLogon; }
		void sentLogon( bool value ) { m_sentLogon = value; }

		//登录回应是否已收到
		bool receivedLogon() const { return m_receivedLogon; }
		void receivedLogon( bool value ) { m_receivedLogon = value; }

		//是否已登录成功
		bool isLoggedOn() const { return sentLogon() && receivedLogon(); }

		//是否登录超时
		bool logonTimedOut() const
		{
			time_t now = time(NULL);
			return (now - getLastReceivedTime()) >= m_logonTimeout;
		}
		

		//请求消息是否已发送
		bool sentRequest() const { return m_sentRequest; }
		void sentRequest( bool value ) { m_sentRequest = value; }

		//登出消息是否已发送		bool sentLogout() const { return m_sentLogout; }
		void sentLogout( bool value ) { m_sentLogout = value; }

		//超时断开
		bool timedOut() const
		{
			time_t now = time(NULL);
			return ( now - getLastReceivedTime() ) >= 5;
		}

		//最后收到消息的时间
		void setLastReceivedTime() { m_lastReceivedTime = time(NULL); }
		const time_t getLastReceivedTime() const { return m_lastReceivedTime; }



	private:
		typedef int ChannelID;
		typedef std::deque<std::string, ALLOCATOR<std::string> > Queue;
		Queue m_sendQueue;

		std::string m_username;
		std::string m_password;
		int m_beginSeqNum;
		int m_endSeqNum;

		int m_socket;

		int m_logonTimeout;
		int m_logoutTimeout;

		bool m_enabled;
		bool m_sentLogon;
		bool m_sentLogout;
		bool m_sentRequest;
		bool m_receivedLogon;
		
		char m_buffer[2046];
		size_t m_len;

		time_t m_lastReceivedTime;

		Connector* m_connector;

		ChannelID m_channelID;

		Channel* m_pChannel;

		fd_set m_fds;

		unsigned m_sendLength;

		Mutex m_mutex;
	};
}



