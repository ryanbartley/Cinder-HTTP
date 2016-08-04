#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

#include "http.hpp"

using namespace ci;
using namespace ci::app;
using namespace std;

class TestApp : public App {
  public:
	void setup() override;
	void mouseDown( MouseEvent event ) override;
	void update() override;
	void draw() override;
	
	std::shared_ptr<ci::http::Session> session;
	ci::gl::TextureRef texture;
};

void TestApp::setup()
{
	auto url = std::make_shared<http::url>( "http://cdn.tutsplus.com/net/authors/jeremymcpeak/http2-http.jpg" );
	
	auto request = std::make_shared<http::Request>( http::RequestMethod::GET, url );
	request->appendHeader( http::Connection( http::Connection::Type::CLOSE ) );
	request->appendHeader( http::Accept() );
	
	session = std::make_shared<http::Session>( url, request,
	[&]( asio::error_code ec, http::Response response ){
		texture = ci::gl::Texture::create( loadImage( ci::DataSourceBuffer::create( response.getContent() ),
													 ImageSource::Options(), ".jpg" ) );
	},
	[]( asio::error_code ec, http::Process process, const std::shared_ptr<http::url> &url, http::Response response ){
		 CI_LOG_E( ec.message() << " Process: " << static_cast<uint32_t>( process ) << " Url: " << url->to_string() );
	});
	//	asio::ip::tcp::endpoint endpoint( asio::ip::address::from_string( "192.168.0.12" ), 8000 );
	session->start();
}

void TestApp::mouseDown( MouseEvent event )
{
}

void TestApp::update()
{
}

void TestApp::draw()
{
	gl::clear( Color( 0, 0, 0 ) );
	
	gl::setMatricesWindow( getWindowSize() );
	if( texture )
		gl::draw( texture );
}

CINDER_APP( TestApp, RendererGl )
