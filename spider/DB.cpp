#include "DB.h"

DataBaseSearcher::DataBaseSearcher(const std::string& connection) : connection_(new pqxx::connection(connection))
{
    std::cout << "Подключение к базе данных установлено!" << std::endl << std::endl;

    pqxx::work t(*connection_);

}

void DataBaseSearcher::CreateTables() // создание таблиц
{

    pqxx::work t(*connection_);

    if ((connection_ == nullptr) || (!(connection_->is_open())))
    {
        last_error = "Create table error. No database connection.";
    }

    try 
    {
      
        if (t.exec("SELECT to_regclass('public.Documents');")[0][0].is_null()) {
            t.exec("CREATE TABLE Documents ("
                "id SERIAL PRIMARY KEY, "
                "title TEXT NOT NULL);");
        }

        
        if (t.exec("SELECT to_regclass('public.Words');")[0][0].is_null()) {
            t.exec("CREATE TABLE Words ("
                "id SERIAL PRIMARY KEY, "
                "word TEXT NOT NULL UNIQUE);"); 
        }

        
        if (t.exec("SELECT to_regclass('public.DocumentWords');")[0][0].is_null()) {
            t.exec("CREATE TABLE DocumentWords ("
                "document_id INTEGER, "
                "word_id INTEGER, "
                "frequency INTEGER, "
                "PRIMARY KEY (document_id, word_id), "
                "FOREIGN KEY (document_id) REFERENCES Documents(id), "
                "FOREIGN KEY (word_id) REFERENCES Words(id));");
        }

        t.commit(); // Зафиксировать изменения
        std::cout << "Таблицы готовы!" << std::endl <<std::endl;
    }
    catch (const std::exception& e)
    {
        last_error = "Ошибка при создании таблиц: " + std::string(e.what());
        std::cerr << last_error << std::endl; 
    }
}

int DataBaseSearcher::InsertDocument(pqxx::work& transaction, const std::string& title) 
{
    try
    {
       
        pqxx::result checkRes = transaction.exec("SELECT COUNT(*) FROM Documents WHERE title = " + transaction.quote(title));
        if (checkRes[0][0].as<int>() > 0)
        {
            std::cout <<"Данный адрес уже существует!!!" << std::endl;
            return -1; 
        }

     
        transaction.exec0("INSERT INTO Documents (title) VALUES (" + transaction.quote(title) + ")");
        
        pqxx::result res = transaction.exec("SELECT currval(pg_get_serial_sequence('Documents', 'id'))");
        std::cout << "Добавление документа прошло успешно!" << std::endl;
        return res[0][0].as<int>();
    }
    catch (const std::exception& e)
    {
        std::cerr << "Ошибка при вставке документа: " << e.what() << std::endl;
        return -1;
    }
}

int DataBaseSearcher::InsertWord(pqxx::work& transaction, const std::string& word)
{
    try
    {
        // Проверяем, существует ли слово
        pqxx::result checkRes = transaction.exec("SELECT id FROM Words WHERE word = " + transaction.quote(word));
        if (!checkRes.empty())
        {
            return checkRes[0][0].as<int>();
        }

        
        transaction.exec0("INSERT INTO Words (word) VALUES (" + transaction.quote(word) + ")");

        pqxx::result res = transaction.exec("SELECT currval(pg_get_serial_sequence('Words', 'id'))");
        return res[0][0].as<int>();
    }
    catch (const std::exception& e)
    {
        std::cerr << "Ошибка при вставке слова: " << e.what() << std::endl;
        return -1; // Возвращаем -1 в случае ошибки
    }
}



void DataBaseSearcher::AddWordsDB(const std::string& docTitle, const std::unordered_map<std::string, int>& wordFrequency) 
{
    pqxx::work transaction(*connection_); 
    int documentId = InsertDocument(transaction, docTitle);
    if (documentId == -1) 
    {
        return; 
    }

    for (const auto& pair : wordFrequency) 
    {
        const std::string& word = pair.first;
        int frequency = pair.second;

        int wordId = InsertWord(transaction, word);
        if (wordId != -1) 
        {
           
            try 
            {
                transaction.exec0("INSERT INTO DocumentWords (document_id, word_id, frequency) VALUES (" +
                    std::to_string(documentId) + ", " + std::to_string(wordId) + ", " + std::to_string(frequency) + ")");
            }
            catch (const std::exception& e) 
            {
                std::cerr << "Ошибка при вставке в DocumentWords: " << e.what() << std::endl;
            }
        }
    }

    transaction.commit(); 
}







