#include "parser.h"
#include <sstream>
#include <iostream>
#include <vector>

void QueryParser::execute(const std::string& query, Database& db) {
    std::stringstream ss(query);
    std::string command;
    ss >> command;

    if (command == "CREATE") {
        std::string firstArg;
        ss >> firstArg;
        
        if (firstArg == "INDEX") {
            std::string tableName, columnName;
            if (ss >> tableName >> columnName) {
                Table* table = db.getTable(tableName);
                if (!table) {
                    std::cout << "Error: Table '" << tableName << "' not found.\n";
                    return;
                }
                if (table->createIndex(columnName)) {
                    std::cout << "Index created on " << tableName << "(" << columnName << ").\n";
                } else {
                    std::cout << "Error: Column '" << columnName << "' not found in table '" << tableName << "'.\n";
                }
            } else {
                std::cout << "Error: Invalid CREATE INDEX syntax. Expected: CREATE INDEX <tableName> <columnName>\n";
            }
        } else {
            // Standard Table Creation: firstArg is the tableName
            std::string tableName = firstArg;
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
        
        std::vector<Row> results;
        std::string optionalWhere;
        
        if (ss >> optionalWhere) {
            if (optionalWhere == "WHERE") {
                std::string columnName, value;
                if (ss >> columnName >> value) {
                    results = table->selectWhere(columnName, value);
                } else {
                    std::cout << "Error: Invalid WHERE clause syntax.\n";
                    return;
                }
            } else {
                std::cout << "Error: Invalid SELECT syntax. Expected: SELECT <tableName> [WHERE <columnName> <value>]\n";
                return;
            }
        } else {
            // No WHERE clause, return all rows
            results = table->getRows();
        }
        
        for (const Row& row : results) {
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

