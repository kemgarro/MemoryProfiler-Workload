#include "Node.hpp"
#include <algorithm>

int Node::countNodes(Node* root) {
    if (!root) return 0;
    return 1 + countNodes(root->left) + countNodes(root->right);
}

int Node::getDepth(Node* root) {
    if (!root) return 0;
    return 1 + std::max(getDepth(root->left), getDepth(root->right));
}

void Node::deleteTree(Node* root) {
    if (!root) return;
    deleteTree(root->left);
    deleteTree(root->right);
    delete root;
}
