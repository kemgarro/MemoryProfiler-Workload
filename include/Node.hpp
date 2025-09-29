#pragma once

struct Node {
    int value;
    Node* left = nullptr;
    Node* right = nullptr;

    Node(int v) : value(v) {}

    static int countNodes(Node* root);
    static int getDepth(Node* root);
    static void deleteTree(Node* root);
};