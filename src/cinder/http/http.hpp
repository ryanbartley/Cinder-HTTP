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
#define USING_SSL
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

enum class protocol {
	http,
	https,
	file
};
	
enum class Process {
	RESOLVE,
	OPEN,
	CONNECT,
	HANDSHAKE,
	REQUEST,
	RESPONSE
};
	
using ResponseHandler = std::function<void( asio::error_code, Response )>;
using ErrorHandler = std::function<void( asio::error_code, Process, const std::shared_ptr<url> &, Response )>;
	
using SessionRef = std::shared_ptr<class Session>;

class Session : public std::enable_shared_from_this<Session> {
public:
	
	Session( std::shared_ptr<url> url, RequestRef request,
			 ResponseHandler responseHandler, ErrorHandler errorHandler,
			 asio::io_service &io_service = ci::app::App::get()->io_service() )
	: io_service( io_service ), socket( io_service ), responseHandler( responseHandler ),
	errorHandler( errorHandler ), url( url ), request( request ) {}
	~Session() = default;
	
	asio::io_service&	get_io_service() { return io_service; }
	const std::shared_ptr<url>&			getUrl() const { return url; }
	std::shared_ptr<url>&				getUrl() { return url; }
	
	const asio::ip::tcp::endpoint&	getEndpoint() const { return endpoint; }
	asio::ip::tcp::endpoint&		getEndpoint() { return endpoint; }
	
	void start()
	{
		auto con = std::make_shared<detail::Connector<Session>>( shared_from_this(), socket );
		con->start();
	}
	
	void start( asio::ip::tcp::endpoint endpoint )
	{
		std::make_shared<detail::Connector<Session>>(
			shared_from_this(), socket )->start( endpoint );
	}
	
private:
	void onOpen( asio::error_code ec )
	{
		std::make_shared<detail::Handshaker<Session>>(
			shared_from_this() )->handshake();
	}
	void onHandshake( asio::error_code ec )
	{
		if( ! request )
			request = std::make_shared<Request>( RequestMethod::GET, url );
		std::make_shared<detail::Requester<Session>>(
			shared_from_this(), std::move( request ) )->request();
	}
	void onRequest( asio::error_code ec )
	{
		std::make_shared<detail::Responder<Session>>(
			shared_from_this() )->read();
	}
	void onResponse( asio::error_code ec, Response response )
	{
		responseHandler( ec, response );
	}
	
	void onError( asio::error_code ec ) {
		errorHandler( ec, Process::CONNECT, url, Response() );
	}
	
	asio::io_service		&io_service;
	asio::ip::tcp::socket	socket;
	
	ResponseHandler			responseHandler;
	ErrorHandler			errorHandler;
	RequestRef				request;
	
	std::shared_ptr<url>	url;
	asio::ip::tcp::endpoint	endpoint;
	
	friend struct detail::Connector<Session>;
	friend struct detail::Handshaker<Session>;
	friend struct detail::Requester<Session>;
	friend struct detail::Responder<Session>;
};
	
#if defined( USING_SSL )
	
using SslSessionRef = std::shared_ptr<class SslSession>;

class SslSession : public std::enable_shared_from_this<SslSession> {
public:
	
	SslSession( std::shared_ptr<url> url, RequestRef request,
			    ResponseHandler responseHandler, ErrorHandler errorHandler,
			    asio::io_service &io_service = ci::app::App::get()->io_service() )
	: io_service( io_service ), context(asio::ssl::context::tlsv12_client),
	socket( io_service, context ), url( url ), responseHandler( responseHandler ),
	errorHandler( errorHandler ), request( request )
	{
		context.set_default_verify_paths();
		auto host = url->host();
		socket.set_verify_callback(asio::ssl::rfc2818_verification{host});
	}
	~SslSession() = default;
	
	asio::io_service&	get_io_service() { return io_service; }
	const std::shared_ptr<url>&			getUrl() const { return url; }
	std::shared_ptr<url>&				getUrl() { return url; }
	
	const asio::ip::tcp::endpoint&	getEndpoint() const { return endpoint; }
	asio::ip::tcp::endpoint&		getEndpoint() { return endpoint; }
	
	void start()
	{
		std::make_shared<detail::Connector<SslSession>>(
			shared_from_this(), socket.next_layer() )->start();
	}
	
	void start( asio::ip::tcp::endpoint endpoint )
	{
		std::make_shared<detail::Connector<SslSession>>(
			shared_from_this(), socket.next_layer() )->start( endpoint );
	}
	
private:
	void onOpen( asio::error_code ec )
	{
		std::make_shared<detail::Handshaker<SslSession>>(
			shared_from_this() )->handshake();
	}
	void onHandshake( asio::error_code ec )
	{
		if( ! request )
			request = std::make_shared<Request>( RequestMethod::GET, url );
		std::make_shared<detail::Requester<SslSession>>(
			shared_from_this(), std::move( request ) )->request();
	}
	void onRequest( asio::error_code ec )
	{
		std::make_shared<detail::Responder<SslSession>>(
			shared_from_this() )->read();
	}
	void onResponse( asio::error_code ec, Response response )
	{
		responseHandler( ec, response );
	}
	
	void onError( asio::error_code ec ) {
		errorHandler( ec, Process::CONNECT, url, Response() );
	}
	
	asio::io_service		&io_service;
	asio::ssl::context		context;
	asio::ssl::stream<asio::ip::tcp::socket> socket;
	
	ResponseHandler			responseHandler;
	ErrorHandler			errorHandler;
	RequestRef				request;
	
	std::shared_ptr<url>	url;
	asio::ip::tcp::endpoint	endpoint;
	
	friend struct detail::Connector<SslSession>;
	friend struct detail::Handshaker<SslSession>;
	friend struct detail::Requester<SslSession>;
	friend struct detail::Responder<SslSession>;
};
	
#endif
	
}} // http // cinder