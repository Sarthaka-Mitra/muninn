#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include "types.h"
#include "btree.h"

class Table {
public:
  //Constructor
  Table(std::string name, std::vector<Column> schema);

  //Insert a row
  bool insertRow(Row row, bool isLoad = false);

  //Read operations
  std::vector<Row>    getRows() const;
  std::vector<Column> getSchema() const;
  std::string         getName() const;
  
  bool createIndex(const std::string& columnName);
  std::vector<Row> selectWhere(const std::string& columnName, const std::string& value);
private:
  std::string         name_;
  std::vector<Column> schema_;
  std::vector<Row>    rows_;
  std::unordered_map<std::string, BTreeIndex> indexes_;

  //Operations
  int getColumnIndex(const std::string& columnName) const;
};
