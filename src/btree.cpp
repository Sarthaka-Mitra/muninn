#include "btree.h"

BTreeNode::BTreeNode(int t, bool leaf)
  : t_(t), leaf_(leaf){}

void BTreeNode::insertNonFull(const std::string& key, int rowIndex) {
    // Start i at the index of the last element
    int i = keys_.size() - 1;

    if (leaf_) {
        // Step 1: Add a dummy slot at the end to make room
        keys_.push_back("");
        rowIndexes_.push_back(0);

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
        rowIndexes_[i + 1] = rowIndex;
        
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
    int midRow=y->rowIndexes_[t_-1];
    y->keys_.resize(t_-1);
    y->rowIndexes_.resize(t_-1);
    if (!y->leaf_) y->children_.resize(t_);
    
    children_.insert(children_.begin()+i+1,z);

    keys_.insert(keys_.begin()+i,midKey);
    rowIndexes_.insert(rowIndexes_.begin()+i,midRow);
}


