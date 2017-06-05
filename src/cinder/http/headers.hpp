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
	
// Header representing basic authorization. 
// https://en.wikipedia.org/wiki/Basic_access_authentication#Client_side
struct BasicAuthorization {
	// Constructor taking \a name and \a password
	BasicAuthorization( std::string name, std::string password )
	: name( std::move( name ) ), password( std::move( password ) ) {}
	// Returns std::string of header value.
	std::string value() const { return "Basic " + ci::toBase64( name + ":" + password ); }
	// Returns const char* header key.
	static const char* key() { return "Authorization"; }
	
private:
	std::string name, password;
};
	
// Header representing accept advertising which content types the client accepts.
// https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Accept
struct Accept {
	// Constructor taking \a types that are accepted. Example of types usage:
	// text/html, application/xhtml+xml...
	Accept( std::string types ) : types( std::move( types ) ) {}
	// Default constructor that says the client application accepts everything.
	Accept() : types( "*/*" ) {}
	
	// Returns std::string header value.
	std::string value() const { return types; }
	// Returns const char* header key.
	static const char* key() { return "Accept"; }
	
private:
	std::string types;
};
	
// Header representing encodings that client application accepts.
// https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Accept-Encoding
struct AcceptEncoding {
	// Encoding Types
	enum class Type {
		COMPRESS,
		DEFLATE,
		GZIP,
		IDENTITY
	};
	// Constructor taking \a type as acceptable encoding type. 
	AcceptEncoding( Type type ) : type( type ) {}
	// Returns std::string header value.
	std::string value() {
		switch( type ) {
			case Type::COMPRESS : return "compress";
			case Type::DEFLATE: return "deflate";
			case Type::GZIP: return "gzip";
			case Type::IDENTITY: return "identity";
			default: return "identity";
		}
	}
	// Returns const char* header key.
	static const char* key() { return "Accept-Encoding"; }
private:
	Type type;
};
	
// Header describing the connection type to the server.
// https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Connection
struct Connection {
	// Connection Type enum.
	enum class Type {
		CLOSE,
		KEEP_ALIVE
	};
	// Constructor taking \a type.
	Connection( Type type ) : type( type ) {}
	// Constructor defaulting to close type connection.
	Connection() : type( Type::CLOSE ) {}
	// Returns std::string header value.
	std::string value() const
	{
		switch( type ) {
			case Type::CLOSE: return "close";
			case Type::KEEP_ALIVE : return "keep-alive";
			default: return "close";
		}
	}
	// Returns const char* header key.
	static const char* key() { return "Connection"; }
	
private:
	Type type;
};

// Forward declaration.
struct MultipartFormData;
	
// Represents and encapsulates multiple related headers.
struct Content {
	// Represents Content-Length header.
	// https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Content-Length
	struct Length {
		// Constructor taking \a length of the content.
		Length( size_t length ) : length( length ) {}
		// Returns std::string header value.
		std::string value() const { return std::to_string( length ); }
		// Returns const char* header key.
		static const char* key() { return "Content-Length"; }
	private:
		size_t length;
	};
	
	// Represents Content-Type header.
	// https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Content-Type
	struct Type {
		// Constructor taking \a type as a std::string.
		Type( std::string type ) : type( std::move( type ) ) {}
		// Returns std::string header value.
		std::string value() const { return type; }
		// Returns const char* header key.
		static const char* key() { return "Content-Type"; }
	private:
		std::string type;
	};
	
	// Represents Content-Encoding header.
	// https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Content-Encoding
	struct Encoding {
		// Content-Encoding Type enum, representing the type of encoding.
		enum class Type {
			COMPRESS,
			DEFLATE,
			GZIP,
			IDENTITY
		};
		// Constructor taking the \a type of encoding.
		Encoding( Type type ) : type( type ) {}
		// Returns std::string header value.
		std::string value() {
			switch( type ) {
				case Type::COMPRESS : return "compress";
				case Type::DEFLATE: return "deflate";
				case Type::GZIP: return "gzip";
				case Type::IDENTITY: return "identity";
			}
		}
		// Returns const char* header key.
		static const char* key() { return "Content-Encoding"; }
	private:
		Type type;
	};

	struct Language {
		
	};
	
	// Constructor taking \a content_type and \a content as std::string. Length initialized as the size 
	// of \a content. Encoding defaults to IDENTITY.
	Content( std::string content_type, std::string content )
	: mLength( content.size() ), mType( std::move( content_type ) ),
		mContent( Buffer::create( content.size() ) ), mEncoding( Encoding::Type::IDENTITY )
	{
		memcpy( mContent->getData(), content.data(), mContent->getSize() );
	}

	Content( std::string content_type, ci::BufferRef content )
	: mLength( content->getSize() ), mType( std::move( content_type ) ),
		mContent( content ), mEncoding( Encoding::Type::IDENTITY )
	{}
	Content( std::string content_type, ci::BufferRef content, Encoding::Type encodingType )
	: mLength( content->getSize() ), mType( std::move( content_type ) ),
		mContent( content ), mEncoding( encodingType )
	{}
	// Constructo taking MultipartFormData as \a multipartFormData.
	Content( const MultipartFormData &multipartFormData );
	
	// Returns const ref of underlying Length member.
	const Length& length() const { return mLength; }
	// Returns const ref of underlying Type member.
	const Type& type() const { return mType; }
	// Returns const ref of undelying Encoding member.
	const Encoding& encoding() const { return mEncoding; }
	// Returns const ref of underlying content Buffer.
	const ci::BufferRef& content() const { return mContent; }
	
private:
	Length			mLength;
	Type			mType;
	Encoding		mEncoding;
	ci::BufferRef	mContent;
};
	
struct Location {
	Location( const std::string &location ) : mLocation( location ) {}
	static const char* key() { return "Location"; }
	std::string value() { return mLocation; }
private:
	std::string mLocation;
};

struct MultipartFormData {
	MultipartFormData( std::string delimiter )
	: delimiter( move( delimiter ) )
	{
	}
	MultipartFormData( const MultipartFormData &other ) = delete;
	MultipartFormData& operator=( const MultipartFormData &other ) = delete;
	MultipartFormData( MultipartFormData &&other ) noexcept = default;
	MultipartFormData& operator=( MultipartFormData &&other ) noexcept = default;
	
	struct Part {
		Part() = default;
		Part( const Part & other ) = delete;
		Part& operator=( const Part &other ) = delete;
		Part( Part &&other ) noexcept = default;
		Part& operator=( Part &&other ) noexcept = default;
		
		//! Set header of this part with \a key and \a value.
		void appendHeader( const std::string &key, const std::string &value )
		{
			headers.emplace_back( key, value );
		}
		//! Sets header of type T.
		template<typename T>
		void appendHeader( T header )
		{
			headers.emplace_back( T::key(), header.value() );
		}
		//! Sets the body as a string.
		void setBody( const std::string &body )
		{
			data.resize( body.size() );
			memcpy( data.data(), body.data(), body.size() );
		}
		//! Sets the body as a buffer.
		void setBody( const ci::Buffer &buf )
		{
			data.resize( buf.getSize() );
			memcpy( data.data(), buf.getData(), buf.getSize() );
		}
		
		std::vector<std::pair<std::string, std::string>>	headers;
		std::vector<uint8_t>								data;
	};
	//! Append a part of the multipart data to the back.
	void appendPart( Part &&part )
	{
		parts.emplace_back( std::move( part ) );
	}
	
	std::string			delimiter;
	std::vector<Part>	parts;
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
	
Content::Content( const MultipartFormData &data )
: mType( "multipart/form-data; boundary=" + data.delimiter ),
	mEncoding( Encoding::Type::IDENTITY ), mLength( 0 )
{
	static const std::string delimBegin = "\r\n--";
	static const std::string delimEnd = "\r\n";
	static const std::string headerSplit = ": ";
	static const std::string headerEnd = "\r\n\r\n";
	static const std::string finalDelim = "--\r\n\r\n";
	
	const auto &delimiter = data.delimiter;
	const auto &parts = data.parts;
	// calculate final size.
	uint64_t size = 0;
	for( const auto &part : parts ) {
		size += delimBegin.size();
		size += delimiter.size();
		size += delimEnd.size();
		for( const auto &header : part.headers ) {
			size += header.first.size();
			size += headerSplit.size();
			size += header.second.size();
		}
		size += headerEnd.size();
		size += part.data.size();
	}
	size += delimBegin.size();
	size += delimiter.size();
	size += finalDelim.size();
	mLength = Length( size );
	
	mContent = ci::Buffer::create( size );
	// copy parts in.
	auto dataPtr = reinterpret_cast<uint8_t*>( mContent->getData() );
	for( const auto &part : parts ) {
		// add delimiter
		memcpy( dataPtr, delimBegin.data(), delimBegin.size() );
		dataPtr += delimBegin.size();
		memcpy( dataPtr, delimiter.data(), delimiter.size() );
		dataPtr += delimiter.size();
		memcpy( dataPtr, delimEnd.data(), delimEnd.size() );
		dataPtr += delimEnd.size();
		// add headers
		for( const auto &header : part.headers ) {
			memcpy( dataPtr, header.first.data(), header.first.size() );
			dataPtr += header.first.size();
			memcpy( dataPtr, headerSplit.data(), headerSplit.size() );
			dataPtr += headerSplit.size();
			memcpy( dataPtr, header.second.data(), header.second.size() );
			dataPtr += header.second.size();
		}
		memcpy( dataPtr, headerEnd.data(), headerEnd.size() );
		dataPtr += headerEnd.size();
		// add data
		memcpy( dataPtr, part.data.data(), part.data.size() );
		dataPtr += part.data.size();
	}
	memcpy( dataPtr, delimBegin.data(), delimBegin.size() );
	dataPtr += delimBegin.size();
	memcpy( dataPtr, delimiter.data(), delimiter.size() );
	dataPtr += delimiter.size();
	memcpy( dataPtr, finalDelim.data(), finalDelim.size() );
}

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
