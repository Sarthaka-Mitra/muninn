#pragma once

#include <string>
#include <vector>
#include "types.h"

class Table {
public:
  //Constructor
  Table(std::string name, std::vector<Column> schema);

  //Insert a row
  bool insertRow(Row row);

  //Read operations
  std::vector<Row>    getRows() const;
  std::vector<Column> getSchema() const;
  std::string         getName() const;

private:
  std::string         name_;
  std::vector<Column> schema_;
  std::vector<Row>    rows_;
};
