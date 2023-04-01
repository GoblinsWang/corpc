import random

class TreeNode:
    def __init__(self, val):
        self.val = val
        self.left = None
        self.right = None

class BinaryTree:
    def __init__(self):
        self.root = None
        self.size = 0

    def countNodes(self, root):
        if root is None:
            return 0
        else:
            return 1 + self.countNodes(root.left) + self.countNodes(root.right)

    def randomDelete(self, root, num):
        if root is None:
            return None
        if num == 1:
            if root.left is None and root.right is None:
                return None
            elif root.left is None:
                return root.right
            elif root.right is None:
                return root.left
            else:
                minNode = self.getMin(root.right)
                root.val = minNode.val
                root.right = self.randomDelete(root.right, minNode.val)
        elif num <= self.countNodes(root.left) + 1:
            root.left = self.randomDelete(root.left, num - 1)
        else:
            root.right = self.randomDelete(root.right, num - self.countNodes(root.left) - 1)
        return root

    def deleteNode(self, val):
        if self.root is None:
            return
        if self.root.val == val:
            if self.root.left is None and self.root.right is None:
                self.root = None
            elif self.root.left is None:
                self.root = self.root.right
            elif self.root.right is None:
                self.root = self.root.left
            else:
                minNode = self.getMin(self.root.right)
                self.root.val = minNode.val
                self.root.right = self.randomDelete(self.root.right, minNode.val)
        else:
            self.deleteNodeHelper(self.root, val)

    def deleteNodeHelper(self, root, val):
        if root is None:
            return
        if root.left is not None and root.left.val == val:
            if root.left.left is None and root.left.right is None:
                root.left = None
            elif root.left.left is None:
                root.left = root.left.right
            elif root.left.right is None:
                root.left = root.left.left
            else:
                minNode = self.getMin(root.left.right)
                root.left.val = minNode.val
                root.left.right = self.randomDelete(root.left.right, minNode.val)
        elif root.right is not None and root.right.val == val:
            if root.right.left is None and root.right.right is None:
                root.right = None
            elif root.right.left is None:
                root.right = root.right.right
            elif root.right.right is None:
                root.right = root.right.left
            else:
                minNode = self.getMin(root.right.right)
                root.right.val = minNode.val
                root.right.right = self.randomDelete(root.right.right, minNode.val)
        else:
            self.deleteNodeHelper(root.left, val)
            self.deleteNodeHelper(root.right, val)

