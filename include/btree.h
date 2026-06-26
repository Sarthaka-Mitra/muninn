#pragma once
#include <vector>
#include <string>
#include <algorithm>

// A node in the B-Tree
class BTreeNode {
    friend class BTreeIndex;
public:
    BTreeNode(int t, bool leaf);
    void insertNonFull(const std::string& key, int rowIndex);
    // i is the index of the child we are splitting, y is the child itself
    void splitChild(int i, BTreeNode* y);
    size_t keyCount() const;
    std::vector<int> search(const std::string& key);

private:
    
    int t_; // Minimum degree
    std::vector<std::vector<int>> rowIndexes_;
    bool leaf_;
    std::vector<BTreeNode*> children_;
    std::vector<std::string> keys_;

    std::vector<int>* searchRef(const std::string& key);
};

class BTreeIndex {
public:
    BTreeIndex(int t = 3);
    void insert(const std::string& key, int rowIndex);
    std::vector<int> search(const std::string& key);
    std::vector<int>* searchRef(const std::string& key);
private:
    BTreeNode* root_;
    int t_;
};


