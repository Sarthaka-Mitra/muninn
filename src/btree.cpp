#include "btree.h"
#include <cstring>
#include <stdexcept>
#include <iostream>

const size_t MAX_KEY_SIZE = 32;
const size_t HEADER_SIZE = 16;
const size_t ENTRY_SIZE = MAX_KEY_SIZE + sizeof(int32_t); // 36 bytes

// ===================== BTreeNode =====================

BTreeNode::BTreeNode(int t, bool leaf, uint32_t pageId)
    : t_(t), pageId_(pageId), leaf_(leaf) {}

PageBuffer BTreeNode::serialize(const BTreeNode& node) {
    PageBuffer buffer = {};

    // Header
    buffer[0] = node.leaf_ ? 1 : 0;
    uint16_t numKeys = static_cast<uint16_t>(node.keys_.size());
    std::memcpy(&buffer[1], &numKeys, sizeof(uint16_t));

    // Key-Value entries
    for (size_t i = 0; i < node.keys_.size(); i++) {
        size_t offset = HEADER_SIZE + (i * ENTRY_SIZE);

        // Write key (copy characters, rest is already zeroed)
        size_t copyLen = std::min(node.keys_[i].length(), MAX_KEY_SIZE);
        std::memcpy(&buffer[offset], node.keys_[i].c_str(), copyLen);

        // Write rowIndex
        int32_t rowIdx = static_cast<int32_t>(node.rowIndexes_[i]);
        std::memcpy(&buffer[offset + MAX_KEY_SIZE], &rowIdx, sizeof(int32_t));
    }

    // Children (only for internal nodes)
    if (!node.leaf_) {
        // Use max keys to calculate offset so children always start at same spot
        size_t childrenOffset = HEADER_SIZE + ((2 * node.t_ - 1) * ENTRY_SIZE);
        for (size_t i = 0; i < node.children_.size(); i++) {
            std::memcpy(&buffer[childrenOffset + (i * sizeof(uint32_t))],
                        &node.children_[i], sizeof(uint32_t));
        }
    }

    return buffer;
}

BTreeNode BTreeNode::deserialize(uint32_t pageId, const PageBuffer& buffer, int t) {
    bool isLeaf = (buffer[0] == 1);
    BTreeNode node(t, isLeaf, pageId);

    uint16_t numKeys;
    std::memcpy(&numKeys, &buffer[1], sizeof(uint16_t));

    // Read Key-Value entries
    for (uint16_t i = 0; i < numKeys; i++) {
        size_t offset = HEADER_SIZE + (i * ENTRY_SIZE);

        // Read key (stops at first null byte)
        std::string key(reinterpret_cast<const char*>(&buffer[offset]));
        node.keys_.push_back(key);

        // Read rowIndex
        int32_t rowIdx;
        std::memcpy(&rowIdx, &buffer[offset + MAX_KEY_SIZE], sizeof(int32_t));
        node.rowIndexes_.push_back(static_cast<int>(rowIdx));
    }

    // Read children (only for internal nodes)
    if (!isLeaf) {
        size_t childrenOffset = HEADER_SIZE + ((2 * t - 1) * ENTRY_SIZE);
        for (uint16_t i = 0; i <= numKeys; i++) {
            uint32_t childId;
            std::memcpy(&childId, &buffer[childrenOffset + (i * sizeof(uint32_t))],
                        sizeof(uint32_t));
            node.children_.push_back(childId);
        }
    }

    return node;
}

// ===================== BTreeIndex =====================

BTreeIndex::BTreeIndex(const std::string& filepath, int t)
    : pager_(filepath), rootPageId_(INVALID_PAGE_ID), t_(t) {
    if (pager_.getPageCount() == 0) {
        // Brand new index file — allocate page 0 as a metadata page
        pager_.allocatePage();
        // Write INVALID_PAGE_ID to metadata page (no root yet)
        PageBuffer metaBuf = {};
        std::memcpy(&metaBuf[0], &rootPageId_, sizeof(uint32_t));
        pager_.writePage(0, metaBuf);
    } else {
        // Existing index file — read root page ID from metadata page (page 0)
        PageBuffer metaBuf = pager_.readPage(0);
        std::memcpy(&rootPageId_, &metaBuf[0], sizeof(uint32_t));
    }
}

BTreeIndex::~BTreeIndex() {
    // Pager destructor handles file closing automatically
}

BTreeIndex::BTreeIndex(BTreeIndex&& other) noexcept
    : pager_(std::move(other.pager_)),
      rootPageId_(other.rootPageId_),
      t_(other.t_) {
    other.rootPageId_ = INVALID_PAGE_ID;
}

BTreeIndex& BTreeIndex::operator=(BTreeIndex&& other) noexcept {
    if (this != &other) {
        pager_ = std::move(other.pager_);
        rootPageId_ = other.rootPageId_;
        t_ = other.t_;
        other.rootPageId_ = INVALID_PAGE_ID;
    }
    return *this;
}

BTreeNode BTreeIndex::loadNode(uint32_t pageId) {
    PageBuffer buffer = pager_.readPage(pageId);
    return BTreeNode::deserialize(pageId, buffer, t_);
}

void BTreeIndex::saveNode(uint32_t pageId, const BTreeNode& node) {
    PageBuffer buffer = BTreeNode::serialize(node);
    pager_.writePage(pageId, buffer);
}

// ===================== Insert Logic =====================

void BTreeIndex::insert(const std::string& key, int rowIndex) {
    if (rootPageId_ == INVALID_PAGE_ID) {
        // Tree is empty — create the first leaf node
        uint32_t newPageId = pager_.allocatePage();
        BTreeNode root(t_, true, newPageId);
        root.keys_.push_back(key);
        root.rowIndexes_.push_back(rowIndex);
        saveNode(newPageId, root);

        // Update metadata page with new root
        rootPageId_ = newPageId;
        PageBuffer metaBuf = {};
        std::memcpy(&metaBuf[0], &rootPageId_, sizeof(uint32_t));
        pager_.writePage(0, metaBuf);
        return;
    }

    BTreeNode root = loadNode(rootPageId_);

    if ((int)root.keys_.size() == 2 * t_ - 1) {
        // Root is full — create a new root and split the old one
        uint32_t newRootPageId = pager_.allocatePage();
        BTreeNode newRoot(t_, false, newRootPageId);
        newRoot.children_.push_back(rootPageId_);
        saveNode(newRootPageId, newRoot);

        splitChild(newRootPageId, 0, rootPageId_);

        // Update metadata page with new root
        rootPageId_ = newRootPageId;
        PageBuffer metaBuf = {};
        std::memcpy(&metaBuf[0], &rootPageId_, sizeof(uint32_t));
        pager_.writePage(0, metaBuf);

        insertNonFull(rootPageId_, key, rowIndex);
    } else {
        insertNonFull(rootPageId_, key, rowIndex);
    }
}

void BTreeIndex::insertNonFull(uint32_t pageId, const std::string& key, int rowIndex) {
    BTreeNode node = loadNode(pageId);
    int i = (int)node.keys_.size() - 1;

    if (node.leaf_) {
        // Shift keys right to make room (same logic as your V2 code)
        node.keys_.push_back("");
        node.rowIndexes_.push_back(0);
        while (i >= 0 && key < node.keys_[i]) {
            node.keys_[i + 1] = node.keys_[i];
            node.rowIndexes_[i + 1] = node.rowIndexes_[i];
            i--;
        }
        node.keys_[i + 1] = key;
        node.rowIndexes_[i + 1] = rowIndex;
        saveNode(pageId, node);
    } else {
        // Find which child to descend into
        while (i >= 0 && key < node.keys_[i]) {
            i--;
        }
        i++;

        // Check if that child is full
        BTreeNode child = loadNode(node.children_[i]);
        if ((int)child.keys_.size() == 2 * t_ - 1) {
            splitChild(pageId, i, node.children_[i]);

            // Reload parent after split modified it
            node = loadNode(pageId);
            if (key > node.keys_[i]) {
                i++;
            }
        }
        insertNonFull(node.children_[i], key, rowIndex);
    }
}

void BTreeIndex::splitChild(uint32_t parentPageId, int i, uint32_t childPageId) {
    BTreeNode parent = loadNode(parentPageId);
    BTreeNode y = loadNode(childPageId);

    // Create new sibling node z
    uint32_t zPageId = pager_.allocatePage();
    BTreeNode z(t_, y.leaf_, zPageId);

    // Save the middle key BEFORE any resizing
    std::string midKey = y.keys_[t_ - 1];
    int midRowIdx = y.rowIndexes_[t_ - 1];

    // Copy upper half of y's keys into z
    for (int j = 0; j < t_ - 1; j++) {
        z.keys_.push_back(y.keys_[t_ + j]);
        z.rowIndexes_.push_back(y.rowIndexes_[t_ + j]);
    }

    // Copy upper half of y's children into z (if internal node)
    if (!y.leaf_) {
        for (int j = 0; j < t_; j++) {
            z.children_.push_back(y.children_[t_ + j]);
        }
    }

    // Truncate y to keep only the lower half
    y.keys_.resize(t_ - 1);
    y.rowIndexes_.resize(t_ - 1);
    if (!y.leaf_) {
        y.children_.resize(t_);
    }

    // Insert z as a new child of parent, and promote midKey
    parent.children_.insert(parent.children_.begin() + i + 1, zPageId);
    parent.keys_.insert(parent.keys_.begin() + i, midKey);
    parent.rowIndexes_.insert(parent.rowIndexes_.begin() + i, midRowIdx);

    // Save all three modified nodes back to disk
    saveNode(childPageId, y);
    saveNode(zPageId, z);
    saveNode(parentPageId, parent);
}

// ===================== Search Logic =====================

std::vector<int> BTreeIndex::search(const std::string& key) {
    std::vector<int> results;
    if (rootPageId_ == INVALID_PAGE_ID) return results;
    searchNode(rootPageId_, key, results);
    return results;
}

void BTreeIndex::searchNode(uint32_t pageId, const std::string& key, std::vector<int>& results) {
    BTreeNode node = loadNode(pageId);

    int i = 0;
    while (i < (int)node.keys_.size()) {
        if (key < node.keys_[i]) {
            // Key is smaller — search the left child, then stop
            if (!node.leaf_) {
                searchNode(node.children_[i], key, results);
            }
            return;
        } else if (key == node.keys_[i]) {
            // Check left child for more duplicates first
            if (!node.leaf_) {
                searchNode(node.children_[i], key, results);
            }
            // Collect this match
            results.push_back(node.rowIndexes_[i]);
            i++;
            // Continue scanning for more duplicates in this node
        } else {
            // key > keys_[i], move to the next key
            i++;
        }
    }

    // After all keys, check the rightmost child
    if (!node.leaf_) {
        searchNode(node.children_[i], key, results);
    }
}
