#pragma once

#include <vector>
#include <string>
#include <algorithm>
#include <memory>
#include <cctype>
#include <iomanip>
#include <sstream>
#include <string>

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
	
enum class RequestMethod {
	GET,
	POST
};
	
using RequestRef = std::shared_ptr<struct Request>;

struct Request {
	//! Constructs the request with /a requestMethod and /a url. Also, sets two 
	//! headers, "Accepts: */*" and "Connection: close"
	Request( RequestMethod requestMethod, const UrlRef &requestUrl );
	Request();

	//! Returns a pair of uint32_t representing the major, minor version number of HTTP
	std::pair<uint32_t, uint32_t> getVersion() const { return{ versionMajor, versionMinor }; }
	//! Sets the underlying /a major, /a minor HTTP version of this request
	void setVersion( uint32_t major, uint32_t minor ) { versionMajor = major; versionMinor = minor; }

	//! Returns a const ref to the url attached to this request
	const UrlRef& getUrl() const { return requestUrl; }
	//! Sets the url of this request
	void setUrl( UrlRef request_url ) { requestUrl = request_url; }

	//! Returns the RequestMethod of this request
	RequestMethod getRequestMethod() { return requestMethod; }
	//! Sets the RequestMethod of this request
	void setRequestMethod( RequestMethod method ) { requestMethod = method; }
	//! Returns a const char* translation of the RequestMethod
	const char* getRequestMethod( RequestMethod method ) const
	{
		switch( method ) {
			case RequestMethod::GET: return "GET"; break;
			case RequestMethod::POST: return "POST"; break;
			default: return "GET"; break;
		}
	}
	
	std::string encode( const std::string &value ) const;
	
	HeaderSet& getHeaders() { return headerSet; }
	const HeaderSet& getHeaders() const { return headerSet; }
	
	template<typename T>
	void appendHeader( T header );

	//! Processes the request for output
	void process( std::ostream &request_buffer ) const;

	RequestMethod	requestMethod;
	UrlRef			requestUrl;
	uint32_t		versionMajor,
					versionMinor;
	HeaderSet 		headerSet;
};

using ResponseRef = std::shared_ptr<struct Response>;

struct Response {
	
	//! Returns a pair of uint32_t representing the major, minor version number of HTTP
	std::pair<uint32_t, uint32_t> getVersion() const { return{ versionMajor, versionMinor }; }
	//! Sets the underlying /a major, /a minor HTTP version of this request
	void setVersion( uint32_t major, uint32_t minor ) { versionMajor = major; versionMinor = minor; }
	
	uint32_t getStatusCode() const { return statusCode; }
	
	HeaderSet& getHeaders() { return headerSet; }
	const HeaderSet& getHeaders() const { return headerSet; }
	
	ci::BufferRef& getContent() { return headerSet.getContent(); }
	const ci::BufferRef& getContent() const { return headerSet.getContent(); }
	
	template<typename T>
	T getContentAs();
	
	uint32_t	statusCode{0},
				versionMajor{0},
				versionMinor{0};
	HeaderSet	headerSet;
};
	


inline Request::Request( RequestMethod requestMethod, const UrlRef &requestUrl )
: requestMethod( requestMethod ), requestUrl( requestUrl ),
	versionMajor( 1 ), versionMinor( 1 )
{
}
	
template<typename T>
inline void Request::appendHeader( T header )
{
	headerSet.appendHeader( std::move( header ) );
}
	
// Found parts of this implementation from this stackoverflow post.
// http://stackoverflow.com/questions/154536/encode-decode-urls-in-c
// still needs work. not exactly what i want.
inline std::string Request::encode( const std::string &value ) const
{
	std::ostringstream escaped;
	escaped.fill('0');
	escaped << std::hex;
	
	for ( auto c : value ) {
		switch( c ) {
			case '-': case '_': case '.': case '!': case '~': case '*':
			case '\'': case '(': case ')': case ':': case '@': case '&':
			case '=': case '+': case '$': case ',': case '/': case ';':
				escaped << c;
			break;
			default: {
				if( std::isalnum(c) ) {
					escaped << c;
				}
				else {
					// Any other characters are percent-encoded
					escaped << std::uppercase;
					escaped << '%' << std::setw(2) << int((unsigned char) c);
					escaped << std::nouppercase;
				}
			}
			break;
		}
	}
	
	return escaped.str();
}

inline void Request::process( std::ostream &request_stream ) const
{
	request_stream << getRequestMethod( requestMethod ) << " ";
	request_stream << encode( requestUrl->to_string( Url::path_component ) );
	request_stream << requestUrl->to_string( Url::query_component );
	request_stream << " HTTP/" << versionMajor << "." << versionMinor << "\r\n";
	request_stream << "Host: ";
	request_stream << requestUrl->to_string( Url::host_component | Url::port_component );
	request_stream << "\r\n";
	bool acceptEncodingCached = false;
	for( auto &header : headerSet.getHeaders() ) {
		if( ! strcmp( header.first.c_str(), AcceptEncoding::key() ) )
			acceptEncodingCached = true;
		request_stream << header.first << ": " << header.second << "\r\n";
	}
	if( ! acceptEncodingCached )
		request_stream << AcceptEncoding::key() << ": gzip, deflate\r\n";
	request_stream << "\r\n";
	auto content = headerSet.getContent();
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
