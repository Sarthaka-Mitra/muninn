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
