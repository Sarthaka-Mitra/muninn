#include <iostream>
#include <vector>
#include "database.h"
#include "storage.h"
#include "types.h"

int main() {
    Database db;

    // 1. Create the schema and tell the Database to create the table
    std::vector<Column> schema = {
        {"name", ColumnType::TEXT},
        {"age", ColumnType::INT}
    };
    db.createTable("students", schema);

    // 2. Get the table from the Database
    Table* studentsTable = db.getTable("students");
    
    if (studentsTable != nullptr) {
        std::cout << "Successfully retrieved table: " << studentsTable->getName() << "\n";

        // 3. Insert a row
        Row validRow = {"Arjun", "21"};
        studentsTable->insertRow(validRow);

        // 4. Ask the StorageEngine to save the table we got from the Database
        StorageEngine::saveTable(*studentsTable);
        std::cout << "Saved to disk via Database pointer.\n";
    }

    return 0;
}

