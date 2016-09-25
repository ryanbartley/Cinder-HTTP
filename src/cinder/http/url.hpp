//
// url.hpp
// ~~~~~~~
//
// Copyright (c) 2009-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// path LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef URDL_URL_HPP
#define URDL_URL_HPP

#include <string>
#include <memory>
#include "asio/error_code.hpp"

namespace cinder {
namespace http {

/// The class @c url enables parsing and accessing the components of URLs.
/**
 * @par Example
 * To extract the components of a URL:
 * @code
 * urdl::url url("http://user:pass@host:1234/dir/page?param=0#anchor");
 * std::cout << "Protocol: " << url.protocol() << std::endl;
 * std::cout << "User Info: " << url.user_info() << std::endl;
 * std::cout << "Host: " << url.host() << std::endl;
 * std::cout << "Port: " << url.port() << std::endl;
 * std::cout << "Path: " << url.path() << std::endl;
 * std::cout << "Query: " << url.query() << std::endl;
 * std::cout << "Fragment: " << url.fragment() << std::endl;
 * @endcode
 * The above code will print:
 * @code
 * Protocol: http
 * User Info: user:pass
 * Host: host
 * Port: 1234
 * Path: /dir/page
 * Query: param=0
 * Fragment: anchor
 * @endcode
 *
 * @par Requirements
 * @e Header: @c <urdl/url.hpp> @n
 * @e Namespace: @c urdl
 */
class Url
{
public:
  /// Constructs an object of class @c url.
  /**
   * @par Remarks
   * Postconditions: @c protocol(), @c user_info(), @c host(), @c path(),
   * @c query(), @c fragment() all return an empty string, and @c port() returns
   * 0.
   */
  Url()
    : ipv6_host_(false)
  {
  }

  /// Constructs an object of class @c url.
  /**
   * @param s URL string to be parsed into its components.
   *
   * @throws std::system_error Thrown when the URL string is invalid.
   */
  Url(const char* s)
    : ipv6_host_(false)
  {
    *this = from_string(s);
  }

  /// Constructs an object of class @c url.
  /**
   * @param s URL string to be parsed into its components.
   *
   * @throws std::system_error Thrown when the URL string is invalid.
   */
  Url(const std::string& s)
    : ipv6_host_(false)
  {
    *this = from_string(s);
  }

  /// Gets the protocol component of the URL.
  /**
   * @returns A string specifying the protocol of the URL. Examples include
   * @c http, @c https or @c file.
   */
  std::string protocol() const
  {
    return protocol_;
  }
	
  inline void set_protocol( std::string protocol );
	
  Url& protocol( std::string protocol )
  {
	set_protocol( std::move( protocol ) );
	return *this;
  }

  /// Gets the user info component of the URL.
  /**
   * @returns A string containing the user info of the URL. Typically in the
   * format <tt>user:password</tt>, but depends on the protocol.
   */
  std::string user_info() const
  {
    return user_info_;
  }
	
  void set_user_info( std::string user_info )
  {
	user_info_ = std::move( user_info );
  }
	
  Url& user_info( std::string user_info )
  {
	set_user_info( std::move( user_info ) );
	return *this;
  }

  /// Gets the host component of the URL.
  /**
   * @returns A string containing the host name of the URL.
   */
  std::string host() const
  {
    return host_;
  }
	
  void set_host( std::string host )
  {
	host_ = std::move( host );
  }
	
  Url& host( std::string host )
  {
	set_host( std::move( host ) );
	return *this;
  }

  /// Gets the port component of the URL.
  /**
   * @returns The port number of the URL.
   *
   * @par Remarks
   * If the URL string did not specify a port, and the protocol is one of @c
   * http, @c https or @c ftp, an appropriate default port number is returned.
   */
  inline uint16_t port() const;
	
  void set_port( std::string port )
  {
	port_ = std::move( port );
  }
	
  void set_port( uint16_t port )
  {
	port_ = std::to_string( port );
  }
	
  Url& port( std::string port )
  {
	set_port( std::move( port ) );
	return *this;
  }
	
  Url& port( uint16_t port )
  {
	set_port( port );
	return *this;
  }

  /// Gets the path component of the URL.
  /**
   * @returns A string containing the path of the URL.
   *
   * @par Remarks
   * The path string is unescaped. To obtain the path in escaped form, use
   * @c to_string(url::path_component).
   */
  inline std::string path() const;
	
  void set_path( std::string path )
  {
	path_ = std::move( path );
  }
	
  Url& path( std::string path )
  {
	set_path( std::move( path ) );
	return *this;
  }
	
  inline Url& append_path( std::string path );

  /// Gets the query component of the URL.
  /**
   * @returns A string containing the query string of the URL.
   *
   * @par Remarks
   * The query string is not unescaped, but is returned in whatever form it
   * takes in the original URL string.
   */
  std::string query() const
  {
    return query_;
  }
	
  inline Url& add_query( std::string query );
  inline Url& add_query( std::string key, std::string value );

  /// Gets the fragment component of the URL.
  /**
   * @returns A string containing the fragment of the URL.
   */
  std::string fragment() const
  {
    return fragment_;
  }
	
  void set_fragment( std::string fragment )
  {
	fragment_ = std::move( fragment );
  }
	
  Url& fragment( std::string fragment )
  {
	set_fragment( std::move( fragment ) );
	return *this;
  }

  /// Components of the URL, used with @c from_string.
  enum components_type
  {
    protocol_component = 1,
    user_info_component = 2,
    host_component = 4,
    port_component = 8,
    path_component = 16,
    query_component = 32,
    fragment_component = 64,
    all_components = protocol_component | user_info_component | host_component
      | port_component | path_component | query_component | fragment_component
  };

  /// Converts an object of class @c url to a string representation.
  /**
   * @param components A bitmask specifying which components of the URL should
   * be included in the string. See the @c url::components_type enumeration for
   * possible values.
   *
   * @returns A string representation of the URL.
   *
   * @par Examples
   * To convert the entire URL to a string:
   * @code
   * std::string s = url.to_string();
   * @endcode
   * To convert only the host and port number into a string:
   * @code
   * std::string s = url.to_string(
   *     urdl::url::host_component
   *     | urdl::url::port_component);
   * @endcode
   */
  inline std::string to_string(int components = all_components) const;

  /// Converts a string representation of a URL into an object of class @c url.
  /**
   * @param s URL string to be parsed into its components.
   *
   * @returns A @c url object corresponding to the specified string.
   *
   * @throws std::system_error Thrown when the URL string is invalid.
   */
  inline static Url from_string(const char* s);

  /// Converts a string representation of a URL into an object of class @c url.
  /**
   * @param s URL string to be parsed into its components.
   *
   * @param ec Error code set to indicate the reason for failure, if any.
   *
   * @returns A @c url object corresponding to the specified string.
   */
  inline static Url from_string(const char* s,
      asio::error_code& ec);

  /// Converts a string representation of a URL into an object of class @c url.
  /**
   * @param s URL string to be parsed into its components.
   *
   * @returns A @c url object corresponding to the specified string.
   *
   * @throws asio::system_error Thrown when the URL string is invalid.
   */
  inline static Url from_string(const std::string& s);

  /// Converts a string representation of a URL into an object of class @c url.
  /**
   * @param s URL string to be parsed into its components.
   *
   * @param ec Error code set to indicate the reason for failure, if any.
   *
   * @returns A @c url object corresponding to the specified string.
   */
  inline static Url from_string(const std::string& s,
      asio::error_code& ec);

  /// Compares two @c url objects for equality.
  friend inline bool operator==(const Url& a, const Url& b);

  /// Compares two @c url objects for inequality.
  friend inline bool operator!=(const Url& a, const Url& b);

  /// Compares two @c url objects for ordering.
  friend inline bool operator<(const Url& a, const Url& b);

private:
  inline static bool unescape_path(const std::string& in, std::string& out);

  std::string protocol_;
  std::string user_info_;
  std::string host_;
  std::string port_;
  std::string path_;
  std::string query_;
  std::string fragment_;
  bool ipv6_host_;
};

using UrlRef = std::shared_ptr<Url>;

} // namespace http
} // namespace cinder

# include "url.ipp"

#endif // URDL_URL_HPP
