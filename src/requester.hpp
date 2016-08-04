//
//  requester.hpp
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
#include "request_response.hpp"
#include "asio/asio.hpp"

namespace cinder {
namespace http { namespace detail {
	
template<typename SessionType>
struct Requester : std::enable_shared_from_this<Requester<SessionType>> {
public:
	Requester( std::shared_ptr<SessionType> session, RequestRef request )
	: mSession( session ), mRequest( std::move( request ) ) {}
	
	void request()
	{
		std::ostream request_stream( &mRequestBuffer );
		mRequest->process( request_stream );
		
		asio::async_write(this->mSession->socket, mRequestBuffer,
						  asio::transfer_all(),
						  std::bind( &Requester<SessionType>::on_request,
									this->shared_from_this(),
									std::placeholders::_1 ) );
	}
	
private:
	void on_request( asio::error_code ec )
	{
		if( !ec ) {
			mSession->socket.get_io_service().post(
				std::bind( &SessionType::onRequest, mSession, ec ) );
		}
		else
			mSession->socket.get_io_service().post(
				std::bind( &SessionType::onError, mSession, ec ) );
	}
	
	std::shared_ptr<SessionType>	mSession;
	asio::streambuf					mRequestBuffer;
	RequestRef						mRequest;
};
	
} // detail
} // http
} // cinder
