#include <iostream>
#include <string>
#include <regex>
#include <unordered_map>
#include <boost/locale.hpp>
#include <vector>

void cleanText(std::string& html_content);
void indexWords(const std::string& html_content, std::unordered_map<std::string, int>& wordFrequency, std::string& cleanedText);

