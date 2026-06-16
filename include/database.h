#pragma once
#include <string>
#include <unordered_map>
#include "table.h"

class Database {
public:
    // Create a new table and store it in our hash map
    void createTable(const std::string& name, const std::vector<Column>& schema);

    // Get a pointer to a table by name. Returns nullptr if it doesn't exist.
    Table* getTable(const std::string& name);

private:
    std::unordered_map<std::string, Table> tables_;
};

