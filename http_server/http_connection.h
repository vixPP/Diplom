#pragma once

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio.hpp>
#include <pqxx/pqxx> 

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

class HttpConnection : public std::enable_shared_from_this<HttpConnection>
{
protected:
    tcp::socket socket_;
    beast::flat_buffer buffer_{ 8192 };
    http::request<http::dynamic_body> request_;
    http::response<http::dynamic_body> response_;
    net::steady_timer deadline_{ socket_.get_executor(), std::chrono::seconds(60) };

    void readRequest();
    void processRequest();
    void createResponseGet();
    void createResponsePost();
    void writeResponse();
    void checkDeadline();
    std::vector<std::pair<int, std::string>> searchDatabase(const std::vector<std::string>& keywords);
    std::string buffers_to_string(const beast::multi_buffer::const_buffers_type& buffers);
    
public:
    HttpConnection(tcp::socket socket);
    void start();
};


