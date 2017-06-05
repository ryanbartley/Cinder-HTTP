//
//  LoggerHttp.hpp
//  Request_Response
//
//  Created by Ryan Bartley on 2/21/17.
//
//

#pragma once

#include "cinder/Log.h"
#include "cinder/http/http.hpp"

namespace cinder { namespace log {

//! \brief LoggerFile will write log messages to a specified file.
//!
//! LoggerFile will write to a specified file, either appending to or overwriting that file at application startup.
class LoggerHttp : public Logger {
public:
	//! LoggerFile writes to a single log file.  File appending is configurable.
	//! If \p filePath is empty, uses the default ('%cinder.log' next to app binary)
	LoggerHttp( http::UrlRef url, asio::io_service &service );
	
	virtual ~LoggerHttp();
	
	void write( const Metadata &meta, const std::string &text ) override;
	
	//! Returns the file path targeted by this logger.
	const ci::http::UrlRef&		getUrl() const		{ return mUrl; }
	
protected:
	
	asio::io_service	&mService;
	ci::http::UrlRef	mUrl;
	std::array<std::vector<std::pair<Metadata, std::string>>, 2> mLogQueues;
	uint32_t		mCurrentlyWriting, mCurrentlySending;
};

}} // log // cinder
