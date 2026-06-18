#pragma once
#include <vector>
#include <string>
#include <algorithm>

// A node in the B-Tree
class BTreeNode {
public:
    BTreeNode(int t, bool leaf);
    void insertNonFull(const std::string& key, int rowIndex);
    // i is the index of the child we are splitting, y is the child itself
    void splitChild(int i, BTreeNode* y); 


private:
    
    int t_; // Minimum degree
    std::vector<int> rowIndexes_;
    bool leaf_;
    std::vector<BTreeNode*> children_;
    std::vector<std::string> keys_;
};

