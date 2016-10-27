#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

#include "jsoncpp/json.h"

#include "cinder/http/http.hpp"

using namespace ci;
using namespace ci::app;
using namespace std;

static const std::string cinder_text = "BRIEF TEXT ABOUT CINDER\n"
"\n"
"Cinder is a C++ library for programming with aesthetic intent - the sort of "
"development often called creative coding. This includes domains like graphics, "
"audio, video, and computational geometry. Cinder is cross-platform, with "
"official support for OS X, Windows, iOS, and WinRT.\n"
"\n"
"Cinder is production-proven, powerful enough to be the primary tool for "
"professionals, but still suitable for learning and experimentation.\n"
"\n"
"Cinder is released under the 2-Clause BSD License.\n"
"\n"
"Contributing... Cinder is developed through Github, and discussion is conducted "
"primarily via its forums. Code contributions, issue reports, and support requests "
"are welcome through these two avenues.\n"
"\n"
"Authors... Cinder's original author and current lead architect is Andrew Bell. "
"Significant portions of Cinder were derived from code coauthored with Hai Nguyen, "
"who continues to help steward the project along with Rich Eakin, Paul Houx, and a "
"growing, global community of users.\n"
"\n"
"http://libcinder.org";

class PostApp : public App {
  public:
	void setup() override;
	void mouseDown( MouseEvent event ) override;
	void update() override;
	void draw() override;

	std::shared_ptr<ci::http::Session>		session;
	std::shared_ptr<ci::http::SslSession>	sslSession;
};

void PostApp::setup()
{
	auto url = make_shared<http::Url>( "http://httpbin.org/post" );
	auto request = std::make_shared<http::Request>( http::RequestMethod::POST, url );
	request->appendHeader( http::Connection( http::Connection::Type::CLOSE ) );
	request->appendHeader( http::Accept() );

	request->appendHeader( http::Content( std::string( "text/plain; charset=utf-8" ), cinder_text ) );

	auto onComplete = [&]( asio::error_code ec, http::ResponseRef response ) {
		if( response ) {
			app::console() << "Headers: " << std::endl;
			app::console() << response->getHeaders() << endl;
			app::console() << "Content: " << std::endl;
			auto value = response->getContentAs<Json::Value>();
	
			CI_LOG_I( value.toStyledString() );
		}
	};
	auto onError = []( asio::error_code ec, const http::UrlRef &url, http::ResponseRef response ) {
		CI_LOG_E( ec.message() << " val: " << ec.value() << " Url: " << url->to_string() );
		if( response ) {
			app::console() << "Headers: " << std::endl;
			app::console() << response->getHeaders() << endl;
		}
	};

	if( url->port() == 80 ) {
		session = std::make_shared<http::Session>( request, onComplete, onError );
		session->start();
	}
	else if( url->port() == 443 ) {
		sslSession = std::make_shared<http::SslSession>( request, onComplete, onError );
		sslSession->start();
	}
}

void PostApp::mouseDown( MouseEvent event )
{
}

void PostApp::update()
{
}

void PostApp::draw()
{
	gl::clear( Color( 0, 0, 0 ) ); 
}

CINDER_APP( PostApp, RendererGl )
