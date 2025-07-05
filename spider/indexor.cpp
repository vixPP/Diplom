#include "indexor.h"




void cleanText(std::string& html_content)
{
    // Удаляем HTML-теги
    html_content = std::regex_replace(html_content, std::regex("<[^>]*>"), " ");

    // Удаляем знаки препинания, табуляцию и переносы строк
    html_content = std::regex_replace(html_content, std::regex("[^\\w\\s]"), " ");

    // Приводим текст к нижнему регистру
    boost::locale::generator gen;
    std::locale loc = gen("en_US.UTF-8");
    html_content = boost::locale::to_lower(html_content, loc);

    // Удаляем лишние пробелы
    html_content = std::regex_replace(html_content, std::regex("\\s+"), " ");
}

void indexWords(const std::string& html_content, std::unordered_map<std::string, int>& wordFrequency, std::string& cleanedText)
{
	std::istringstream iss(html_content);
	std::string word;

	while (iss >> word)
	{
		// Отбрасываем слова короче 3 и длиннее 32 символов
		if (word.length() >= 4 && word.length() <= 32)
		{
			// Добавляем слово в частотный словарь
			wordFrequency[word]++;
			// Добавляем слово в очищенный текст
			cleanedText += word + " "; // Добавляем пробел после слова
		}
	}
}