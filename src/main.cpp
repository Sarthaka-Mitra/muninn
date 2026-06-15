#include <iostream>
#include <vector>
#include "table.h"
#include "types.h"
#include "storage.h"

int main() {
  std::vector<Column> schema = {
  {"name", ColumnType::TEXT},
  {"age", ColumnType::INT}
  };

  Table studentsTable("students", schema);

  std::cout<<"Table created successfully: " << studentsTable.getName() << "\n";

  Row validRow={"Arjun","21"};
  bool success1=studentsTable.insertRow(validRow);
  std::cout<<"Valid insert (should be 1): " << success1 << "\n";

  Row invalidRow={"Priya","Twenty"};
  bool success2=studentsTable.insertRow(invalidRow);
  std::cout<<"Invalid insert (should be 0):" << success2 << "\n";

  Row wrongSize={"Rahul"};
  bool success3=studentsTable.insertRow(wrongSize);
  std::cout<<"Invalid insert (should be 0):" << success3 << "\n";
  
  bool saved=StorageEngine::saveTable(studentsTable);
  std::cout<<"Save successful:"<<saved<<"\n";

  Table loadedTable("students", schema);
  StorageEngine::loadTable(loadedTable);

  std::cout << "Loaded name: " << loadedTable.getRows()[0][0] << "\n";

  return 0;
}
