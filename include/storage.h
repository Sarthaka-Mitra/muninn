#pragma once

#include "table.h"
#include <string>

class StorageEngine {
public:
    // Saves all rows in the table to a file inside the data/ folder.
    static bool saveTable(const Table& table);

    // Loads rows from the file into the provided table object.
    static bool loadTable(Table& table);

private:
    // Helper to generate the file path (e.g., "data/students.db")
    static std::string getFilePath(const std::string& tableName);
};

