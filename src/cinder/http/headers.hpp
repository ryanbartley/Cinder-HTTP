//
//  headers.hpp
//  Manifold
//
//  Created by Ryan Bartley on 7/9/16.
//
//

#pragma once

#include <string>
#include <vector>

#include "cinder/Base64.h"
#include "cinder/Log.h"

namespace cinder { namespace http {
	
struct BasicAuthorization {
	BasicAuthorization( std::string name, std::string password )
	: name( std::move( name ) ), password( std::move( password ) ) {}
	
	std::string value() const { return "Basic " + ci::toBase64( name + ":" + password ); }
	static const char* key() { return "Authorization"; }
	
private:
	std::string name, password;
};
	
struct Accept {
	Accept( std::string types ) : types( std::move( types ) ) {}
	Accept() : types( "*/*" ) {}
	
	std::string value() const { return types; }
	static const char* key() { return "Accept"; }
	
private:
	std::string types;
};
	
struct Connection {
	enum class Type {
		CLOSE,
		KEEP_ALIVE
	};
	
	Connection( Type type ) : type( type ) {}
	Connection() : type( Type::CLOSE ) {}
	
	std::string value() const
	{
		switch( type ) {
			case Type::CLOSE: return "close";
			case Type::KEEP_ALIVE : return "keep-alive";
			default: return "close";
		}
	}
	static const char* key() { return "Connection"; }
	
private:
	Type type;
};
	
struct Content {
	Content( std::string content_type, std::string content )
	: mLength( content.size() ), mType( std::move( content_type ) ),
		mContent( Buffer::create( content.size() ) )
	{
		memcpy( mContent->getData(), content.data(), mContent->getSize() );
	}
	Content( std::string content_type, ci::BufferRef content )
	: mLength( content->getSize() ), mType( std::move( content_type ) ),
		mContent( content )
	{}
	
	struct Length {
		Length( size_t length ) : length( length ) {}
		std::string value() const { return std::to_string( length ); }
		static const char* key() { return "Content-Length"; }
	private:
		size_t length;
	};
	
	struct Type {
		Type( std::string type ) : type( std::move( type ) ) {}
		std::string value() const { return type; }
		static const char* key() { return "Content-Type"; }
	private:
		std::string type;
	};
	
	const Length& length() const { return mLength; }
	const Type& type() const { return mType; }
	const ci::BufferRef& content() const { return mContent; }
	
private:
	Length			mLength;
	Type			mType;
	ci::BufferRef	mContent;
};
	
struct TransferEncoding {
	enum class Type {
		CHUNKED,
		COMPRESS,
		DEFLATE,
		GZIP,
		IDENTITY
	};
	
	TransferEncoding( Type type ) : type( type ) {}
	
	static const char* key() { return "Transfer-Encoding"; }
	std::string value() {
		switch( type ) {
			case Type::CHUNKED: return "chunked";
			case Type::COMPRESS : return "compress";
			case Type::DEFLATE: return "deflate";
			case Type::GZIP: return "gzip";
			case Type::IDENTITY: return "identity";
		}
	}
private:
	Type type;
};
	
struct HeaderSet {
	using Header = std::pair<std::string, std::string>;
	using Headers = std::vector<Header>;
	HeaderSet() = default;
	
	//! Returns a const ref to the headers attached to this request
	const Headers& getHeaders() const { return headers; }
	Headers& getHeaders() { return headers; }
	//! Adds /a header to the set of headers with /a headerValue, doesn't replace
	void appendHeader( const std::string &header, const std::string &headerValue );
	//! Changes the value of /a header to /a headerValue
	void changeHeader( std::string header, std::string headerValue );
	
	template<typename T>
	void appendHeader( T header );
	
	const Header* findHeader( const std::string &headerKey ) const;
	const Header* findHeader( const char *headerKey ) const;
	
	//! Returns a const ref to the content attached to this request
	const ci::BufferRef& getContent() const { return content; }
	ci::BufferRef& getContent() { return content; }
	//! Sets content for this request, creating a header for Content-Type and Content-Length
	//! as well as setting the content
	
private:
	
	Headers			headers;
	ci::BufferRef	content;
	
	friend std::ostream& operator<<( std::ostream &stream, const HeaderSet &headers );
};

template<typename T>
inline void HeaderSet::appendHeader( T header )
{
	auto endIt = end( headers );
	auto foundIt = std::lower_bound( begin( headers ), endIt, T::key(),
	[]( const Header &a, const char *key ){
		return strcmp( a.first.c_str(), key ) < 0;
	});
	if( foundIt != endIt ) {
		if( strcmp( foundIt->first.c_str(), T::key() ) == 0 ) {
			CI_LOG_I( "Header: " << T::key() << " exists, changing value" );
			foundIt->second = header.value();
		}
		else
			headers.emplace( foundIt, std::make_pair( std::string( T::key() ), header.value() ) );
 	}
	else
		headers.emplace_back( T::key(), header.value() );
}
	
inline const HeaderSet::Header* HeaderSet::findHeader( const std::string &header ) const
{
	return findHeader( header.c_str() );
}
	
inline void HeaderSet::appendHeader( const std::string &header, const std::string &headerValue )
{
	auto endIt = end( headers );
	auto foundIt = std::lower_bound( begin( headers ), endIt, header.c_str(),
	[]( const Header &a, const char *key ){
		return strcmp( a.first.c_str(), key ) < 0;
	});
	if( foundIt != endIt ) {
		if( foundIt->first == header ) {
			CI_LOG_I( "Header: " << headerValue << " exists, changing value" );
			foundIt->second = headerValue;
		}
		else
			headers.emplace( foundIt, std::make_pair( header, headerValue ) );
	}
	else
		headers.emplace_back( header, headerValue );
}
	
inline const HeaderSet::Header* HeaderSet::findHeader( const char *header ) const
{
	auto endIt = end( headers );
	auto foundIt = std::lower_bound( begin( headers ), endIt, header,
	[]( const Header &a, const char *key ){
		return strcmp( a.first.c_str(), key ) < 0;
	});
	if( foundIt != endIt ) {
		auto &foundHeader = foundIt->first;
		if( ! strcmp( foundHeader.c_str(), header ) )
			return &(*foundIt);
		else
			return nullptr;
	}
	else
		return nullptr;
}
	
template<>
inline void HeaderSet::appendHeader( Content header )
{
	appendHeader( header.length() );
	appendHeader( header.type() );
	content = header.content();
}
	
inline std::ostream& operator<<( std::ostream &stream, const HeaderSet &headers )
{
	for( auto & header : headers.headers ) {
		stream << header.first << ": " << header.second << "\r\n";
	}
	return stream;
}

}}
