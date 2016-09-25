//
//  handshaker.hpp
//  Test
//
//  Created by Ryan Bartley on 6/17/16.
//
//

#pragma once

#if ! defined( ASIO_STANDALONE )
#define ASIO_STANDALONE 1
#endif

#include "url.hpp"
#include "asio/asio.hpp"

#if defined( USING_SSL )
#include "asio/ssl.hpp"
#endif

namespace cinder {
namespace http { namespace detail {

template<typename Handshaker, typename Handler, typename Socket>
void async_handshake( std::shared_ptr<Handshaker> handshaker, Handler handler, Socket &socket );

template<typename SessionType>
struct Handshaker : std::enable_shared_from_this<Handshaker<SessionType>> {
	Handshaker( std::shared_ptr<SessionType> session )
	: mSession( session ), mHost( session->mSessionUrl->host() ) {}
	
	void handshake()
	{
		async_handshake( this->shared_from_this(),
						 std::bind( &Handshaker<SessionType>::on_handshake,
									this->shared_from_this(), 
									std::placeholders::_1 ),
						 mSession->socket );
	}
	
private:
	void on_handshake( asio::error_code ec )
	{
		auto &io = mSession->socket.get_io_service();
		if( !ec )
			io.post( std::bind( &SessionType::onHandshake, mSession, ec ) );
		else
			io.post( std::bind( &SessionType::onError, mSession, ec ) );
	}
	
	std::shared_ptr<SessionType>	mSession;
	std::string						mHost;
};
	
#if defined( USING_SSL )
template<typename Handshaker, typename Handler>
void async_handshake( std::shared_ptr<Handshaker> handshaker, Handler handler, 
					  asio::ssl::stream<asio::ip::tcp::socket> &socket )
{
	socket.async_handshake( asio::ssl::stream_base::client, handler );
}
#endif
	
template<typename Handshaker, typename Handler>
void async_handshake( std::shared_ptr<Handshaker> handshaker, Handler handler, 
					  asio::ip::tcp::socket &socket )
{
	asio::error_code ec;
	handler( ec );
}
	
} // detail
} // http
} // cinder
