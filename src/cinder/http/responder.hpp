//
//  responder.hpp
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
#include "parsers.hpp"
#include "error_codes.hpp"
#include "request_response.hpp"
#include "redirector.hpp"
#include <zlib.h>

namespace cinder {
namespace http {
	
namespace detail {

using iterator = asio::buffers_iterator<asio::streambuf::const_buffers_type>;

struct ReadChunkHeaderMatchCondition {
    std::pair<iterator, bool> operator()( iterator begin, iterator end ) {
        for( auto i = begin; i != end - 1; ++i ) {
            if( *i == '\r'  && *(i + 1) == '\n' )
                return { i + 2, true };
        }
        return { begin, false };
    }
};

struct ReadChunkMatchCondition {
    ReadChunkMatchCondition( size_t chunkSize ) : mChunkSize( chunkSize ) {}
    std::pair<iterator, bool> operator()( iterator begin, iterator end ) {
        auto dist = std::distance( begin, end );
        if( dist > mChunkSize + 2 ) {
            auto endChunkIter = begin + mChunkSize;
            CI_ASSERT( *(endChunkIter) == '\r' && *(endChunkIter + 1) == '\n' );
            return { endChunkIter + 3, true };
        }
        return { begin, false };
    }

private:
    size_t mChunkSize;
};

template<typename SessionType>
struct Responder : std::enable_shared_from_this<Responder<SessionType>> {
	Responder( std::shared_ptr<SessionType> session );
	
	void read();
private:
	void on_read_status( asio::error_code ec, size_t lengthRead );
	void on_read_headers( asio::error_code ec, size_t lengthRead );
	void on_read_content( asio::error_code ec, size_t lengthRead );
	// chunk reading
    void on_read_chunk_header( asio::error_code ec, size_t lengthRead );
	void on_read_chunk( asio::error_code ec, size_t lengthRead );
	void on_finalize_chunks( asio::error_code ec, size_t lengthRead );
	
	void read_transferred_data( size_t bytes_transferred );
	void finalize_buffer();
	void extract_buffer();
	
	std::shared_ptr<SessionType>	mSession;
	asio::streambuf					mReplyBuffer;
	ResponseRef						mResponse;
	std::vector<uint8_t>			contentBuffer;
	size_t							writeHead{0},
									content_length{0},
									current_chunk_length{0};
	bool							content_encoded{false};
};

template<typename SessionType>
Responder<SessionType>::Responder( std::shared_ptr<SessionType> session )
: mSession( std::move( session ) ), mResponse( std::make_shared<Response>() )
{
	mSession->response = mResponse;
}
	
template<typename SessionType>
inline void Responder<SessionType>::read()
{
	asio::async_read_until( mSession->socket, mReplyBuffer, "\r\n",
						    std::bind( &Responder<SessionType>::on_read_status,
									   this->shared_from_this(),
									   std::placeholders::_1,
									   std::placeholders::_2 ) );
}
	
template<typename SessionType>
void Responder<SessionType>::read_transferred_data( size_t bytes_transferred )
{
	// Write all of the data that has been read so far.
	auto begIt = asio::buffers_begin( mReplyBuffer.data() );
	// Copy out all data
	contentBuffer.insert( contentBuffer.end(), begIt, begIt + bytes_transferred );
	// Consume the response buffer
	mReplyBuffer.consume(bytes_transferred);
	writeHead += bytes_transferred;
}

template<typename SessionType>
void Responder<SessionType>::finalize_buffer()
{
	// Write all of the data that has been read so far.
	auto begIt = asio::buffers_begin( mReplyBuffer.data() );
	auto endIt = asio::buffers_end( mReplyBuffer.data() );
	// Copy out all data
	contentBuffer.insert( contentBuffer.end(), begIt, endIt );
	// Consume the response buffer
	mReplyBuffer.consume(endIt - begIt);
	
	extract_buffer();
}
	
template<typename SessionType>
void Responder<SessionType>::extract_buffer()
{
	if( content_encoded ) {
		auto encoded_buf = ci::Buffer( contentBuffer.data(), contentBuffer.size() );
		auto header = mResponse->headerSet.findHeader( Content::Encoding::key() );
		CI_ASSERT( header );
		auto &val = header->second;
		auto &buf = mResponse->getContent();
		buf = std::make_shared<ci::Buffer>( std::move( ci::decompressBuffer( encoded_buf, true, val == "gzip" ) ) );
	}
	else {
		auto &buf = mResponse->getContent();
		auto size = contentBuffer.size();
		buf = ci::Buffer::create( size );
		memcpy( buf->getData(), contentBuffer.data(), size );
	}
}
	
template<typename SessionType>
void Responder<SessionType>::on_read_status( asio::error_code ec, size_t bytes_transferred )
{
	if( ! ec )
	// Check the response code to see if we got the page correctly.
	{
		auto begIt = asio::buffers_begin( mReplyBuffer.data() );
		if (!urdl::detail::parse_http_status_line( begIt, begIt + bytes_transferred,
												   mResponse->versionMajor,
												   mResponse->versionMinor,
												   mResponse->statusCode ) )
		{
			ec = http::errc::malformed_status_line;
			mSession->socket.get_io_service().post(
				std::bind( &SessionType::onError, mSession, ec ) );
			return;
		}
		// Consume read bytes
		mReplyBuffer.consume( bytes_transferred );
		// TODO: figure out what he was doing here
		// A "continue" header means we need to keep waiting.
		if ( mResponse->statusCode != http::errc::continue_request ) {
			// Read list of headers and save them. If there's anything left in the
			// reply buffer afterwards, it's the start of the content returned by the
			// HTTP server.
			asio::async_read_until( mSession->socket, mReplyBuffer, "\r\n\r\n",
								    std::bind( &Responder<SessionType>::on_read_headers,
											   this->shared_from_this(),
											   std::placeholders::_1,
											   std::placeholders::_2 ) );
		}
	}
	else
		mSession->socket.get_io_service().post(
			std::bind( &SessionType::onError, mSession, ec ) );
}
	
template<typename SessionType>
void Responder<SessionType>::on_read_headers( asio::error_code ec, size_t bytes_transferred )
{
	std::string headers;
	headers.resize( bytes_transferred );
	mReplyBuffer.sgetn(&headers[0], bytes_transferred);
	if ( ! ec ) {
		// Parse the headers to get Content-Type and Content-Length.
		if (!urdl::detail::parse_http_headers( headers.begin(), headers.end(),
											   mResponse->headerSet.getHeaders() ) )
		{
			ec = http::errc::malformed_response_headers;
			mSession->socket.get_io_service().post(
				std::bind( &SessionType::onError, mSession, ec ) );
			return;
		}
		
		// Check the response code to see if we got the page correctly.
		if (mResponse->statusCode != http::errc::ok)
			ec = make_error_code(static_cast<http::errc::errc_t>(mResponse->statusCode));
		
		auto &headerSet = mResponse->headerSet.getHeaders();
		std::sort( begin( headerSet ), end( headerSet ),
				  []( const HeaderSet::Header &a, const HeaderSet::Header &b ) {
					  return a.first < b.first;
				  });
		
		content_encoded = mResponse->headerSet.findHeader( Content::Encoding::key() ) != nullptr;
		
		if( ! ec ) {
			if( auto contentLengthHeader = mResponse->headerSet.findHeader( Content::Length::key() ) ) {
				content_length = atoi(contentLengthHeader->second.c_str());
				if( content_length > 0 )
					contentBuffer.reserve( content_length );
				asio::async_read( mSession->socket, mReplyBuffer,
								asio::transfer_all(),
								std::bind( &Responder<SessionType>::on_read_content,
										  this->shared_from_this(),
										  std::placeholders::_1,
										  std::placeholders::_2 ));
			}
			else if( auto transferEncoding = mResponse->headerSet.findHeader( TransferEncoding::key() ) ) {
                std::function<std::pair<iterator, bool>( iterator, iterator )> match = ReadChunkHeaderMatchCondition();
				asio::async_read_until( mSession->socket, mReplyBuffer, match,
							  std::bind( &Responder<SessionType>::on_read_chunk_header,
										this->shared_from_this(),
										std::placeholders::_1,
										std::placeholders::_2 ));
			}
			else {
				asio::async_read( mSession->socket, mReplyBuffer,
							  asio::transfer_at_least(1),
							  std::bind( &Responder<SessionType>::on_read_content,
										this->shared_from_this(),
										std::placeholders::_1,
										std::placeholders::_2 ));
			}
		}
	}
	
	if( ec.value() >= 300 && ec.value() < 400  )
		std::make_shared<Redirector<SessionType>>( mSession )->redirect();
	else if( ec )
		mSession->socket.get_io_service().post(
			std::bind( &SessionType::onError, mSession, ec ) );
}
	
template<typename SessionType>
void Responder<SessionType>::on_read_content( asio::error_code ec, size_t bytes_transferred )
{
	if ( ! ec ) {
		read_transferred_data( bytes_transferred );
		// Continue reading remaining data until EOF.
		asio::async_read( mSession->socket, mReplyBuffer,
						  asio::transfer_at_least(1),
						  std::bind( &Responder<SessionType>::on_read_content,
									 this->shared_from_this(),
									 std::placeholders::_1,
									 std::placeholders::_2 ));
	}
	else if ( ec == asio::error::eof ) {
		finalize_buffer();
		mSession->socket.get_io_service().post(
			std::bind( &SessionType::onResponse, mSession, ec ) );
	}
#if defined( USING_SSL )
	// TODO: this is super hacky because there isn't a definition for short read and most are
	// supposed to be ignored, new asio fixes this.
	else if( ec.value() == 335544539 && bytes_transferred > 0 ) {
		read_transferred_data( bytes_transferred );
		// Continue reading remaining data until EOF.
		asio::async_read( mSession->socket, mReplyBuffer,
						 asio::transfer_at_least(1),
						 std::bind( &Responder<SessionType>::on_read_content,
								   this->shared_from_this(),
								   std::placeholders::_1,
								   std::placeholders::_2 ));
	}
	else if( ec.value() == 335544539 ) {
		finalize_buffer();
		mSession->socket.get_io_service().post(
			std::bind( &SessionType::onResponse, mSession, ec ) );
	}
#endif
	else {
		mSession->socket.get_io_service().post(
			std::bind( &SessionType::onError, mSession, ec ) );
	}
}

template<typename SessionType>
void Responder<SessionType>::on_read_chunk_header( asio::error_code ec, size_t bytes_transferred )
{
	if ( ! ec ) {
		// Write all of the data that has been read so far.
		auto begIt = asio::buffers_begin( mReplyBuffer.data() );
		std::string bytes( begIt, begIt + bytes_transferred );
		auto hex = bytes.substr( 0, bytes.find( "\r\n" ) );
		std::stringstream ss;
		ss << std::hex << hex;
		ss >> current_chunk_length;
		mReplyBuffer.consume( bytes_transferred );
		if( current_chunk_length != 0 ) {
			// Continue reading remaining data until EOF.
            std::function<std::pair<iterator, bool>( iterator, iterator )> match = ReadChunkMatchCondition( current_chunk_length );
			asio::async_read_until( mSession->socket, mReplyBuffer, match,
							 std::bind( &Responder<SessionType>::on_read_chunk,
									   this->shared_from_this(),
									   std::placeholders::_1,
									   std::placeholders::_2 ));
		}
		else {
			// chunk is done.
			asio::async_read_until( mSession->socket, mReplyBuffer, "\r\n",
								   std::bind( &Responder<SessionType>::on_finalize_chunks,
									   this->shared_from_this(),
									   std::placeholders::_1,
									   std::placeholders::_2 ));
		}
	}
	else {
		mSession->socket.get_io_service().post(
			std::bind( &SessionType::onError, mSession, ec ) );
	}
}
	
template<typename SessionType>
void Responder<SessionType>::on_read_chunk( asio::error_code ec, size_t bytes_transferred )
{
	if ( ! ec ) {
		// Write all of the data that has been read so far.
		auto begIt = asio::buffers_begin(mReplyBuffer.data());
		if (current_chunk_length != bytes_transferred - 3)
			CI_LOG_W("current_chunk_length: " << current_chunk_length << ", doesn't match bytes_transferred: "
											  << bytes_transferred);
		// Copy out all data
		contentBuffer.insert( contentBuffer.end(), begIt, begIt + current_chunk_length );
		// Consume the response buffer
		mReplyBuffer.consume( current_chunk_length + 2 );
		// Continue reading remaining data until EOF.
        std::function<std::pair<iterator, bool>( iterator, iterator )> match = ReadChunkHeaderMatchCondition();
		asio::async_read_until( mSession->socket, mReplyBuffer, match,
							   std::bind( &Responder<SessionType>::on_read_chunk_header,
										 this->shared_from_this(),
										 std::placeholders::_1,
										 std::placeholders::_2 ));
	}
	else {
		mSession->socket.get_io_service().post(
			std::bind( &SessionType::onError, mSession, ec ) );
	}
}
	
template<typename SessionType>
void Responder<SessionType>::on_finalize_chunks( asio::error_code ec, size_t bytes_transferred )
{
	if ( ! ec ) {
		// Write all of the data that has been read so far.
		if( bytes_transferred != 2 )
			CI_LOG_W( "In finalize and it's not 2 bytes. Instead, " << bytes_transferred );
		mReplyBuffer.consume(bytes_transferred);
		// Call extract buffer to make sure we don't try to copy the last 2 bytes.
		// Ugly but works. TODO: Check other options about finalizing buffers.
		extract_buffer();
		
		mSession->socket.get_io_service().post(
			std::bind( &SessionType::onResponse, mSession, ec ) );
		
	}
#if defined( USING_SSL )
	else if( ec == asio::error::eof /*|| ec == asio::ssl::error::stream_truncated*/ ) {
		finalize_buffer();
		mSession->socket.get_io_service().post(
			std::bind( &SessionType::onResponse, mSession, ec ) );
	}
#endif
	else {
		mSession->socket.get_io_service().post(
			std::bind( &SessionType::onError, mSession, ec ) );
	}
}
	
} // detail
} // http
} // cinder
