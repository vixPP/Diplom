#include <string>
#include <unordered_set>
#include <regex>

enum class ProtocolType
{
    HTTP = 0,
    HTTPS = 1
};

struct Link
{
    ProtocolType protocol;
    std::string hostName;
    std::string query;

    // Конструктор для инициализации из полной URL
    Link(const std::string& fullUrl)
    {
        std::regex urlRegex(R"((https?)://([^/]+)(/.*)?)");
        std::smatch urlMatch;

        if (std::regex_match(fullUrl, urlMatch, urlRegex))
        {
            protocol = (urlMatch[1] == "https") ? ProtocolType::HTTPS : ProtocolType::HTTP;
            hostName = urlMatch[2];
            query = urlMatch[3].str(); // Запрос может быть пустым, если его нет
        }
        else
        {
            throw std::invalid_argument("Invalid URL format");
        }
    }

    std::string GetFullUrl() const
    {
        return (protocol == ProtocolType::HTTPS ? "https://" : "http://") + hostName + query;
    }
};

