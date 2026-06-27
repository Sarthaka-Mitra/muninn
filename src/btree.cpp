#include "btree.h"
#include <cstddef>
#include <string>
#include <vector>

BTreeNode::BTreeNode(int t, bool leaf)
  : t_(t), leaf_(leaf){}

void BTreeNode::insertNonFull(const std::string& key, int rowIndex) {
    // Start i at the index of the last element
    int i = keys_.size() - 1;

    if (leaf_) {
        // Step 1: Add a dummy slot at the end to make room
        keys_.push_back("");
        rowIndexes_.push_back({});

        // Step 2 & 3: Shift elements to the right until we find the right spot
        while (i >= 0 && keys_[i] > key) {
            // TODO: Move the key at index 'i' to index 'i + 1'
            // TODO: Do the exact same thing for rowIndexes_ !
            keys_[i+1]=keys_[i];
            rowIndexes_[i+1]=rowIndexes_[i];
            i--; // Move pointer left
        }

        // Step 6: Drop the new values into the space we just created
        keys_[i + 1] = key;
        rowIndexes_[i + 1] = {rowIndex};
        
    } else {
        while (i>=0 && keys_[i]>key) {
        i--;
        }
      if (children_[i+1]->keys_.size()==2*t_-1){
        splitChild(i+1, children_[i+1]);
        if (keys_[i+1]<key){
          i++;
        }
      }

      

      children_[i+1]->insertNonFull(key, rowIndex);
    }
}

void BTreeNode::splitChild(int i, BTreeNode* y) {
    // 1. Create the new node Z. It has the same 'leaf' status as Y.
    BTreeNode* z = new BTreeNode(y->t_, y->leaf_);
    
    // 2. Move the right half of Y's keys and rowIndexes into Z
    // Y currently has (2t - 1) keys. We want to move the last (t - 1) keys into Z.
    for (int j = 0; j < t_ - 1; j++) {
        // TODO: push_back the correct key from 'y' into 'z'
        // Hint: The index in 'y' starts at j + t_
        z->keys_.push_back(y->keys_[j+t_]);
        z->rowIndexes_.push_back(y->rowIndexes_[j+t_]);
        // TODO: do the same for rowIndexes_
    }
    
    // 3. If Y is not a leaf, we also have to move the right half of its children pointers!
    if (!y->leaf_) {
        for (int j = 0; j < t_; j++) {
            // TODO: push_back the correct child pointer from 'y->children_' into 'z->children_'
            z->children_.push_back(y->children_[j+t_]);
        }
    }
    
    // (We will finish the rest of this method in the next step)
    std::string midKey=y->keys_[t_-1];
  std::vector<int> midRow=y->rowIndexes_[t_-1];
    y->keys_.resize(t_-1);
    y->rowIndexes_.resize(t_-1);
    if (!y->leaf_) y->children_.resize(t_);
    
    children_.insert(children_.begin()+i+1,z);

    keys_.insert(keys_.begin()+i,midKey);
    rowIndexes_.insert(rowIndexes_.begin()+i,midRow);
}

size_t BTreeNode::keyCount() const { return keys_.size(); }

std::vector<int> BTreeNode::search(const std::string& key) {
    int i = 0;
    // 1. Loop to find the first key greater than or equal to 'key'
    while (i < keys_.size() && key > keys_[i]) {
        i++;
    }

    // 2. If the key matches keys_[i], return the rowIndex at keys_[i]
    if (i < keys_.size() && keys_[i] == key) {
        return rowIndexes_[i]; // Returns a vector containing the single rowIndex
    }

    // 3. If it's a leaf node, the key is not in the tree
    if (leaf_) {
        return {}; // Returns empty vector
    }

    // 4. Otherwise, recursively search the correct child
    // TODO: Write the recursive call to search children_[i]
    return children_[i]->search(key);
}

std::vector<int>* BTreeNode::searchRef(const std::string& key) {
    int i = 0;
    while (i < keys_.size() && key > keys_[i]) {
        i++;
    }
    if (i < keys_.size() && keys_[i] == key) {
        return &rowIndexes_[i];
    }
    if (leaf_) {
        return nullptr;
    }
    return children_[i]->searchRef(key);
}

BTreeNode::~BTreeNode() {
  if (!leaf_) {
    for (BTreeNode* child: children_) {
      delete child;
    }
  }
}


//--------------End of BTreeNode method implementation-------------//

BTreeIndex::BTreeIndex(int t)
  : root_(nullptr), t_(t){} //Constructor


void BTreeIndex::insert(const std::string& key, int rowIndex) {
    std::vector<int>* existing = searchRef(key);
    if (existing != nullptr) {
        existing->push_back(rowIndex);
        return; // Done! No tree modifications needed.
    }
    // Case 1: Tree is empty
    if (root_ == nullptr) {
        // TODO: Create a new leaf BTreeNode and assign to root_
        // TODO: Call insertNonFull on root_ with key and rowIndex
        root_ = new BTreeNode(t_, true);
        root_->insertNonFull(key, rowIndex);
        
    } else if (root_->keyCount() < 2*t_ - 1) {
        // Case 2: Root exists and has space
        // TODO: One line — delegate directly to root_
        root_->insertNonFull(key, rowIndex);
        
    } else {
        // Case 3: Root is full — tree must grow upward
        BTreeNode* oldRoot = root_;
        
        // TODO: Create a brand new NON-LEAF node and assign it to root_
        root_= new BTreeNode(t_, false);
        // TODO: Make oldRoot the first child of the new root_
        //       (Hint: root_->children_ is a vector of BTreeNode*)
        root_->children_.push_back(oldRoot);
        // TODO: Call splitChild(0, oldRoot) on root_
        root_->splitChild(0, oldRoot);
        // After the split, the new root has one key (the promoted middle key)
        // Decide which child to insert into
        int i = 0;
        if (root_->keys_[0]< key) {
        i=1;
    }
       // TODO: Call insertNonFull on children_[i] of root_
      root_->children_[i]->insertNonFull(key, rowIndex);
  }
}

std::vector<int>BTreeIndex::search(const std::string& key) {
    if (root_==nullptr) {
    return {};
  }
  return root_->search(key);
}

std::vector<int>* BTreeIndex::searchRef(const std::string& key) {
    if (root_ == nullptr) return nullptr;
    return root_->searchRef(key);
}

BTreeIndex::~BTreeIndex() {
  delete root_;
}

// Move constructor
BTreeIndex::BTreeIndex(BTreeIndex&& other) noexcept 
  : root_(other.root_), t_(other.t_) {
    other.root_ = nullptr; // Nullify the old owner so it doesn't delete our root!
}

// Move assignment operator
BTreeIndex& BTreeIndex::operator=(BTreeIndex&& other) noexcept {
    if (this != &other) {
        delete root_;        // Clean up our own existing root first
        root_ = other.root_; // Take ownership of other's root
        t_ = other.t_;
        other.root_ = nullptr; // Nullify other
    }
    return *this;
}

