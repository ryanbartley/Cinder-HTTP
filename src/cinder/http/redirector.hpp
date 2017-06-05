//
//  redirector.h
//  Post
//
//  Created by Ryan Bartley on 10/26/16.
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

namespace cinder {
namespace http {
namespace detail {

template <typename SessionType>
class Redirector : public std::enable_shared_from_this<Redirector<SessionType>> {
public:
	Redirector( std::shared_ptr<SessionType> session, asio::error_code originatingError );
	
	void redirect();
private:
	std::shared_ptr<SessionType>	mSession;
	asio::error_code				mOriginatingError;
};

template<typename SessionType>
Redirector<SessionType>::Redirector( std::shared_ptr<SessionType> session, asio::error_code originatingError )
: mSession( session ), mOriginatingError( originatingError )
{
}

template<typename SessionType>
void Redirector<SessionType>::redirect()
{
	auto &request = *mSession->request;
	if( request.mMaxRedirects != -1 ) {
		if( mSession->attempted_redirects < request.mMaxRedirects ) {
			mSession->attempted_redirects++;
			mSession->socket().get_io_service().post(
				std::bind( &SessionType::onError, mSession, mOriginatingError ) );
			return;
		}
	}
	auto &response = *mSession->response;
	auto locationHeader = response.getHeaders().findHeader( Location::key() );
	// Todo: this is a hack that needs to be resolved in the headers class where some servers
	// don't send the correctly formatted header title.
	if( ! locationHeader ) {
		locationHeader = response.getHeaders().findHeader( "location" );
	}
		
	CI_ASSERT( locationHeader );
	CI_ASSERT( mSession->response->statusCode >= 300 && mSession->response->statusCode < 400 );
	auto &location = locationHeader->second;
	
	if( location[0] == '/' ) {
		mSession->request->getUrl()->set_path( location );
		auto endpoint = mSession->endpoint;
		mSession = std::make_shared<SessionType>( mSession->request,
												  mSession->responseHandler,
												  mSession->errorHandler,
												  mSession->get_io_service() );
		mSession->start( endpoint );
	}
	else if( location[0] == 'h' ) {
		mSession->request->setUrl( std::make_shared<Url>( location ) );
		mSession = std::make_shared<SessionType>( mSession->request,
												  mSession->responseHandler,
												  mSession->errorHandler,
												  mSession->get_io_service() );
		mSession->start();
	}
}
	
} // detail
} // http
} // cinder
