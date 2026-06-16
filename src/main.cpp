#include <iostream>
#include <string>
#include "database.h"
#include "parser.h"

int main() {
    Database db;
    std::cout << "====================================\n";
    std::cout << "    Muninn Database Engine V1       \n";
    std::cout << "====================================\n";
    std::cout << "Commands:\n";
    std::cout << "  CREATE <table_name> <col1> <type1> <col2> <type2>...\n";
    std::cout << "  INSERT <table_name> <val1> <val2>...\n";
    std::cout << "  SELECT <table_name>\n";
    std::cout << "  EXIT\n\n";

    std::string line;
    while (true) {
        std::cout << "muninn> ";
        if (!std::getline(std::cin, line)) break;
        if (line == "EXIT" || line == "exit") break;
        if (line.empty()) continue;

        QueryParser::execute(line, db);
    }

    return 0;
}

