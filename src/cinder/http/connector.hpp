//
//  connector.hpp
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

namespace cinder {
namespace http { namespace detail {
	
template<typename SessionType>
struct Connector : public std::enable_shared_from_this<Connector<SessionType>> {
	
	Connector( std::shared_ptr<SessionType> session, asio::ip::tcp::socket &socket )
	: mSession( session ), mSocket( socket ), mResolver( socket.get_io_service() ) {}
	
	void start();
	void start( asio::ip::tcp::endpoint endpoint );
	
private:
	void on_resolve( asio::error_code ec, asio::ip::tcp::resolver::iterator iter );
	void on_connect( asio::error_code ec, asio::ip::tcp::resolver::iterator iter );
	void on_connecting_to_endpoint( asio::error_code ec );

	std::shared_ptr<SessionType>	mSession;
	asio::ip::tcp::resolver			mResolver;
	asio::ip::tcp::socket			&mSocket;
};

template<typename SessionType>
void Connector<SessionType>::start()
{
	asio::error_code ec;
	// Fail if the socket is already open.
	if (mSocket.lowest_layer().is_open()) {
		ec = asio::error::already_open;
		mSocket.get_io_service().post(
			std::bind( &SessionType::onError, mSession, ec ) );
		return;
	}
	
	mSocket.open(asio::ip::tcp::v4(), ec);
	if( ec ) {
		mSocket.get_io_service().post(
			std::bind( &SessionType::onError, mSession, ec ) );
		return;
	}
	
	asio::ip::tcp::resolver::query query(mSession->mSessionUrl->host(),
										 std::to_string( mSession->mSessionUrl->port() ) );
	mResolver.async_resolve( query, bind( &Connector<SessionType>::on_resolve,
										this->shared_from_this(),
										std::placeholders::_1,
										std::placeholders::_2 ) );
}
	
template<typename SessionType>
void Connector<SessionType>::start( asio::ip::tcp::endpoint endpoint )
{
	asio::error_code ec;
	// Fail if the socket is already open.
	if (mSocket.lowest_layer().is_open()) {
		ec = asio::error::already_open;
		mSocket.get_io_service().post(
			std::bind( &SessionType::onError, mSession, ec ) );
		return;
	}
	
	mSocket.open(asio::ip::tcp::v4(), ec);
	if( ec ) {
		mSocket.get_io_service().post(
			std::bind( &SessionType::onError, mSession, ec ) );
		return;
	}
	
	mSession->endpoint = endpoint;
	mSocket.async_connect( mSession->endpoint,
						  std::bind( &Connector<SessionType>::on_connecting_to_endpoint,
									this->shared_from_this(),
									std::placeholders::_1 ) );
}

template<typename SessionType>
void Connector<SessionType>::on_resolve(asio::error_code ec, asio::ip::tcp::resolver::iterator iter)
{
	if (!ec) {
		// Attempt a connection to the first endpoint in the list. Each endpoint
		// will be tried until we successfully establish a connection.
		mSession->endpoint = *iter;
		mSocket.async_connect( mSession->endpoint,
							   std::bind( &Connector<SessionType>::on_connect,
										  this->shared_from_this(),
										  std::placeholders::_1,
										  ++iter ) );
	}
	else
		mSocket.get_io_service().post(
			std::bind( &SessionType::onError, mSession, ec ) );
}

template<typename SessionType>
void Connector<SessionType>::on_connect( asio::error_code ec, asio::ip::tcp::resolver::iterator iter )
{
	// Try each endpoint until we successfully establish a connection
	if( !ec ) {
		// Check whether the operation has been cancelled.
		if (!mSocket.is_open())
		{
			ec = asio::error::operation_aborted;
			mSocket.get_io_service().post(
				std::bind( &SessionType::onError, mSession, ec ) );
			return;
		}
		
		// Disable the Nagle algorithm on all sockets.
		mSocket.set_option(asio::ip::tcp::no_delay(true), ec);
		
		// Signal that we're done
		mSocket.get_io_service().post(
			std::bind( &SessionType::onOpen, mSession, ec ) );
	}
	else if( iter != asio::ip::tcp::resolver::iterator() ) {
		if (!mSocket.is_open()) {
			ec = asio::error::operation_aborted;
			mSocket.get_io_service().post(
				std::bind( &SessionType::onError, mSession, ec ) );
			return;
		}
		
		// Try next endpoint.
		mSocket.close(ec);
		mSession->endpoint = *iter;
		mSocket.async_connect( mSession->endpoint,
							   std::bind( &Connector<SessionType>::on_connect,
										  this->shared_from_this(),
										  std::placeholders::_1,
										  ++iter ) );
	}
	else if ( ec ) 
		mSocket.get_io_service().post(
			std::bind( &SessionType::onError, mSession, ec ) );
}
	
template<typename SessionType>
void Connector<SessionType>::on_connecting_to_endpoint( asio::error_code ec )
{
	if( !ec ) {
		// Check whether the operation has been cancelled.
		if (!mSocket.is_open())
		{
			ec = asio::error::operation_aborted;
			mSocket.get_io_service().post(
				std::bind( &SessionType::onError, mSession, ec ) );
			return;
		}
		
		// Disable the Nagle algorithm on all sockets.
		mSocket.set_option(asio::ip::tcp::no_delay(true), ec);
		
		// Signal that we're done
		mSocket.get_io_service().post(
			std::bind( &SessionType::onOpen, mSession, ec ) );
	}
	else if ( ec )
		mSocket.get_io_service().post(
			std::bind( &SessionType::onError, mSession, ec ) );
}
	
} // detail
} // http
} // cinder
