#include <cctype>
#include <cstddef>
#include <string>
#include <vector>
#include "table.h"
#include "types.h"

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

bool Table::insertRow(Row row){
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
  return true;


}
