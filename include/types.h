#pragma once

#include <string>
#include <vector>

enum class ColumnType {
  INT,
  TEXT
};

struct Column {
  std::string name;
  ColumnType type;
};

using Row = std::vector<std::string>;

enum class Op { EQUAL, NOT_EQUAL, GREATER, GREATER_EQUAL, LESS, LESS_EQUAL, AND, OR };

struct Expr {
  Op op;
  std::string colName;
  std::string value;
  Expr* left = nullptr;
  Expr* right = nullptr;

  Expr(Op o, std::string c, std::string v, Expr* l = nullptr, Expr* r = nullptr)
      : op(o), colName(c), value(v), left(l), right(r) {}

  ~Expr() {
    delete left;
    delete right;
  }
};





