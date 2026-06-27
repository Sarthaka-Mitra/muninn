#include "storage.h"
#include <cstddef>
#include <cstdio>
#include <fstream>
#include <iostream>

std::string StorageEngine::getFilePath(const std::string& tableName) {
  return "../data/" + tableName + ".db";
}

bool StorageEngine::saveTable(const Table &table) {
  std::string filePath=getFilePath(table.getName());

  std::ofstream outFile(filePath, std::ios::binary | std::ios::trunc);
  if (!outFile.is_open()) return false;

  std::vector<Row> rows=table.getRows();

  for (const Row &row : rows){
    for (const std::string& value : row){
      size_t size = value.size();
      outFile.write(reinterpret_cast<const char*>(&size), sizeof(size_t));
      
      outFile.write(value.c_str(), size);

    }
  }

  outFile.close();
  return true;
}

bool StorageEngine::loadTable(Table& table){
  std::string filePath=getFilePath(table.getName());

  std::ifstream inFile(filePath, std::ios::binary);
  if (!inFile.is_open()) return false;

  size_t columnsPerRow = table.getSchema().size();

  Row currentRow;

  while (inFile.peek() != EOF) {
    size_t size;
    if (!inFile.read(reinterpret_cast<char*>(&size), sizeof(size_t))) break;

    std::string value(size, '\0');
    inFile.read(&value[0], size);

    currentRow.push_back(value);

    if (columnsPerRow==currentRow.size()){
      table.insertRow(currentRow, true);
      currentRow.clear();
    }
  }

  inFile.close();
  return true;
}
