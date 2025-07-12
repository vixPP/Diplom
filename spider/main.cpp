#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <pqxx/pqxx>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>

#include "indexor.h"
#include "http_utils.h"
#include <functional>
#include "DB.h"

// Глобальные переменные для конфигурации
std::string db_host;
int db_port;
std::string db_name;
std::string db_user;
std::string db_password;
std::string start_page;
int recursion_depth;
int search_port;

std::mutex mtx;
std::condition_variable cv;
std::queue<std::function<void()>> tasks;
bool exitThreadPool = false;
std::string html_content;

void loadConfig(const std::string& filename)
{
	try
	{
		boost::property_tree::ptree pt;
		boost::property_tree::ini_parser::read_ini(filename, pt);

		db_host = pt.get<std::string>("database.host");
		db_port = pt.get<int>("database.port");
		db_name = pt.get<std::string>("database.dbname");
		db_user = pt.get<std::string>("database.user");
		db_password = pt.get<std::string>("database.password");
		start_page = pt.get<std::string>("spider.start_page");
		recursion_depth = pt.get<int>("spider.recursion_depth");
		search_port = pt.get<int>("spider.search_port");
	}
	catch (const boost::property_tree::ini_parser::ini_parser_error& e) 
	{
		std::cerr << "Error parsing INI file: " << e.what() << std::endl;
		throw; 
	}
	catch (const std::exception& e) 
	{
		std::cerr << "General error: " << e.what() << std::endl;
		throw; 
	}
}

void threadPoolWorker()
{
	std::unique_lock<std::mutex> lock(mtx);

	while (!exitThreadPool || !tasks.empty())
	{
		if (tasks.empty())
		{
			cv.wait(lock);
		}
		else
		{
			auto task = tasks.front();
			tasks.pop();
			lock.unlock();
			task();
			lock.lock();
		}
	}
}

void parseLink(const Link& link, int depth)
{
	try
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
		std::string html = getHtmlContent(link);

		if (html.size() == 0)
		{
			std::cout << "Failed to get HTML Content" << std::endl;
			return;
		}

		html_content = html;

		std::vector<Link> links =
		{
			{start_page}
		
		};

		if (depth > 0)
		{
			std::lock_guard<std::mutex> lock(mtx);
			size_t count = links.size();
			size_t index = 0;

			for (auto& subLink : links)
			{
				tasks.push([subLink, depth]() { parseLink(subLink, depth - 1); });
			}
			cv.notify_one();
		}
	}
	catch (const std::exception& e)
	{
		std::cout << e.what() << std::endl;
	}
}



int main()
{

	loadConfig("D:/HOMEWORKS/DiplomicProject/Diplom/spider/config.ini");

	std::unordered_map<std::string, int> wordFrequency;
	std::string cleanedText; // Строка для хранения очищенного текста
	


	try
	{
		int numThreads = std::thread::hardware_concurrency();
		std::vector<std::thread> threadPool;

		for (int i = 0; i < numThreads; ++i)
		{
			threadPool.emplace_back(threadPoolWorker);
		}


		Link link{start_page};
		{
			std::lock_guard<std::mutex> lock(mtx);
			tasks.push([link]() { parseLink(link, 1); });
			cv.notify_one();
		}

		

		std::this_thread::sleep_for(std::chrono::seconds(2));
		{
			std::lock_guard<std::mutex> lock(mtx);
			exitThreadPool = true;
			cv.notify_all();
		}


		for (auto& t : threadPool)
		{
			t.join();
		}
	}

	catch (const std::exception& e)
	{
		std::cout << e.what() << std::endl;
	}

	cleanText(html_content);
	indexWords(html_content, wordFrequency, cleanedText); // Передаем cleanedText
	//std::cout << "Очищенный текст без коротких и длинных слов: " << std::endl << cleanedText << std::endl; // Выводим очищенный текст
	//std::cout << "Частота слов:" << std::endl;

	for (const auto& pair : wordFrequency)
	{
		//	std::cout << pair.first << ": " << pair.second << std::endl;
		wordFrequency;
	}

	try
	{
		std::string connection_string =
			"host=" + db_host + " "
			"port=" + std::to_string(db_port) + " "
			"dbname=" + db_name + " "
			"user=" + db_user + " "
			"password=" + db_password;

		DataBaseSearcher DBS(connection_string);

		Link link{start_page};
		std::string fullUrl = link.GetFullUrl();

		DBS.CreateTables();
		std::cout << "Адрес: " << fullUrl << std::endl << std::endl;
		std::cout << "Результат: ";

		DBS.AddWordsDB(fullUrl, wordFrequency);
		
	}
	catch (pqxx::sql_error& e) 
	{
		std::cout << e.what() << std::endl;
	}

	return 0;

}