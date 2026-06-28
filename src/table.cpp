#include <cctype>
#include <cstddef>
#include <string>
#include <vector>
#include "table.h"
#include "btree.h"
#include "types.h"
#include "storage.h"
#include <filesystem>

Table::Table(std::string name, std::vector<Column> schema)
    : name_(name), schema_(schema) {


}

std::string Table::getName() const {
  return name_;
}

std::vector<Row> Table::getRows() const {
  return rows_;
}

std::vector<Column> Table::getSchema() const {
  return schema_;
}

bool Table::insertRow(Row row, bool isLoad){
  if (schema_.size()!=row.size())
    return false;

  for (size_t i = 0; i < schema_.size(); i++) {
    if (schema_[i].type == ColumnType::INT) {
      const std::string& value = row[i];

      if (value.empty()) return false;

      size_t start_idx = 0;

      if (value[0] == '-'){
        if (value.length()==1) return false;
        start_idx=1;
      }

      for (size_t j=start_idx; j<value.length();j++){
        if (!std::isdigit(value[j])) {
          return false;
        }
      }
    }
  }

  rows_.push_back(row);
  
  int rowIndex=rows_.size()-1;
  for (auto& pair: indexes_) {
    const std::string& colName=pair.first;
    BTreeIndex& index=pair.second;
    int colIdx=getColumnIndex(colName);
    index.insert(row[colIdx], rowIndex);
  }
  
  if (!isLoad) {
    StorageEngine::saveTable(*this);
  }
  return true;
}

int Table::getColumnIndex(const std::string& columnName) const{
  for (size_t i=0; i<schema_.size(); i++) {
    if (schema_[i].name == columnName) {
      return i;
    }
  }
  return -1;
}

bool Table::createIndex(const std::string& columnName) {
  int index = getColumnIndex(columnName);
  if (index==-1)
    return false;
  if (indexes_.find(columnName)!= indexes_.end())
      return true;
  std::string indexPath = "../data/" + name_ + "_" + columnName + ".idx";
  bool indexExists = std::filesystem::exists(indexPath);
  indexes_.emplace(columnName, BTreeIndex(indexPath, 3));

  if (!indexExists) {
    auto it = indexes_.find(columnName);
    for (size_t i = 0; i < rows_.size(); i++) {
        it->second.insert(rows_[i][index], i);
    }
  }  
  return true;
}

std::vector<Row> Table::selectWhere(const std::string& columnName, const std::string& value) {
  int colIdx=getColumnIndex(columnName);
  if (colIdx==-1)
    return {};

  std::vector<Row> results;

  auto it=indexes_.find(columnName);
  if (it!=indexes_.end()){
    std::vector<int> rowIndices = it->second.search(value);
    for (int idx:rowIndices)
      results.push_back(rows_[idx]);
  } else {
    for (const auto& r: rows_) {
      if (r[colIdx]==value)
        results.push_back(r);
    }
  }

  return results;
}

namespace {

bool evaluateExpr(Expr* expr, const Row& row, const Table& table) {
  if (!expr) return true;
  if (expr->op == Op::AND) {
    return evaluateExpr(expr->left, row, table) && evaluateExpr(expr->right, row, table);
  }
  if (expr->op == Op::OR) {
    return evaluateExpr(expr->left, row, table) || evaluateExpr(expr->right, row, table);
  }

  int colIdx = table.getColumnIndex(expr->colName);
  if (colIdx == -1) return false;
  const std::string& cellValue = row[colIdx];
  const std::string& queryValue = expr->value;

  ColumnType colType = table.getSchema()[colIdx].type;

  if (colType == ColumnType::INT) {
    try {
      int cellInt = std::stoi(cellValue);
      int queryInt = std::stoi(queryValue);
      switch (expr->op) {
        case Op::EQUAL: return cellInt == queryInt;
        case Op::NOT_EQUAL: return cellInt != queryInt;
        case Op::GREATER: return cellInt > queryInt;
        case Op::GREATER_EQUAL: return cellInt >= queryInt;
        case Op::LESS: return cellInt < queryInt;
        case Op::LESS_EQUAL: return cellInt <= queryInt;
        default: return false;
      }
    } catch (...) {
      return false;
    }
  } else {
    switch (expr->op) {
      case Op::EQUAL: return cellValue == queryValue;
      case Op::NOT_EQUAL: return cellValue != queryValue;
      case Op::GREATER: return cellValue > queryValue;
      case Op::GREATER_EQUAL: return cellValue >= queryValue;
      case Op::LESS: return cellValue < queryValue;
      case Op::LESS_EQUAL: return cellValue <= queryValue;
      default: return false;
    }
  }
}

bool findIndexFilter(Expr* expr, const Table& table, std::string& outCol, std::string& outVal) {
  if (!expr) return false;
  if (expr->op == Op::EQUAL) {
    if (table.hasIndex(expr->colName)) {
      outCol = expr->colName;
      outVal = expr->value;
      return true;
    }
  }
  if (expr->op == Op::AND) {
    if (findIndexFilter(expr->left, table, outCol, outVal)) return true;
    if (findIndexFilter(expr->right, table, outCol, outVal)) return true;
  }
  return false;
}

} // namespace

bool Table::hasIndex(const std::string& columnName) const {
  return indexes_.find(columnName) != indexes_.end();
}

std::vector<Row> Table::selectWhere(Expr* expr) {
  std::vector<Row> results;
  std::string indexCol, indexVal;
  
  if (findIndexFilter(expr, *this, indexCol, indexVal)) {
    auto it = indexes_.find(indexCol);
    std::vector<int> candidateIndices = it->second.search(indexVal);
    for (int idx : candidateIndices) {
      if (evaluateExpr(expr, rows_[idx], *this)) {
        results.push_back(rows_[idx]);
      }
    }
  } else {
    for (const auto& row : rows_) {
      if (evaluateExpr(expr, row, *this)) {
        results.push_back(row);
      }
    }
  }
  return results;
}

