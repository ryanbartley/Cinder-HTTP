//
//  LoggerHttp.cpp
//  Request_Response
//
//  Created by Ryan Bartley on 2/21/17.
//
//

#include "LoggerHttp.hpp"

using namespace ci;
using namespace std;

namespace cinder { namespace log {

void LoggerHttp::write( const Metadata &meta, const std::string &text )
{
//	std::stringstream stream;
//	stream << meta.mLevel << " ";
	
//	if( isTimestampEnabled() )
//		stream << getCurrentDateTimeString() << " ";
	
//	stream << meta.mLocation << " " << text << endl;
	mLogQueues[mCurrentlyWriting].emplace_back( meta, text );
}
	
}}
