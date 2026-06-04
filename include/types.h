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




