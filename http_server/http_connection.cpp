#include "http_connection.h"
#include <sstream>
#include <iomanip>
#include <locale>
#include <codecvt>
#include <iostream>

#include "parser_config.h"


namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

std::string HttpConnection::buffers_to_string(const beast::multi_buffer::const_buffers_type& buffers)
{
    std::string result;
    for (const auto& buffer : buffers)
    {
        result.append(boost::beast::buffers_to_string(buffer));
    }
    return result;
}

std::vector<std::pair<int, std::string>> HttpConnection::searchDatabase(const std::vector<std::string>& keywords)
{
    std::vector<std::pair<int, std::string>> results;

    try
    {
        Config config = loadConfig("D:/HOMEWORKS/DiplomicProject/Diplom/config.ini");

        std::string connection_string =
            "dbname=" + config.db_name + " "
            "user=" + config.db_user + " "
            "password=" + config.db_password;

        pqxx::connection C(connection_string);
        pqxx::work W(C);

        std::string query = "SELECT d.id, d.title, SUM(dw.frequency) AS relevance "
            "FROM documentwords dw "
            "JOIN words w ON dw.word_id = w.id "
            "JOIN documents d ON dw.document_id = d.id "
            "WHERE w.word IN (";

        for (size_t i = 0; i < keywords.size(); ++i)
        {
            query += "'" + W.esc(keywords[i]) + "'";
            if (i < keywords.size() - 1)
            {
                query += ", ";
            }
        }

        query += ") "
            "GROUP BY d.id, d.title "
            "HAVING COUNT(DISTINCT w.id) >= 1 " // Изменено на >= 1, чтобы находить документы с любым из ключевых слов
            "ORDER BY relevance DESC "
            "LIMIT 10;";

        std::cout << "Executing query: " << query << std::endl; // Отладочная печать

        // Выполняем запрос
        pqxx::result R = W.exec(query);
        W.commit();

        // Обрабатываем результаты
        for (const auto& row : R)
        {
            results.emplace_back(row[0].as<int>(), row[1].as<std::string>());
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << std::endl << "Проверьте данные подключения к базе!!! " << std::endl << e.what() << std::endl;
    }

    return results;
}


HttpConnection::HttpConnection(tcp::socket socket)
    : socket_(std::move(socket))
{
}

void HttpConnection::start()
{
    readRequest();
    checkDeadline();
}

void HttpConnection::readRequest()
{
    auto self = shared_from_this();

    http::async_read
    (
        socket_,
        buffer_,
        request_,
        [self](beast::error_code ec, std::size_t bytes_transferred)
        {
            boost::ignore_unused(bytes_transferred);
            if (!ec)
                self->processRequest();
        });
}

void HttpConnection::processRequest()
{
    response_.version(request_.version());
    response_.keep_alive(false);

    switch (request_.method())
    {
    case http::verb::get:
        response_.result(http::status::ok);
        response_.set(http::field::server, "Beast");
        createResponseGet();
        break;
    case http::verb::post:
        response_.result(http::status::ok);
        response_.set(http::field::server, "Beast");
        createResponsePost();
        break;

    default:
        response_.result(http::status::bad_request);
        response_.set(http::field::content_type, "text/plain");
        beast::ostream(response_.body())
            << "Invalid request-method '"
            << std::string(request_.method_string())
            << "'";
        break;
    }

    writeResponse();
}

void HttpConnection::createResponseGet()
{
    if (request_.target() == "/")
    {
        response_.set(http::field::content_type, "text/html");
        beast::ostream(response_.body())
            << "<html>\n"
            << "<head><meta charset=\"UTF-8\"><title>Search Engine</title></head>\n"
            << "<body>\n"
            << "<h1>Search Engine</h1>\n"
            << "<p>Welcome!<p>\n"
            << "<form action=\"/\" method=\"post\">\n"
            << "    <label for=\"search\">Search:</label><br>\n"
            << "    <input type=\"text\" id=\"search\" name=\"search\"><br>\n"
            << "    <input type=\"submit\" value=\"Search\">\n"
            << "</form>\n"
            << "</body>\n"
            << "</html>\n";
    }
    else
    {
        response_.result(http::status::not_found);
        response_.set(http::field::content_type, "text/plain");
        beast::ostream(response_.body()) << "File not found\r\n";
    }
}

void HttpConnection::createResponsePost()
{
    if (request_.target() == "/")
    {
        std::string s = buffers_to_string(request_.body().data());

        size_t pos = s.find('=');
        if (pos == std::string::npos)
        {
            response_.result(http::status::bad_request);
            response_.set(http::field::content_type, "text/plain");
            beast::ostream(response_.body()) << "Invalid search query\r\n";
            return;
        }

        std::string key = s.substr(0, pos);
        std::string value = s.substr(pos + 1);

        if (key != "search")
        {
            response_.result(http::status::bad_request);
            response_.set(http::field::content_type, "text/plain");
            beast::ostream(response_.body()) << "Invalid search key\r\n";
            return;
        }

        std::vector<std::string> keywords;
        std::istringstream iss(value);
        std::string word;

        // Извлечение слов из строки, игнорируя лишние пробелы
        while (iss >> word)
        {
            if (keywords.size() < 4) // Ограничение на количество ключевых слов
            {
                keywords.push_back(word);
            }
        }

        // Теперь передаем ключевые слова в базу данных
        auto searchResult = searchDatabase(keywords);

        response_.set(http::field::content_type, "text/html");
        beast::ostream(response_.body())
            << "<html>\n"
            << "<head><meta charset=\"UTF-8\"><title>Search Results</title></head>\n"
            << "<body>\n"
            << "<h1>Search Results</h1>\n"
            << "<ul>\n";

        if (searchResult.empty())
        {
            beast::ostream(response_.body()) << "<li>No results found.</li>";
        }
        else
        {
            for (const auto& result : searchResult)
            {
                beast::ostream(response_.body())
                    << "<li><a href=\"" << "\">" << result.second << "</a></li>";
            }
        }

        beast::ostream(response_.body())
            << "</ul>\n"
            << "</body>\n"
            << "</html>\n";
    }
    else
    {
        response_.result(http::status::not_found);
        response_.set(http::field::content_type, "text/plain");
        beast::ostream(response_.body()) << "File not found\r\n";
    }
}


void HttpConnection::writeResponse()
{
    auto self = shared_from_this();

    response_.content_length(response_.body().size());

    http::async_write(
        socket_,
        response_,
        [self](beast::error_code ec, std::size_t)
        {
            self->socket_.shutdown(tcp::socket::shutdown_send, ec);
            self->deadline_.cancel();
        });
}

void HttpConnection::checkDeadline()
{
    auto self = shared_from_this();

    deadline_.async_wait(
        [self](beast::error_code ec)
        {
            if (!ec)
            {
                self->socket_.close(ec);
            }
        });
}