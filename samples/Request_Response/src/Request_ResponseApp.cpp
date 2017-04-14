#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

#define USING_SSL = 1 // Comment out to disable SSL
#include "cinder/http/http.hpp"

using namespace ci;
using namespace ci::app;
using namespace std;

class TestApp : public App {
public:
	void setup() override;
	void mouseDown(MouseEvent event) override;
	void update() override;
	void draw() override;

	void makeRequest(http::UrlRef url);

	std::shared_ptr<ci::http::Session>		session;
	std::shared_ptr<ci::http::SslSession>	sslSession; // Comment out if USING_SSL not enabled
	ci::gl::TextureRef texture;
	http::UrlRef							httpUrl, httpsUrl;
	bool useHttp = false;
};

void TestApp::setup()
{
	httpUrl = std::make_shared<http::Url>("http://www.lingosolutions.co.uk/wp-content/uploads/2016/05/HTTP-wallpaper.jpg");
	httpsUrl = std::make_shared<http::Url>("https://upload.wikimedia.org/wikipedia/commons/d/da/Internet2.jpg");

	makeRequest(httpUrl);
}

void TestApp::makeRequest(http::UrlRef url)
{
	auto request = std::make_shared<http::Request>(http::RequestMethod::GET, url);
	request->appendHeader(http::Connection(http::Connection::Type::CLOSE));
	request->appendHeader(http::Accept());

	auto onComplete = [&](asio::error_code ec, http::ResponseRef response) {
		texture = ci::gl::Texture::create(response->getContentAs<ci::Surface>());
	};
	auto onError = [](asio::error_code ec, const http::UrlRef &url, http::ResponseRef response) {
		CI_LOG_E(ec.message() << " val: " << ec.value() << " Url: " << url->to_string());
		if (response) {
			app::console() << "Headers: " << std::endl;
			app::console() << response->getHeaders() << endl;
		}
	};

	if (url->port() == 80) {
		session = std::make_shared<http::Session>(request, onComplete, onError);
		session->start();
	}

	// Comment out if USING_SSL not enabled
	else if (url->port() == 443) {
		sslSession = std::make_shared<http::SslSession>(request, onComplete, onError);
		sslSession->start();
	}
}

void TestApp::mouseDown(MouseEvent event)
{
	if (useHttp)
		makeRequest(httpUrl);
	else
		makeRequest(httpsUrl);
	useHttp = !useHttp;
}

void TestApp::update()
{
}

void TestApp::draw()
{
	gl::clear(Color(0, 0, 0));

	gl::setMatricesWindow(getWindowSize());
	if (texture)
		gl::draw(texture, texture->getBounds(), Rectf(vec2(0.0f), vec2(getWindowSize())));
}

CINDER_APP(TestApp, RendererGl)