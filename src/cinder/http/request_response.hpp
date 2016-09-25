#pragma once

#include <vector>
#include <string>
#include <algorithm>
#include <memory>

#include "url.hpp"
#include "headers.hpp"
#include "cinder/Base64.h"

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
	Request( RequestMethod requestMethod, const urlref &request_url );
	Request();

	//! Returns a pair of uint32_t representing the major, minor version number of HTTP
	std::pair<uint32_t, uint32_t> getVersion() const { return{ versionMajor, versionMinor }; }
	//! Sets the underlying /a major, /a minor HTTP version of this request
	void setVersion( uint32_t major, uint32_t minor ) { versionMajor = major; versionMinor = minor; }

	//! Returns a const ref to the url attached to this request
	const urlref& getUrl() const { return mRequestUrl; }
	//! Sets the url of this request
	void setUrl( urlref requestUrl ) { this->mRequestUrl = requestUrl; }

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
	
	HeaderSet& getHeaders() { return headerSet; }
	const HeaderSet& getHeaders() const { return headerSet; }
	
	template<typename T>
	void appendHeader( T header );

	//! Processes the request for output
	void process( std::ostream &request_buffer ) const;

	RequestMethod	requestMethod;
	urlref		mRequestUrl;
	uint32_t	versionMajor,
			versionMinor;
	HeaderSet 	headerSet;
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
	
	uint32_t	statusCode{0},
				versionMajor{0},
				versionMinor{0};
	HeaderSet	headerSet;
};

inline Request::Request( RequestMethod requestMethod, const urlref &requestUrl )
: requestMethod( requestMethod ), mRequestUrl( requestUrl ),
	versionMajor( 1 ), versionMinor( 1 )
{
}
	
template<typename T>
inline void Request::appendHeader( T header )
{
	headerSet.appendHeader( std::move( header ) );
}

inline void Request::process( std::ostream &request_stream ) const
{
	request_stream << getRequestMethod( requestMethod ) << " ";
	request_stream << mRequestUrl->to_string( url::path_component | url::query_component );
	request_stream << " HTTP/" << versionMajor << "." << versionMinor << "\r\n";
	request_stream << "Host: ";
	request_stream << mRequestUrl->to_string( url::host_component | url::port_component );
	request_stream << "\r\n";
	for( auto &header : headerSet.getHeaders() ) {
		request_stream << header.first << ": " << header.second << "\r\n";
	}
	request_stream << "\r\n";
	auto content = headerSet.getContent();
	request_stream.write( static_cast<const char*>( content->getData() ), content->getSize() );
}
	
inline std::ostream& operator<<( std::ostream &stream, const Request &request )
{
	request.process( stream );
	return stream;
}

} // http
} // cinder
