#include "parser.h"
#include <sstream>
#include <iostream>
#include <vector>

void QueryParser::execute(const std::string& query, Database& db) {
    std::stringstream ss(query);
    std::string command;
    ss >> command;

    if (command == "CREATE") {
        std::string tableName;
        ss >> tableName;
        
        std::vector<Column> schema;
        std::string colName, colTypeStr;
        
        // Parse pairs of column name and type (e.g., name TEXT age INT)
        while (ss >> colName >> colTypeStr) {
            ColumnType type = (colTypeStr == "INT") ? ColumnType::INT : ColumnType::TEXT;
            schema.push_back({colName, type});
        }
        
        db.createTable(tableName, schema);
        std::cout << "Table '" << tableName << "' created.\n";
    } 
    else if (command == "INSERT") {
        std::string tableName;
        ss >> tableName;
        
        Table* table = db.getTable(tableName);
        if (!table) {
            std::cout << "Error: Table not found.\n";
            return;
        }
        
        Row row;
        std::string val;
        while (ss >> val) {
            row.push_back(val);
        }
        
        if (table->insertRow(row)) {
            std::cout << "1 row inserted.\n";
        } else {
            std::cout << "Error: Schema mismatch or invalid data.\n";
        }
    }
    else if (command == "SELECT") {
        std::string tableName;
        ss >> tableName;
        
        Table* table = db.getTable(tableName);
        if (!table) {
            std::cout << "Error: Table not found.\n";
            return;
        }
        
        for (const Row& row : table->getRows()) {
            for (const std::string& val : row) {
                std::cout << val << "\t";
            }
            std::cout << "\n";
        }
    }
    else {
        std::cout << "Unknown command: " << command << "\n";
    }
}
