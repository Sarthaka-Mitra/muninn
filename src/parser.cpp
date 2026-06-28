#include "parser.h"
#include <sstream>
#include <iostream>
#include <vector>
#include <cctype>
#include <algorithm>

namespace {

std::vector<std::string> tokenize(const std::string& str) {
  std::vector<std::string> tokens;
  std::string token;
  for (size_t i = 0; i < str.length(); ++i) {
    char c = str[i];
    if (std::isspace(c)) {
      if (!token.empty()) {
        tokens.push_back(token);
        token.clear();
      }
    } else if (c == '(' || c == ')') {
      if (!token.empty()) {
        tokens.push_back(token);
        token.clear();
      }
      tokens.push_back(std::string(1, c));
    } else if (c == '>' || c == '<' || c == '=' || c == '!') {
      if (!token.empty()) {
        tokens.push_back(token);
        token.clear();
      }
      if (i + 1 < str.length() && str[i+1] == '=') {
        tokens.push_back(str.substr(i, 2));
        i++;
      } else {
        tokens.push_back(std::string(1, c));
      }
    } else {
      token += c;
    }
  }
  if (!token.empty()) {
    tokens.push_back(token);
  }
  return tokens;
}

class ExprParser {
  std::vector<std::string> tokens_;
  size_t pos_ = 0;

  std::string peek() {
    if (pos_ < tokens_.size()) return tokens_[pos_];
    return "";
  }

  std::string consume() {
    if (pos_ < tokens_.size()) return tokens_[pos_++];
    return "";
  }

  bool match(const std::string& expected) {
    if (peek() == expected) {
      consume();
      return true;
    }
    return false;
  }

public:
  ExprParser(const std::vector<std::string>& tokens) : tokens_(tokens) {}

  Expr* parse() {
    Expr* node = parseOr();
    if (pos_ < tokens_.size()) {
      throw std::runtime_error("Unexpected token: " + tokens_[pos_]);
    }
    return node;
  }

private:
  Expr* parseOr() {
    Expr* node = parseAnd();
    while (peek() == "OR" || peek() == "or") {
      consume();
      Expr* right = parseAnd();
      node = new Expr(Op::OR, "", "", node, right);
    }
    return node;
  }

  Expr* parseAnd() {
    Expr* node = parsePrimary();
    while (peek() == "AND" || peek() == "and") {
      consume();
      Expr* right = parsePrimary();
      node = new Expr(Op::AND, "", "", node, right);
    }
    return node;
  }

  Expr* parsePrimary() {
    if (match("(")) {
      Expr* node = parseOr();
      if (!match(")")) {
        throw std::runtime_error("Mismatched parentheses");
      }
      return node;
    }

    std::string colName = consume();
    std::string opStr = consume();
    std::string val = consume();

    if (colName.empty() || opStr.empty() || val.empty()) {
      throw std::runtime_error("Malformed comparison expression");
    }

    Op op;
    if (opStr == "=" || opStr == "==") op = Op::EQUAL;
    else if (opStr == "!=") op = Op::NOT_EQUAL;
    else if (opStr == ">") op = Op::GREATER;
    else if (opStr == ">=") op = Op::GREATER_EQUAL;
    else if (opStr == "<") op = Op::LESS;
    else if (opStr == "<=") op = Op::LESS_EQUAL;
    else {
      throw std::runtime_error("Invalid operator: " + opStr);
    }

    return new Expr(op, colName, val);
  }
};

} // namespace

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
                std::string whereClause;
                std::getline(ss, whereClause);
                
                try {
                    std::vector<std::string> tokens = tokenize(whereClause);
                    if (tokens.empty()) {
                        std::cout << "Error: Empty WHERE clause.\n";
                        return;
                    }
                    ExprParser parser(tokens);
                    Expr* expr = parser.parse();
                    results = table->selectWhere(expr);
                    delete expr;
                } catch (const std::exception& e) {
                    std::cout << "Error parsing WHERE clause: " << e.what() << "\n";
                    return;
                }
            } else {
                std::cout << "Error: Invalid SELECT syntax. Expected: SELECT <tableName> [WHERE <expression>]\n";
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

