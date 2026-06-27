#include "database.h"
#include "table.h"
#include <iostream>
#include "storage.h"
#include <utility>

void Database::createTable(const std::string& name, const std::vector<Column>& schema) {
    // TODO: Create a new Table object using the name and schema.
    // Then, insert it into the tables_ hash map.
    // Hint: To insert into a C++ unordered_map, you can use the bracket syntax just like a Python dictionary!
    // Example: myMap[key] = value;
    Table newTable(name, schema);
    StorageEngine::loadTable(newTable);
    tables_.emplace(name, std::move(newTable));
}

Table* Database::getTable(const std::string& name) {
    // We use .find() to look for the key
    auto it = tables_.find(name);
    
    // If it equals .end(), it means the key wasn't found
    if (it == tables_.end()) {
        return nullptr; 
    }
    
    // If found, return a pointer to the Table object (the 'value' in our key-value pair)
    return &(it->second);
}

