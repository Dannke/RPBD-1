#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <odbcinst.h>
#include <iostream>
#include <sqlext.h>
#include <string>
#include <iomanip>
#include <random>

using namespace std;

class TableGateway {
public:
    SQLHDBC hdbc;
    string tableName;
    vector<string> fields;
    const vector<string> fieldTypes;

    TableGateway(SQLHDBC hdbc, const string& tableName) : hdbc(hdbc), tableName(tableName) {}

    vector<string> GetFieldNames() {
        SQLHSTMT hstmt;
        SQLRETURN retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);

        std::vector<std::string> fieldNames;

        std::string query = "SELECT column_name FROM information_schema.columns WHERE table_name = '" + tableName + "' AND column_name != 'id';";
        retcode = SQLExecDirectA(hstmt, (SQLCHAR*)query.c_str(), SQL_NTS);

        if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
            SQLCHAR columnName[255];
            while (SQLFetch(hstmt) == SQL_SUCCESS) {
                SQLGetData(hstmt, 1, SQL_C_CHAR, columnName, sizeof(columnName), NULL);
                fieldNames.push_back(std::string(reinterpret_cast<const char*>(columnName)));
            }
        }

        SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
        return fieldNames;
    }

    vector<string> GetFieldTypes() {
        SQLHSTMT hstmt;
        SQLRETURN retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);

        std::vector<std::string> fieldTypes;

        std::string query = "SELECT data_type FROM information_schema.columns WHERE table_name = '" + tableName + "' AND column_name != 'id';";
        retcode = SQLExecDirectA(hstmt, (SQLCHAR*)query.c_str(), SQL_NTS);

        if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
            SQLCHAR dataType[255];
            while (SQLFetch(hstmt) == SQL_SUCCESS) {
                SQLGetData(hstmt, 1, SQL_C_CHAR, dataType, sizeof(dataType), NULL);
                fieldTypes.push_back(std::string(reinterpret_cast<const char*>(dataType)));
            }
        }

        SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
        return fieldTypes;
    }

    bool RecordExists(int id) {
        SQLHSTMT hstmt;
        SQLRETURN retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);

        // Формируем SQL-запрос для проверки наличия записи
        string query = "SELECT 1 FROM " + tableName + " WHERE id = " + to_string(id);

        retcode = SQLExecDirectA(hstmt, (SQLCHAR*)query.c_str(), SQL_NTS);

        if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
            SQLCHAR result[2]; // Здесь мы ожидаем получить "1"
            if (SQLFetch(hstmt) == SQL_SUCCESS) {
                SQLGetData(hstmt, 1, SQL_C_CHAR, result, sizeof(result), NULL);
                if (strcmp((char*)result, "1") == 0) {
                    // Запись с указанным ID существует
                    return true;
                }
            }
        }
        // Запись с указанным ID не существует
        return false;
    }

    void ShowTable(int startRange = -1, int endRange = -1) {
        SQLHSTMT hstmt;
        SQLRETURN retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);

        string query = "SELECT * FROM \"" + tableName + "\";";
        if (startRange != -1 && endRange != -1) {
            query = "SELECT * FROM \"" + tableName + "\" WHERE book_inventory_number BETWEEN " + to_string(startRange) + " AND " + to_string(endRange) + ";";
        }

        retcode = SQLExecDirectA(hstmt, (SQLCHAR*)query.c_str(), SQL_NTS);

        if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
            cout << "\nTable Contents (" << tableName << "):\n";
            cout << "\n";

            // Вывод заголовков столбцов
            SQLCHAR columnName[255];
            SQLSMALLINT numColumns;
            SQLNumResultCols(hstmt, &numColumns);
            for (int i = 1; i <= numColumns; ++i) {
                SQLDescribeColA(hstmt, i, columnName, sizeof(columnName), NULL, NULL, NULL, NULL, NULL);
                cout << setw(21) << columnName << " | ";
            }
            cout << endl;

            for (int i = 1; i <= numColumns; ++i) {
                cout << "----------------------| ";
            }
            cout << endl;

            while (SQLFetch(hstmt) == SQL_SUCCESS) {
                for (int i = 1; i <= numColumns; ++i) {
                    SQLGetData(hstmt, i, SQL_C_CHAR, columnName, sizeof(columnName), NULL);
                    cout << setw(21) << columnName << " | ";
                }
                cout << endl;
            }

            cout << "\n--------------------------------------------------\n";
        }
        else {
            cout << "\nError fetching table contents." << endl;
        }

        SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
    }

    void InsertRecord(const vector<string>& fields, const vector<string>& fieldTypes) {
        SQLHSTMT hstmt;
        SQLRETURN retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);

        vector<string> values;
        for (size_t i = 0; i < fields.size(); i++) {
            string value;
            cout << "Enter " << fields[i] << ": ";
            cin >> value;
            values.push_back(value);
        }

        string query = "INSERT INTO \"" + tableName + "\" (";

        for (int i = 0; i < fields.size(); ++i) {
            if (i > 0) {
                query += ", ";
            }
            query += "\"" + fields[i] + "\"";
        }

        query += ") VALUES (";

        for (int i = 0; i < fields.size(); ++i) {
            if (i > 0) {
                query += ", ";
            }

            if (fieldTypes[i] == "character varying") {
                query += "'" + values[i] + "'";
            }
            else if (fieldTypes[i] == "integer") {
                query += values[i];
            }
            else if (fieldTypes[i] == "date") {
                query += "'" + values[i] + "'";
            }
        }

        query += ");";

        retcode = SQLExecDirectA(hstmt, (SQLCHAR*)query.c_str(), SQL_NTS);
        if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
            cout << "\nRecord added to " << tableName << " successfully!" << endl;
        }
        else {
            cout << "\nError adding record to " << tableName << ". Native Error: " << retcode << endl;
            SQLCHAR sqlState[6];
            SQLSMALLINT msgLen;
            SQLCHAR errorMsg[SQL_MAX_MESSAGE_LENGTH];
            while (SQLErrorA(SQL_NULL_HENV, hdbc, hstmt, sqlState, NULL, errorMsg, sizeof(errorMsg), &msgLen) == SQL_SUCCESS) {
                cout << "SQLSTATE: " << sqlState << " Error Message: " << errorMsg << endl;
            }
        }

        SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
    }

    void UpdateRecord(const vector<string>& fields, const vector<string>& fieldTypes) {
        SQLHSTMT hstmt;
        SQLRETURN retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);

        int id;
        cout << "Enter the ID of the record to update: ";
        cin >> id;

        // Проверка наличия записи с указанным ID
        if (!RecordExists(id)) {
            cout << "Record with ID " << id << " does not exist in " << tableName << endl;
            return;
        }

        // Запрос новых значений у пользователя
        vector<string> newValues;
        for (size_t i = 0; i < fields.size(); i++) {
            string newValue;
            cout << "Enter new " << fields[i] << ": ";
            cin >> newValue;
            newValues.push_back(newValue);
        }

        string query = "UPDATE " + tableName + " SET ";

        // Формирование части запроса для обновления значений полей
        for (int i = 0; i < fields.size(); ++i) {
            if (i > 0) {
                query += ", ";
            }
            query += "\"" + fields[i] + "\" = ";

            // В зависимости от типа поля добавляем значение в запрос
            if (fieldTypes[i] == "character varying") {
                query += "'" + newValues[i] + "'";
            }
            else if (fieldTypes[i] == "integer") {
                query += newValues[i];
            }
            else if (fieldTypes[i] == "date") {
                query += "'" + newValues[i] + "'";
            }
        }

        // Добавление условия WHERE для указания ID записи, которую необходимо обновить
        query += " WHERE id = " + to_string(id);

        retcode = SQLExecDirectA(hstmt, (SQLCHAR*)query.c_str(), SQL_NTS);
        if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
            cout << "\nRecord with ID " << id << " updated in " << tableName << " successfully!" << endl;
        }
        else {
            cout << "\nError updating record in " << tableName << ". Native Error: " << retcode << endl;
            SQLCHAR sqlState[6];
            SQLSMALLINT msgLen;
            SQLCHAR errorMsg[SQL_MAX_MESSAGE_LENGTH];
            while (SQLErrorA(SQL_NULL_HENV, hdbc, hstmt, sqlState, NULL, errorMsg, sizeof(errorMsg), &msgLen) == SQL_SUCCESS) {
                cout << "SQLSTATE: " << sqlState << " Error Message: " << errorMsg << endl;
            }
        }
        SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
    }

    void DeleteRecord() {
        SQLHSTMT hstmt;
        SQLRETURN retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);

        cout << "Enter the ID of the record you want to delete (0 to cancel): ";
        int id;
        cin >> id;

        if (id == 0) {
            cout << "Deletion canceled." << endl;
            return;
        }

        // Проверка, существует ли запись с указанным ID
        if (!RecordExists(id)) {
            cout << "Record with ID " << id << " does not exist in the table." << endl;
            return;
        }

        string query = "DELETE FROM \"" + tableName + "\" WHERE id = " + to_string(id) + ";";

        retcode = SQLExecDirectA(hstmt, (SQLCHAR*)query.c_str(), SQL_NTS);
        if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
            cout << "Record with ID " << id << " deleted from " << tableName << " successfully!" << endl;
        }
        else {
            cout << "Error deleting record from " << tableName << ". Native Error: " << retcode << endl;
            SQLCHAR sqlState[6];
            SQLSMALLINT msgLen;
            SQLCHAR errorMsg[SQL_MAX_MESSAGE_LENGTH];
            while (SQLErrorA(SQL_NULL_HENV, hdbc, hstmt, sqlState, NULL, errorMsg, sizeof(errorMsg), &msgLen) == SQL_SUCCESS) {
                cout << "SQLSTATE: " << sqlState << " Error Message: " << errorMsg << endl;
            }
        }
        SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
    }
};

class BookCollectionTable: public TableGateway {
public:
    BookCollectionTable(SQLHDBC hdbc) : TableGateway(hdbc, "book_collection") {}

    static bool CreateTable(SQLHDBC hdbc) {
        const char* createBookCollectionSQL = "CREATE TABLE IF NOT EXISTS \"book_collection\" ("
            "   book_title VARCHAR(255) UNIQUE,"
            "   authors VARCHAR(255),"
            "   year_of_creation INT,"
            "   place_of_creation VARCHAR(255),"
            "   publishing_house VARCHAR(255),"
            "   UDC INT REFERENCES systematic_catalog(id) ON DELETE CASCADE,"
            "   id SERIAL PRIMARY KEY"
            ");";
        
        SQLHSTMT hstmt;
        SQLRETURN retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);

        retcode = SQLExecDirectA(hstmt, (SQLCHAR*)createBookCollectionSQL, SQL_NTS);
        SQLFreeHandle(SQL_HANDLE_STMT, hstmt);

        return retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO;
    }
    vector<string> GetFieldNames() { return TableGateway::GetFieldNames(); }
    vector<string> GetFieldTypes() { return TableGateway::GetFieldTypes(); }
    void ShowTable() {TableGateway::ShowTable();}
    void InsertRecord() {TableGateway::InsertRecord(GetFieldNames(), GetFieldTypes());}
    void UpdateRecord() {
        SQLHSTMT hstmt;
        SQLRETURN retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);

        vector<string> fields = GetFieldNames();
        vector<string> fieldTypes = GetFieldTypes();

        string tittleBook;
        cout << "Enter the title of book to update: ";
        cin >> tittleBook;

        // Запрос новых значений у пользователя
        vector<string> newValues;
        for (size_t i = 0; i < fields.size(); i++) {
            string newValue;
            cout << "Enter new " << fields[i] << ": ";
            cin >> newValue;
            newValues.push_back(newValue);
        }

        string query = "UPDATE " + tableName + " SET ";

        // Формирование части запроса для обновления значений полей
        for (int i = 0; i < fields.size(); ++i) {
            if (i > 0) {
                query += ", ";
            }
            query += "\"" + fields[i] + "\" = ";

            // В зависимости от типа поля добавляем значение в запрос
            if (fieldTypes[i] == "character varying") {
                query += "'" + newValues[i] + "'";
            }
            else if (fieldTypes[i] == "integer") {
                query += newValues[i];
            }
            else if (fieldTypes[i] == "date") {
                query += "'" + newValues[i] + "'";
            }
        }

        // Добавление условия WHERE для указания ID записи, которую необходимо обновить
        query += " WHERE book_title = '" + tittleBook + "'";

        retcode = SQLExecDirectA(hstmt, (SQLCHAR*)query.c_str(), SQL_NTS);
        if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
            cout << "\nRecord with ID " << tittleBook << " updated in " << tableName << " successfully!" << endl;
        }
        else {
            cout << "\nError updating record in " << tableName << ". Native Error: " << retcode << endl;
            SQLCHAR sqlState[6];
            SQLSMALLINT msgLen;
            SQLCHAR errorMsg[SQL_MAX_MESSAGE_LENGTH];
            while (SQLErrorA(SQL_NULL_HENV, hdbc, hstmt, sqlState, NULL, errorMsg, sizeof(errorMsg), &msgLen) == SQL_SUCCESS) {
                cout << "SQLSTATE: " << sqlState << " Error Message: " << errorMsg << endl;
            }
        }
        SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
    }
    void DeleteRecord() {
        SQLHSTMT hstmt;
        SQLRETURN retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);

        cout << "Enter the book_title that you want to delete (0 to cancel): ";
        string tittleBook;
        cin >> tittleBook;

        if (tittleBook == "0") {
            cout << "Deletion canceled." << endl;
            return;
        }

        string query = "DELETE FROM \"" + tableName + "\" WHERE book_title = '" + tittleBook + "';";

        retcode = SQLExecDirectA(hstmt, (SQLCHAR*)query.c_str(), SQL_NTS);
        if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
            cout << "Record with title_book " << tittleBook << " deleted from " << tableName << " successfully!" << endl;
        }
        else {
            cout << "Error deleting record from " << tableName << ". Native Error: " << retcode << endl;
            SQLCHAR sqlState[6];
            SQLSMALLINT msgLen;
            SQLCHAR errorMsg[SQL_MAX_MESSAGE_LENGTH];
            while (SQLErrorA(SQL_NULL_HENV, hdbc, hstmt, sqlState, NULL, errorMsg, sizeof(errorMsg), &msgLen) == SQL_SUCCESS) {
                cout << "SQLSTATE: " << sqlState << " Error Message: " << errorMsg << endl;
            }
        }
        SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
    }
    void FindRecordsByUDC() {
        SQLHSTMT hstmt;
        SQLRETURN retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);

        int udc;
        cout << "Enter the UDC value to find records: ";
        cin >> udc;

        string query = "SELECT * FROM " + tableName + " WHERE udc = " + to_string(udc) + ";";
        retcode = SQLExecDirectA(hstmt, (SQLCHAR*)query.c_str(), SQL_NTS);
        cout << "Query: " << query << endl;

        if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
            int colCount;
            SQLNumResultCols(hstmt, (SQLSMALLINT*)&colCount);

            SQLCHAR colName[256];
            for (int i = 1; i <= colCount; i++) {
                SQLDescribeColA(hstmt, i, colName, sizeof(colName), NULL, NULL, NULL, NULL, NULL);
                cout << colName << "\t\t";
            }
            cout << endl;

            while (SQLFetch(hstmt) == SQL_SUCCESS) {
                for (int i = 1; i <= colCount; i++) {
                    SQLLEN indicator;
                    SQLCHAR colValue[256];
                    SQLGetData(hstmt, i, SQL_CHAR, colValue, sizeof(colValue), &indicator);
                    if (indicator == SQL_NULL_DATA) {
                        cout << "NULL" << "\t\t";
                    }
                    else {
                        cout << colValue << "\t\t";
                    }
                }
                cout << endl;
            }
        }
        else {
            cout << "Error while executing the query." << endl;
            SQLCHAR sqlState[6];
            SQLSMALLINT msgLen;
            SQLCHAR errorMsg[SQL_MAX_MESSAGE_LENGTH];
            while (SQLErrorA(SQL_NULL_HENV, hdbc, hstmt, sqlState, NULL, errorMsg, sizeof(errorMsg), &msgLen) == SQL_SUCCESS) {
                cout << "SQLSTATE: " << sqlState << " Error Message: " << errorMsg << endl;
            }
        }
        SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
    }
};

class SystematicCatalogTable : public TableGateway {
public:
    SystematicCatalogTable(SQLHDBC hdbc) : TableGateway(hdbc, "systematic_catalog") {}

    static bool CreateTable(SQLHDBC hdbc) {
        const char* createSystematicCatalogSQL = "CREATE TABLE IF NOT EXISTS \"systematic_catalog\" ("
            "   id SERIAL PRIMARY KEY,"
            "   UDC INT UNIQUE"
            ");";

        SQLHSTMT hstmt;
        SQLRETURN retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);

        retcode = SQLExecDirectA(hstmt, (SQLCHAR*)createSystematicCatalogSQL, SQL_NTS);
        SQLFreeHandle(SQL_HANDLE_STMT, hstmt);

        return retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO;
    }
    vector<string> GetFieldNames() { return TableGateway::GetFieldNames(); }
    vector<string> GetFieldTypes() { return TableGateway::GetFieldTypes(); }
    void ShowTable() { TableGateway::ShowTable(); }
    void UpdateRecord() { TableGateway::UpdateRecord(GetFieldNames(), GetFieldTypes()); }
    void DeleteRecord() { TableGateway::DeleteRecord(); }
    void InsertRecord() {
        int numRecords;
        cout << "Enter the number of records you want to insert: ";
        cin >> numRecords;

        if (numRecords <= 0) {
            cout << "Number of records to insert should be greater than 0." << endl;
            return;
        }

        // Генератор случайных чисел
        random_device rd;
        mt19937 gen(rd());
        uniform_int_distribution<int> dist(100, 999);  // Генерирует значения от 100 до 999 (можете изменить диапазон)

        // SQL-запрос для вставки данных
        const char* insertDataSQL = "INSERT INTO \"systematic_catalog\" (UDC) VALUES (?);";

        SQLHSTMT hstmt;
        SQLRETURN retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);

        for (int i = 0; i < numRecords; i++) {
            int udcValue = dist(gen);  // Генерация случайного значения UDC

            retcode = SQLPrepareA(hstmt, (SQLCHAR*)insertDataSQL, SQL_NTS);
            if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO) {
                cout << "Error preparing SQL statement." << endl;
                return;
            }

            retcode = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &udcValue, 0, NULL);
            if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO) {
                cout << "Error binding parameter." << endl;
                return;
            }

            retcode = SQLExecute(hstmt);
            if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO) {
                cout << "Error executing SQL statement." << endl;
                return;
            }
        }
        cout << numRecords << " random records inserted into systematic_catalog." << endl;
    }
};

class InventoryListTable : public TableGateway {
public:
    InventoryListTable(SQLHDBC hdbc) : TableGateway(hdbc, "inventory_list") {}

    static bool CreateTable(SQLHDBC hdbc) {
        const char* createInventoryListSQL = "CREATE TABLE IF NOT EXISTS \"inventory_list\" ("
            "   id SERIAL PRIMARY KEY,"
            "   id_book INT REFERENCES book_collection(ID) ON DELETE CASCADE,"
            "   book_number INT UNIQUE"
            ");";

        SQLHSTMT hstmt;
        SQLRETURN retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);

        retcode = SQLExecDirectA(hstmt, (SQLCHAR*)createInventoryListSQL, SQL_NTS);
        SQLFreeHandle(SQL_HANDLE_STMT, hstmt);

        return retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO;
    }
    vector<string> GetFieldNames() { return TableGateway::GetFieldNames(); }
    vector<string> GetFieldTypes() { return TableGateway::GetFieldTypes(); }
    void ShowTable() { TableGateway::ShowTable(); }
    void UpdateRecord() { TableGateway::UpdateRecord(GetFieldNames(), GetFieldTypes()); }
    void DeleteRecord() { TableGateway::DeleteRecord(); }
    void InsertRecord() {
        std::mt19937 gen(static_cast<unsigned>(time(0)));
        std::uniform_int_distribution<int> dist(100, 9999);

        SQLHSTMT hstmt;
        SQLRETURN retcode;

        int bookID;
        cout << "Enter the ID of the book to insert inventory numbers: ";
        cin >> bookID;

        // Проверка наличия записи с указанным ID в таблице book_collection
        shared_ptr<TableGateway> bookCollectionGateway = make_shared<TableGateway>(hdbc, "book_collection");
        if (!bookCollectionGateway->RecordExists(bookID)) {
            cout << "Record with ID " << bookID << " does not exist in the book_collection table." << endl;
            return;
        }

        int numRecords;
        cout << "Enter the number of records you want to insert: ";
        cin >> numRecords;

        if (numRecords <= 0) {
            cout << "Number of records to insert should be greater than 0." << endl;
            return;
        }

        retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);

        for (int i = 0; i < numRecords; i++) {
            int bookNumber = dist(gen);

            std::string query = "INSERT INTO inventory_list(id_book, book_number) VALUES (?, ?);";
            retcode = SQLPrepareA(hstmt, (SQLCHAR*)query.c_str(), SQL_NTS);

            if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
                SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &bookID, 0, NULL);
                SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &bookNumber, 0, NULL);

                retcode = SQLExecute(hstmt);

                if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO) {
                    std::cout << "Error inserting record." << std::endl;
                }
            }
            else {
                std::cout << "Error preparing SQL statement." << std::endl;
            }
        }

        SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
    }

};

class ReadersTable : public TableGateway {
public:
    ReadersTable(SQLHDBC hdbc) : TableGateway(hdbc, "readers") {}

    static bool CreateTable(SQLHDBC hdbc) {
        const char* createReadersSQL = "CREATE TABLE IF NOT EXISTS \"readers\" ("
            "   surname VARCHAR(255),"
            "   first_name VARCHAR(255),"
            "   patronymic VARCHAR(255),"
            "   telephone VARCHAR(20),"
            "   address VARCHAR(255),"
            "   registration_date DATE,"
            "   id SERIAL PRIMARY KEY"
            ");";

        SQLHSTMT hstmt;
        SQLRETURN retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);

        retcode = SQLExecDirectA(hstmt, (SQLCHAR*)createReadersSQL, SQL_NTS);
        SQLFreeHandle(SQL_HANDLE_STMT, hstmt);

        return retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO;
    }
    vector<string> GetFieldNames() { return TableGateway::GetFieldNames(); }
    vector<string> GetFieldTypes() { return TableGateway::GetFieldTypes(); }
    void ShowTable() { TableGateway::ShowTable(); }
    void InsertRecord() { TableGateway::InsertRecord(GetFieldNames(), GetFieldTypes()); }
    void UpdateRecord() { TableGateway::UpdateRecord(GetFieldNames(), GetFieldTypes()); }
    void DeleteRecord() { TableGateway::DeleteRecord(); }
};

class AbonementTable : public TableGateway {
public:
    AbonementTable(SQLHDBC hdbc) : TableGateway(hdbc, "abonement") {}

    static bool CreateTable(SQLHDBC hdbc) {
        const char* createAbonementSQL = "CREATE TABLE IF NOT EXISTS \"abonement\" ("
            "   id SERIAL PRIMARY KEY,"
            "   id_reader INT REFERENCES readers(ID) ON DELETE CASCADE,"
            "   book_numbers INT UNIQUE REFERENCES inventory_list(id) ON DELETE CASCADE,"
            "   book_issue_date DATE NOT NULL,"
            "   delivery_date DATE"
            ");";

        SQLHSTMT hstmt;
        SQLRETURN retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);

        retcode = SQLExecDirectA(hstmt, (SQLCHAR*)createAbonementSQL, SQL_NTS);
        SQLFreeHandle(SQL_HANDLE_STMT, hstmt);

        return retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO;
    }
    vector<string> GetFieldNames() { return TableGateway::GetFieldNames(); }
    vector<string> GetFieldTypes() { return TableGateway::GetFieldTypes(); }
    void ShowTable() { TableGateway::ShowTable(); }
    void InsertRecord() { TableGateway::InsertRecord(GetFieldNames(), GetFieldTypes()); }
    void UpdateRecord() {
        SQLHSTMT hstmt;
        SQLRETURN retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);

        vector<string> fields = GetFieldNames();
        vector<string> fieldTypes = GetFieldTypes();

        int id_reader;
        cout << "Enter the ID of the record to update: ";
        cin >> id_reader;

        // Запрос новых значений у пользователя
        vector<string> newValues;
        for (size_t i = 0; i < fields.size(); i++) {
            string newValue;
            cout << "Enter new " << fields[i] << ": ";
            cin >> newValue;
            newValues.push_back(newValue);
        }

        string query = "UPDATE " + tableName + " SET ";

        // Формирование части запроса для обновления значений полей
        for (int i = 0; i < fields.size(); ++i) {
            if (i > 0) {
                query += ", ";
            }
            query += "\"" + fields[i] + "\" = ";

            // В зависимости от типа поля добавляем значение в запрос
            if (fieldTypes[i] == "character varying") {
                query += "'" + newValues[i] + "'";
            }
            else if (fieldTypes[i] == "integer") {
                query += newValues[i];
            }
            else if (fieldTypes[i] == "date") {
                query += "'" + newValues[i] + "'";
            }
        }

        // Добавление условия WHERE для указания ID записи, которую необходимо обновить
        query += " WHERE id = " + to_string(id_reader);

        retcode = SQLExecDirectA(hstmt, (SQLCHAR*)query.c_str(), SQL_NTS);
        if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
            cout << "\nRecord with ID " << id_reader << " updated in " << tableName << " successfully!" << endl;
        }
        else {
            cout << "\nError updating record in " << tableName << ". Native Error: " << retcode << endl;
            SQLCHAR sqlState[6];
            SQLSMALLINT msgLen;
            SQLCHAR errorMsg[SQL_MAX_MESSAGE_LENGTH];
            while (SQLErrorA(SQL_NULL_HENV, hdbc, hstmt, sqlState, NULL, errorMsg, sizeof(errorMsg), &msgLen) == SQL_SUCCESS) {
                cout << "SQLSTATE: " << sqlState << " Error Message: " << errorMsg << endl;
            }
        }
        SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
    }
    void DeleteRecord() { TableGateway::DeleteRecord(); }
};

void ShowTableMenu(BookCollectionTable& bookTable, SystematicCatalogTable& catalogtable, InventoryListTable& inventoryTable, ReadersTable& readersTable, AbonementTable& abonementTable) {
    int choice;
    while (true) {
        cout << "\nSelect a table to display:" << endl;
        cout << "1. Book Collection" << endl;
        cout << "2. Readers" << endl;
        cout << "3. Abonement" << endl;
        cout << "4. Systematic Catalog" << endl;
        cout << "5. Inventory List" << endl;
        cout << "0. Exit" << endl;
        cout << "Enter your choice: ";
        cin >> choice;

        switch (choice) {
        case 1:
            bookTable.ShowTable();
            break;
        case 2:
            readersTable.ShowTable();
            break;
        case 3:
            abonementTable.ShowTable();
            break;
        case 4:
            catalogtable.ShowTable();
            break;
        case 5:
            inventoryTable.ShowTable();
            break;
        case 0:
            return;  // Возврат к предыдущему меню
        default:
            cout << "Invalid choice. Please try again." << endl;
            break;
        }
    }
}

void InsertDataMenu(BookCollectionTable& bookTable, SystematicCatalogTable& catalogtable, InventoryListTable& inventoryTable, ReadersTable& readersTable, AbonementTable& abonementTable) {
    int choice;
    while (true) {

        cout << "\nSelect a table to insert data into:" << endl;
        cout << "1. Book Collection" << endl;
        cout << "2. Abonement" << endl;
        cout << "3. Readers" << endl;
        cout << "4. Inventory List" << endl;
        cout << "5. Systematic Catalog" << endl;
        cout << "0. Back to Main Menu" << endl;
        cout << "Enter your choice: ";
        cin >> choice;

        switch (choice) {
        case 1:
            bookTable.InsertRecord();
            break;
        case 2:
            abonementTable.InsertRecord();
            break;
        case 3:
            readersTable.InsertRecord();
            break;
        case 4:
            inventoryTable.InsertRecord();
            break;
        case 5:
            catalogtable.InsertRecord();
            break;
        case 0:
            return;  // Возврат к предыдущему меню
        default:
            cout << "Invalid choice. Please try again." << endl;
            break;
        }
    }
}

void UpdateTableMenu(BookCollectionTable& bookTable, SystematicCatalogTable& catalogtable, InventoryListTable& inventoryTable, ReadersTable& readersTable, AbonementTable& abonementTable) {
    int choice;
    while (true) {
        cout << "\nSelect a table to update:" << endl;
        cout << "1. Book Collection" << endl;
        cout << "2. Readers" << endl;
        cout << "3. Abonement" << endl;
        cout << "4. Systematic Catalog" << endl;
        cout << "5. Inventory List" << endl;
        cout << "0. Exit" << endl;
        cout << "Enter your choice: ";
        cin >> choice;

        switch (choice) {
        case 1:
            bookTable.UpdateRecord();
            break;
        case 2:
            readersTable.UpdateRecord();
            break;
        case 3:
            abonementTable.UpdateRecord();
            break;
        case 4:
            catalogtable.UpdateRecord();
            break;
        case 5:
            inventoryTable.UpdateRecord();
            break;
        case 0:
            return;  // Возврат к предыдущему меню
        default:
            cout << "Invalid choice. Please try again." << endl;
            break;
        }
    }
}

void DeleteRecordMenu(BookCollectionTable& bookTable, SystematicCatalogTable& catalogtable, InventoryListTable& inventoryTable, ReadersTable& readersTable, AbonementTable& abonementTable) {
    int choice;
    while (true) {
        cout << "\nSelect a table to delete a record:" << endl;
        cout << "1. Book Collection" << endl;
        cout << "2. Readers" << endl;
        cout << "3. Abonement" << endl;
        cout << "4. Systematic Catalog" << endl;
        cout << "5. Inventory List" << endl;
        cout << "0. Exit" << endl;
        cout << "Enter your choice: ";
        cin >> choice;

        switch (choice) {
        case 1:
            bookTable.DeleteRecord();
            break;
        case 2:
            readersTable.DeleteRecord();
            break;
        case 3:
            abonementTable.DeleteRecord();
            break;
        case 4:
            catalogtable.DeleteRecord();
            break;
        case 5:
            inventoryTable.DeleteRecord();
            break;
        case 0:
            return;  // Возврат к предыдущему меню
        default:
            cout << "Invalid choice. Please try again." << endl;
            break;
        }
    }
}

void FindMenu(BookCollectionTable& bookTable, SystematicCatalogTable& catalogtable, InventoryListTable& inventoryTable, ReadersTable& readersTable, AbonementTable& abonementTable) {
    int choice;
    while (true) {
        cout << "\nSelect a table to find a data:" << endl;
        cout << "1. Book Collection" << endl;
        cout << "2. Readers" << endl;
        cout << "3. Abonement" << endl;
        cout << "4. Systematic Catalog" << endl;
        cout << "5. Inventory List" << endl;
        cout << "0. Exit" << endl;
        cout << "Enter your choice: ";
        cin >> choice;

        switch (choice) {
        case 1:
            bookTable.FindRecordsByUDC();
            break;
        case 2:
            readersTable.DeleteRecord();
            break;
        case 3:
            abonementTable.DeleteRecord();
            break;
        case 4:
            catalogtable.DeleteRecord();
            break;
        case 5:
            inventoryTable.DeleteRecord();
            break;
        case 0:
            return;  // Возврат к предыдущему меню
        default:
            cout << "Invalid choice. Please try again." << endl;
            break;
        }
    }
}

int main() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode;
    GetConsoleMode(hConsole, &dwMode);
    SetConsoleMode(hConsole, dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);

    SQLHENV henv;
    SQLHDBC hdbc;
    SQLRETURN retcode;

    retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);
    retcode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);
    retcode = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);

    retcode = SQLConnectA(hdbc, (SQLCHAR*)"LibaA", SQL_NTS, (SQLCHAR*)"admin", SQL_NTS, (SQLCHAR*)"123", SQL_NTS);

    if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
        std::system("chcp 1251");
        cout << "Connected to the database!\n" << endl;

        BookCollectionTable bookTable(hdbc);
        SystematicCatalogTable catalogtable(hdbc);
        InventoryListTable inventoryTable(hdbc);
        ReadersTable readersTable(hdbc);
        AbonementTable abonementTable(hdbc);

        bookTable.CreateTable(hdbc);
        catalogtable.CreateTable(hdbc);
        inventoryTable.CreateTable(hdbc);
        readersTable.CreateTable(hdbc);
        abonementTable.CreateTable(hdbc);

        while (true) {
            cout << "\nLibrary Management System" << endl;
            cout << "1. Show Contents of Tables" << endl;
            cout << "2  Insert Data into Table" << endl;
            cout << "3. Update Data in Table" << endl;
            cout << "4. Delete Record" << endl;
            cout << "5. Find Data" << endl;
            cout << "0. Exit" << endl;
            cout << "Enter your choice: ";

            int choice;
            cin >> choice;
            cin.ignore();

            switch (choice)
            {
            case 1:
                ShowTableMenu(bookTable, catalogtable, inventoryTable, readersTable, abonementTable);
                break;
            case 2:
                InsertDataMenu(bookTable, catalogtable, inventoryTable, readersTable, abonementTable);
                break;
            case 3:
                UpdateTableMenu(bookTable, catalogtable, inventoryTable, readersTable, abonementTable);
                break;
            case 4:
                DeleteRecordMenu(bookTable, catalogtable, inventoryTable, readersTable, abonementTable);
                break;
            case 5:
                FindMenu(bookTable, catalogtable, inventoryTable, readersTable, abonementTable);
                break;
            case 0:
                SQLDisconnect(hdbc);
                SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
                SQLFreeHandle(SQL_HANDLE_ENV, henv);
                return 0;
            default:
                cout << "\nInvalid choice. Please try again.\n" << endl;
                break;
            }
        }
    }
    else {
        cout << "Connection failed!" << endl;
    }
    return 0;
}