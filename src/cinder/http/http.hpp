//
//  http.hpp
//  UrlTest
//
//  Created by Ryan Bartley on 6/1/16.
//
//

#pragma once

#if ! defined( ASIO_STANDALONE )
#define ASIO_STANDALONE 1
#endif

#include "asio/asio.hpp"
#if defined( USING_SSL )
#include "asio/ssl.hpp"
#endif

#include "cinder/app/App.h"
#include "cinder/Log.h"

#include "url.hpp"
#include "connector.hpp"
#include "handshaker.hpp"
#include "requester.hpp"
#include "responder.hpp"
#include "request_response.hpp"

namespace cinder {
namespace http {
	
using Timer = asio::basic_waitable_timer<std::chrono::high_resolution_clock>;
using TimerRef = std::shared_ptr<Timer>;

enum class protocol {
	http,
	https,
	file
};
	
using ResponseHandler = std::function<void( asio::error_code, ResponseRef )>;
using ErrorHandler = std::function<void( asio::error_code, const UrlRef &, ResponseRef )>;
	
namespace detail {
	
struct Session {
public:
	Session( asio::io_service &service, const UrlRef &url )
	: mSocket( service )
	{}
	
protected:
	asio::ip::tcp::socket& socket() { return mSocket; }
	void cancel() { mSocket.cancel(); };
	
	asio::ip::tcp::socket	mSocket;
};

#if defined(USING_SSL)
struct SslSession {
public:
	SslSession( asio::io_service &service, const UrlRef &url )
	: context(asio::ssl::context::tlsv12_client),
		mSocket( service, context )
	{
		context.set_default_verify_paths();
		auto &&host = url->host();
		auto &&verification = asio::ssl::rfc2818_verification{std::move(host)};
		mSocket.set_verify_callback(verification);
	}
	
protected:
	asio::ip::tcp::socket& socket() { return mSocket.next_layer(); }
	void cancel() { mSocket.next_layer().cancel(); }
	
	asio::ssl::context							context;
	asio::ssl::stream<asio::ip::tcp::socket>	mSocket;
};
#endif

template<typename socket_impl>
class ClientImpl : public socket_impl,
	public std::enable_shared_from_this<ClientImpl<socket_impl>> {
public:
	
	ClientImpl( RequestRef request, ResponseHandler responseHandler, ErrorHandler errorHandler,
			    asio::io_service &io_service = ci::app::App::get()->io_service() )
	: socket_impl( io_service, request->getUrl() ), io_service( io_service ), timeout_reached( false ),
		responseHandler( responseHandler ), errorHandler( errorHandler ), request( request )
	{
	}
	~ClientImpl() = default;
	
	asio::io_service&	get_io_service() { return io_service; }
	const UrlRef&		getUrl() const { return request->getUrl(); }
	
	const asio::ip::tcp::endpoint&	getEndpoint() const { return endpoint; }
	asio::ip::tcp::endpoint&		getEndpoint() { return endpoint; }
	
	void start()
	{
		if( request->mTimeout != std::chrono::nanoseconds(0) )
			createTimeout( request->mTimeout );
		std::make_shared<detail::Connector<ClientImpl>>(
				this->shared_from_this(), socket_impl::socket() )->start();
	}
	
	void start( asio::ip::tcp::endpoint endpoint )
	{
		if( request->mTimeout != std::chrono::nanoseconds(0) )
			createTimeout( request->mTimeout );
		std::make_shared<detail::Connector<ClientImpl>>(
				this->shared_from_this(), socket_impl::socket() )->start( endpoint );
	}
	
protected:
	ClientImpl( const ClientImpl &other )
	{
		
	}
	ClientImpl( ClientImpl &&other )
	{
		
	}
	ClientImpl& operator=( const ClientImpl &other ) = delete;
	ClientImpl operator=( ClientImpl &&other ) = delete;
		
	void onOpen( asio::error_code ec )
	{
		std::make_shared<detail::Handshaker<ClientImpl>>(
				this->shared_from_this() )->handshake();
	}
	void onHandshake( asio::error_code ec )
	{
		std::make_shared<detail::Requester<ClientImpl>>(
				this->shared_from_this() )->request();
	}
	void onRequest( asio::error_code ec )
	{
		std::make_shared<detail::Responder<ClientImpl>>(
				this->shared_from_this() )->read();
	}
	void onResponse( asio::error_code ec )
	{
		if( timeout_clock )
			timeout_clock->cancel();
		responseHandler( ec, response );
	}
		
	void createTimeout( std::chrono::nanoseconds timeout_duration )
	{
		timeout_clock = std::make_shared<Timer>( io_service );
		auto &&self = this->shared_from_this();
		timeout_clock->expires_from_now( timeout_duration );
		timeout_clock->async_wait( [self]( asio::error_code ec ){ self->onDeadline( ec ); });
	}
	
	void onError( asio::error_code ec )
	{
		if( ec == asio::error::operation_aborted && timeout_reached )
			return;
		
		if( timeout_clock )
			timeout_clock->cancel();
		errorHandler( ec, request->getUrl(), response );
	}
	
	void onDeadline( asio::error_code ec )
	{
		timeout_clock.reset();
		if( ec == asio::error::operation_aborted )
			return;
		
		timeout_reached.store( true );
		socket_impl::cancel();
		ec = asio::error::timed_out;
		
		errorHandler( ec, request->getUrl(), response );
	}
	
	asio::io_service		&io_service;
	
	TimerRef				timeout_clock;
	std::atomic_bool		timeout_reached;
	uint32_t				attempted_redirects{0};
	
	ResponseHandler		responseHandler;
	ErrorHandler		errorHandler;
	RequestRef			request;
	ResponseRef			response;
		
	
	asio::ip::tcp::endpoint	endpoint;
	
	friend struct detail::Connector<ClientImpl>;
	friend struct detail::Handshaker<ClientImpl>;
	friend struct detail::Requester<ClientImpl>;
	friend struct detail::Responder<ClientImpl>;
	friend struct detail::Redirector<ClientImpl>;
};

}
	
using Session = detail::ClientImpl<detail::Session>;
using SessionRef = std::shared_ptr<Session>;
#if defined(USING_SSL)
using SslSession = detail::ClientImpl<detail::SslSession>;
using SslSessionRef = std::shared_ptr<SslSession>;
#endif

}} // http // cinder
