#pragma once

#include <iostream>
#include <pqxx/pqxx>
#include <string>
#include "indexor.h"

class DataBaseSearcher
{
private:
    pqxx::connection* connection_ = NULL; // Соединение с базой данных
   // pqxx::work transaction_; // Транзакция для работы с базой данных
    std::string last_error;
public:

    int InsertDocument(pqxx::work& transaction, const std::string& title);
    int InsertWord(pqxx::work& transaction, const std::string& word);

    DataBaseSearcher(const std::string& connection);
    void CreateTables();
    void AddWordsDB(const std::string& docTitle, const std::unordered_map<std::string, int>& wordFrequency);


};
