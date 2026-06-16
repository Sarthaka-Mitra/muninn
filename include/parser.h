#pragma once
#include <string>
#include "database.h"

class QueryParser {
public:
    // Takes a raw string from the REPL and executes it against the database
    static void execute(const std::string& query, Database& db);
};
