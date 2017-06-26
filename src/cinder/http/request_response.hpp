#pragma once

#include <vector>
#include <string>
#include <algorithm>
#include <memory>
#include <cctype>
#include <iomanip>
#include <sstream>
#include <string>


#if defined(_WIN32)
#undef DELETE	
#endif

#include "url.hpp"
#include "headers.hpp"
#include "cinder/Base64.h"
#include "cinder/Surface.h"
#include "cinder/DataSource.h"
#include "cinder/ImageIo.h"
#include "jsoncpp/json.h"
#include "cinder/Url.h"

namespace cinder {
namespace http {
	
//! Enums representing HTTP request methods. Info from:
//! https://developer.mozilla.org/en-US/docs/Web/HTTP/Methods
enum class RequestMethod {
	//! The GET method request a representation of the specified resource.
	GET,
	//! The HEAD method is identical to GET, but without the response body.    
	HEAD,   
	//! The POST method is used to submit an entity to the specified resource.
	POST,   
	//! The PUT method replaces all current representations of the target 
	//! resource with the request payload.
	PUT,  
	//! The DELETE method deletes the specified resource.  
	DELETE,
	//! The CONNECT method establishes a tunnel to the server identified by the
	//! target resource. 
	CONNECT,
	//! The OPTIONS method is used to describe the communication options for the 
	//! target resource.
	OPTIONS,
	//! The TRACE method performs a message loop-back test along the path to the 
	//! target resource.
	TRACE,
	// The PATCH method is used to apply partial modifications to a resource.
	PATCH
};
	
// Request shared ref Alias.
using RequestRef = std::shared_ptr<struct Request>;

struct Request {
	//! Constructs the request with /a requestMethod and /a url. Also, sets two 
	//! headers, "Accepts: */*" and "Connection: close"
	Request( RequestMethod requestMethod, const UrlRef &requestUrl )
	: mRequestMethod( requestMethod ), mRequestUrl( requestUrl ) {}
	Request();

	//! Returns a pair of uint32_t representing the major, minor version number of HTTP
	std::pair<uint32_t, uint32_t> getVersion() const { return{ mVersionMajor, mVersionMinor }; }
	//! Sets the underlying /a major, /a minor HTTP version of this request
	void setVersion( uint32_t major, uint32_t minor ) { mVersionMajor = major; mVersionMinor = minor; }

	//! Returns a const ref to the url attached to this request
	const UrlRef& getUrl() const { return mRequestUrl; }
	//! Sets the url of this request
	void setUrl( UrlRef request_url ) { mRequestUrl = request_url; }

	//! Returns the RequestMethod of this request
	RequestMethod getRequestMethod() { return mRequestMethod; }
	//! Sets the RequestMethod of this request
	void setRequestMethod( RequestMethod method ) { mRequestMethod = method; }
	//! Returns a const char* translation of the RequestMethod
	const char* getRequestMethod( RequestMethod method ) const
	{
		switch( method ) {
			case RequestMethod::GET: return "GET"; break;
			case RequestMethod::POST: return "POST"; break;
			case RequestMethod::HEAD: return "HEAD"; break;
			case RequestMethod::PUT: return "PUT"; break;
			case RequestMethod::DELETE: return "DELETE"; break;
			case RequestMethod::CONNECT: return "CONNECT"; break;
			case RequestMethod::OPTIONS: return "OPTIONS"; break;
			case RequestMethod::TRACE: return "TRACE"; break;
			case RequestMethod::PATCH: return "PATCH"; break;
			default: CI_ASSERT_MSG( false, "Unknown RequestMethod" ); return ""; break;
		}
	}
	
	//! Returns a reference to the underlying HeaderSet.
	HeaderSet& getHeaders() { return mHeaderSet; }
	//! Returns a const ref to the underlying HeaderSet.
	const HeaderSet& getHeaders() const { return mHeaderSet; }
	//! Appends \a header to the underlying HeaderSet as type T.
	template<typename T>
	void appendHeader( T header );
	//! Sets the maximum amount of redirects before the request errors.
	void maxRedirects( int32_t max ) { mMaxRedirects = max; }
	//! Sets the timeout of the request. Default is 0 or off.
	template<typename DurationType>
	void timeout( DurationType duration )
	{
		mTimeout = std::chrono::duration_cast<std::chrono::nanoseconds>( duration );
	}

	//! Processes the request for output
	void process( std::ostream &request_buffer ) const;

	RequestMethod	mRequestMethod;
	UrlRef			mRequestUrl;
	uint32_t		mVersionMajor{1},
					mVersionMinor{1};
	int32_t			mMaxRedirects{-1};
	HeaderSet 		mHeaderSet;
	std::chrono::nanoseconds mTimeout{0};
};

//! Alias of a shared Response.
using ResponseRef = std::shared_ptr<struct Response>;

struct Response {
	//! Returns a pair of uint32_t representing the major, minor version number of HTTP
	std::pair<uint32_t, uint32_t> getVersion() const { return{ versionMajor, versionMinor }; }
	//! Sets the underlying /a major, /a minor HTTP version of this request
	void setVersion( uint32_t major, uint32_t minor ) { versionMajor = major; versionMinor = minor; }
	//! Returns the status code of the request.
	uint32_t getStatusCode() const { return statusCode; }
	//! Returns a reference to the underlying headerSet.
	HeaderSet& getHeaders() { return headerSet; }
	//! Returns a const ref to the underlying headerSet.
	const HeaderSet& getHeaders() const { return headerSet; }
	//! Returns a reference to the underlying content of the response.
	ci::BufferRef& getContent() { return headerSet.getContent(); }
	//! Returns a const ref to the underlying content of the response.
	const ci::BufferRef& getContent() const { return headerSet.getContent(); }
	//! Returns content converted to type T.
	template<typename T>
	T getContentAs();
	
	uint32_t	statusCode{0},
				versionMajor{0},
				versionMinor{0};
	HeaderSet	headerSet;
};
	
template<typename T>
inline void Request::appendHeader( T header )
{
	mHeaderSet.appendHeader( std::move( header ) );
}

inline void Request::process( std::ostream &request_stream ) const
{
	request_stream << getRequestMethod( mRequestMethod ) << " ";
	request_stream << mRequestUrl->to_escaped_string( Url::path_component | Url::query_component );
	request_stream << " HTTP/" << mVersionMajor << "." << mVersionMinor << "\r\n";
	request_stream << "Host: ";
	request_stream << mRequestUrl->to_escaped_string( Url::host_component | Url::port_component );
	request_stream << "\r\n";
	bool acceptEncodingCached = false;
	for( const auto &header : mHeaderSet.getHeaders() ) {
		if( ! strcmp( header.first.c_str(), AcceptEncoding::key() ) )
			acceptEncodingCached = true;
		request_stream << header.first << ": " << header.second << "\r\n";
	}
	if( ! acceptEncodingCached )
		request_stream << AcceptEncoding::key() << ": gzip, deflate\r\n";
	request_stream << "\r\n";
	const auto &content = mHeaderSet.getContent();
	if( content )
		request_stream.write( static_cast< const char* >( content->getData() ), content->getSize() );
}
	
inline std::ostream& operator<<( std::ostream &stream, const Request &request )
{
	request.process( stream );
	return stream;
}
	
template<>
inline ci::SurfaceRef Response::getContentAs<ci::SurfaceRef>()
{
	auto typeHeader = headerSet.findHeader( Content::Type::key() );
	CI_ASSERT( typeHeader );
	auto &type = typeHeader->second;
	std::string extension;
	
	if( type == "image/png" ) extension = "png";
	else if( type == "image/jpeg" ) extension = "jpeg";
	
	auto dataSource = ci::DataSourceBuffer::create( headerSet.getContent() );
	return ci::Surface::create( loadImage( dataSource, ImageSource::Options(), extension ) );
}
	
template<>
inline ci::Surface Response::getContentAs<ci::Surface>()
{
	auto typeHeader = headerSet.findHeader( Content::Type::key() );
	CI_ASSERT( typeHeader );
	auto &type = typeHeader->second;
	CI_ASSERT( type.find( "image/" ) != std::string::npos );
	
	std::string extension;
	if( type == "image/png" ) extension = "png";
	else if( type == "image/jpeg" ) extension = "jpeg";
	else CI_ASSERT( false );
	
	auto dataSource = ci::DataSourceBuffer::create( headerSet.getContent() );
	return ci::Surface( loadImage( dataSource, ImageSource::Options(), extension ) );
}
	
template<>
inline Json::Value Response::getContentAs<Json::Value>()
{
	auto typeHeader = headerSet.findHeader( Content::Type::key() );
	CI_ASSERT( typeHeader );
	CI_ASSERT( typeHeader->second.find( "application/json" ) != std::string::npos );
	
	auto &content = headerSet.getContent();
	auto begIt = static_cast< const char* >( content->getData() );
	auto endIt = begIt + content->getSize();
	
	Json::Features features;
	features.allowComments_ = true;
	features.strictRoot_ = true;
	Json::Reader reader( features );
	Json::Value value;
	reader.parse( begIt, endIt, value, false );
	
	return value;
}

} // http
} // cinder

#if defined(_WIN32)
#define DELETE                           (0x00010000L)
#endif
