#pragma once
#include <vector>
#include <string>
#include <algorithm>
#include "pager.h"

const uint32_t INVALID_PAGE_ID = 0xFFFFFFFF;

class BTreeNode {
    friend class BTreeIndex;
public:
    BTreeNode(int t, bool leaf, uint32_t pageId = INVALID_PAGE_ID);
    
    // Binary serialization helpers
    static PageBuffer serialize(const BTreeNode& node);
    static BTreeNode deserialize(uint32_t pageId, const PageBuffer& buffer, int t);

private:
    int t_; 
    uint32_t pageId_; 
    bool leaf_;
    
    std::vector<std::string> keys_;
    std::vector<int> rowIndexes_;      // Flat row index list (1-to-1)
    std::vector<uint32_t> children_;   // Child Page IDs
};

class BTreeIndex {
public:
    BTreeIndex(const std::string& filepath, int t = 3);
    
    void insert(const std::string& key, int rowIndex);
    std::vector<int> search(const std::string& key);

    ~BTreeIndex();
    
    BTreeIndex(const BTreeIndex&) = delete;
    BTreeIndex& operator=(const BTreeIndex&) = delete;
    BTreeIndex(BTreeIndex&& other) noexcept;
    BTreeIndex& operator=(BTreeIndex&& other) noexcept;

private:
    Pager pager_;
    uint32_t rootPageId_;
    int t_;

    // Helper functions to load/save nodes using the Pager
    BTreeNode loadNode(uint32_t pageId);
    void saveNode(uint32_t pageId, const BTreeNode& node);

    // Tree traversal logic moved here (Option B)
    void insertNonFull(uint32_t pageId, const std::string& key, int rowIndex);
    void splitChild(uint32_t parentPageId, int i, uint32_t childPageId);

    void searchNode(uint32_t pageId, const std::string& key, std::vector<int>& results);
};
