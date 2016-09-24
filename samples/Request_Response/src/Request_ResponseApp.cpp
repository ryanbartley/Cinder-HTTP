#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

#include "cinder/http/http.hpp"

using namespace ci;
using namespace ci::app;
using namespace std;

class TestApp : public App {
public:
	void setup() override;
	void mouseDown( MouseEvent event ) override;
	void update() override;
	void draw() override;
	
	void makeRequest( std::shared_ptr<http::url> url );
	
	std::shared_ptr<ci::http::Session>		session;
	std::shared_ptr<ci::http::SslSession>	sslSession;
	ci::gl::TextureRef texture;
	std::shared_ptr<ci::http::url> httpUrl, httpsUrl;
	bool useHttp = false; 
};

void TestApp::setup()
{
	httpUrl = std::make_shared<http::url>( "http://www.lingosolutions.co.uk/wp-content/uploads/2016/05/HTTP-wallpaper.jpg" );
	httpsUrl = std::make_shared<http::url>( "https://upload.wikimedia.org/wikipedia/commons/d/da/Internet2.jpg" );
	
	makeRequest( httpUrl );
}

void TestApp::makeRequest( std::shared_ptr<http::url> url )
{
	auto request = std::make_shared<http::Request>( http::RequestMethod::GET, url );
	request->appendHeader( http::Connection( http::Connection::Type::CLOSE ) );
	request->appendHeader( http::Accept() );
	
	auto onComplete = [&]( asio::error_code ec, http::ResponseRef response ) {
		texture = ci::gl::Texture::create( loadImage( ci::DataSourceBuffer::create( response->getContent() ),
													 ImageSource::Options(), ".jpg" ) );
	};
	auto onError = []( asio::error_code ec, const std::shared_ptr<http::url> &url, http::ResponseRef response ){
		CI_LOG_E( ec.message() << " val: " << ec.value() << " Url: " << url->to_string() );
		if( response ) {
			app::console() << "Headers: " << std::endl;
			app::console() << response->getHeaders() << endl;
		}
	};
	
	if( url->port() == 80 ) {
		session = std::make_shared<http::Session>( url, request, onComplete, onError );
		session->start();
	}
	else if( url->port() == 443 ) {
		sslSession = std::make_shared<http::SslSession>( url, request, onComplete, onError );
		sslSession->start();
	}
}

void TestApp::mouseDown( MouseEvent event )
{
	if( useHttp )
		makeRequest( httpUrl );
	else
		makeRequest( httpsUrl );
	useHttp = ! useHttp;
}

void TestApp::update()
{
}

void TestApp::draw()
{
	gl::clear( Color( 0, 0, 0 ) );
	
	gl::setMatricesWindow( getWindowSize() );
	if( texture )
		gl::draw( texture, texture->getBounds(), Rectf( vec2(0.0f), vec2(getWindowSize()) ) );
}

CINDER_APP( TestApp, RendererGl )
