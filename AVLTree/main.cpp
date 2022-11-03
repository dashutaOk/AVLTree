#include <iostream>
#include <map>
#include <utility>
#include <functional>
#include <memory>
#include <iterator>

namespace TreeLib {

    template <typename KeyT, typename ValT>
    class Node {
    public:
        KeyT key;
        ValT value;
        std::shared_ptr<Node> left;
        std::shared_ptr<Node> right;
        std::weak_ptr<Node> parent;
        uint32_t height;
        Node(const KeyT& key, const ValT& val)
                : key(key), value(val), left(nullptr), right(nullptr), height(1) {

        }
    };

    template <typename KeyT, typename ValT, typename Allocator = std::allocator<Node<KeyT, ValT>>>
    class AVLMap {
    public:

        template<bool IsConst = false>
        class Iterator {
        public:

            typedef std::pair<
                    std::reference_wrapper<const KeyT>,
                    std::reference_wrapper<typename std::conditional<IsConst, const ValT, ValT>::type>
            > value_type;

            typedef const Node<KeyT, ValT>* pointer;
            typedef std::bidirectional_iterator_tag iterator_category;
            typedef int difference_type;


            Iterator(const Iterator& other) : node_(other.node_) {
            }

            Iterator<IsConst>& operator=(const Iterator<IsConst>& other) {
                if (this != &other) {
                    node_ = other.node_;
                }
                return *this;
            }

            value_type operator*() {
                auto node_ptr = node_.lock();
                return std::make_pair(std::cref(node_ptr->key), std::ref(node_ptr->value));
            }

            pointer* operator ->() {
                return node_;
            }

            Iterator<IsConst>& operator++() {
                auto node_ptr = node_.lock();
                if (node_ptr == nullptr) {
                    throw std::out_of_range("Iterator out of range");
                }
                if (node_ptr->right != nullptr) {
                    node_ = NextRight(node_ptr->right);
                } else {
                    node_ = NextTopPlus(node_ptr->parent);
                }
                return *this;
            }

            Iterator<IsConst>& operator--() {
                if (node_ == nullptr) {
                    node_ = AVLMap::GetRight(tree_root_);
                }
                if (node_->left != nullptr) {
                    node_ = NextLeft(node_->left);
                } else {
                    node_ = NextTopMinus(node_->parent);
                }
                return *this;
            }

            Iterator<IsConst> operator++(int) {
                Iterator it = *this;
                ++*this;
                return it;
            }

            Iterator<IsConst> operator--(int) {
                Iterator it = *this;
                --*this;
                return it;
            }

            bool operator==(const Iterator<IsConst>& other) {
                return node_.lock() == other.node_.lock();
            }

            bool operator!=(const Iterator<IsConst>& other) {
                return node_.lock() != other.node_.lock();
            }

        private:

            std::shared_ptr<Node<KeyT, ValT>> NextRight(std::weak_ptr<Node<KeyT, ValT>> node) {
                auto node_shared = node.lock();
                if (node_shared->left == nullptr) {
                    return node_shared;
                }
                return NextRight(node_shared->left);
            }

            std::shared_ptr<Node<KeyT, ValT>> NextTopPlus(std::weak_ptr<Node<KeyT, ValT>> node) {
                auto node_shared = node.lock();
                if (node_shared == nullptr) {
                    return nullptr;
                }
                if (node_shared->key > node_.lock()->key) {
                    return node_shared;
                }
                return NextTopPlus(node_shared->parent);
            }

            std::shared_ptr<Node<KeyT, ValT>> NextLeft(std::weak_ptr<Node<KeyT, ValT>> node) {
                if (node->right == nullptr) {
                    return node;
                }
                return NextLeft(node->right);
            }

            std::shared_ptr<Node<KeyT, ValT>> NextTopMinus(std::weak_ptr<Node<KeyT, ValT>> node) {
                if (node == nullptr) {
                    return nullptr;
                }
                if (node->key < node_->key) {
                    return node;
                }
                return NextTopMinus(node->parent);
            }

            friend AVLMap;

            Iterator(std::weak_ptr<Node<KeyT, ValT>> node, std::weak_ptr<Node<KeyT, ValT>> tree_root)
                    : node_(std::move(node)), tree_root_(std::move(tree_root)) {
            }

            std::weak_ptr<Node<KeyT, ValT>> tree_root_;
            std::weak_ptr<Node<KeyT, ValT>> node_;
        };

        typedef Iterator<false> iterator;
        typedef Iterator<true> const_iterator;

        typedef typename iterator::value_type value_type;

        AVLMap(): root_(nullptr) {
        }

        AVLMap(const AVLMap<KeyT, ValT, Allocator>& other) {
            root_ = MakeCopy(other.root_);
        }

        AVLMap<KeyT, ValT, Allocator>& operator=(const AVLMap<KeyT, ValT, Allocator>& other) {
            if (this != &other) {
                root_ = MakeCopy(other);
            }
            return *this;
        }

        ~AVLMap() = default;

        void Insert(const KeyT& key, const ValT& val) {
            Insert(nullptr, root_, key, val);
        }

        void Delete(const KeyT& key) {
            root_ = Delete(root_, key);
        }

        ValT& Get(const KeyT& key) {
            return Get(root_, key);
        }

        size_t TreeHeight() {
            return height(root_);
        }

        bool Contains(const KeyT& key) {
            return Contains(root_, key);
        }

        ValT& operator [] (const KeyT& key) {
            if (!Contains(key)) {
                Insert(key, ValT());
            }
            return Get(key);
        }

        iterator begin() {
            return iterator(GetLeft(root_), root_);
        }

        iterator end() {
            return iterator(std::shared_ptr<Node<KeyT, ValT>>(nullptr), root_);
        }

        const_iterator cbegin() {
            return const_iterator(GetLeft(root_), root_);
        }

        const_iterator cend() {
            return const_iterator(std::shared_ptr<Node<KeyT, ValT>>(nullptr), root_);
        }

        std::reverse_iterator<iterator> rbegin() {
            return std::reverse_iterator(end());
        }

        std::reverse_iterator<iterator> rend() {
            return std::reverse_iterator(begin());
        }

        std::reverse_iterator<const_iterator> crbegin() {
            return std::reverse_iterator(cend());
        }

        std::reverse_iterator<iterator> crend() {
            return std::reverse_iterator(cbegin());
        }

    private:

        static std::shared_ptr<Node<KeyT, ValT>> GetLeft(std::shared_ptr<Node<KeyT, ValT>> root) {
            if (root == nullptr) {
                return nullptr;
            }
            if (root->left == nullptr) {
                return root;
            }
            return GetLeft(root->left);
        }

        static std::shared_ptr<Node<KeyT, ValT>> GetRight(std::shared_ptr<Node<KeyT, ValT>> root) {
            if (root == nullptr) {
                return nullptr;
            }
            if (root->right == nullptr) {
                return root;
            }
            return GetRight(root->right);
        }

        std::shared_ptr<Node<KeyT, ValT>> Delete(std::shared_ptr<Node<KeyT, ValT>>& root, const KeyT& key) {
            if (root == nullptr) {
                return nullptr;
            }

            if (root->key == key) {
                std::shared_ptr<Node<KeyT, ValT>> result;
                if (root->left != nullptr) {
                    std::shared_ptr<Node<KeyT, ValT>> replace;
                    root->left = GetMaxLeft(root, replace);
                    replace->parent = std::move(root->parent);
                    replace->left = std::move(root->left);
                    replace->right = std::move(root->right);
                    if (replace->left != nullptr) {
                        replace->left->parent = replace;
                    }
                    if (replace->right != nullptr) {
                        replace->right->parent = replace;
                    }
                    FixHeight(replace);
                    result = replace;
                } else {
                    result = std::move(root->right);
                    result->parent = root->parent;
                }
                FixHeight(result);
                BalanceNode(result);
                return result;
            } else if (key < root->key) {
                root->right = Delete(root->right, key);
            } else {
                root->left = Delete(root->left, key);
            }
            FixHeight(root);
            BalanceNode(root);
            return root;
        }

        static std::shared_ptr<Node<KeyT, ValT>> GetMaxLeft(
                std::shared_ptr<Node<KeyT, ValT>> node,
                std::shared_ptr<Node<KeyT, ValT>>& result
        ) {
            if (node->right != nullptr) {
                node->right = GetMaxLeft(node->right, result);
                FixHeight(node->right);
                return node;
            }
            auto to_return = node->left;
            to_return->parent = std::move(node->parent);
            result = std::move(node);
            FixHeight(result);
            return to_return;
        }

        void PostOrderGo(
                const std::function<void(std::shared_ptr<Node<KeyT, ValT>>)>& callback,
                std::shared_ptr<Node<KeyT, ValT>> root
        ) {
            if (root == nullptr) {
                return;
            }
            PostOrderGo(callback, root->left);
            PostOrderGo(callback, root->right);
            callback(root);
        }

        static ValT& Get(std::shared_ptr<Node<KeyT, ValT>> node, const KeyT& key) {
            if (node == nullptr) {
                throw std::runtime_error("Not found");
            }
            if (node->key == key) {
                return node->value;
            } else if (node->key > key) {
                return Get(node->left, key);
            } else {
                return Get(node->right, key);
            }
        }

        static bool Contains(std::shared_ptr<Node<KeyT, ValT>> node, const KeyT& key) {
            if (node == nullptr) {
                return false;
            }
            if (node->key == key) {
                return true;
            }
            if (node->key > key) {
                return Contains(node->left, key);
            }
            return Contains(node->right, key);
        }

        void Insert(
                std::shared_ptr<Node<KeyT, ValT>> parent,
                std::shared_ptr<Node<KeyT, ValT>>& node,
                const KeyT& key,
                const ValT& val
        ) {
            if (node == nullptr) {
                node = std::allocate_shared<Node<KeyT, ValT>>(allocator_, key, val);
                node->parent = parent;
                return;
            }
            if (node->key == key) {
                node->value = val;
            }
            if (node->key > key) {
                Insert(node, node->left, key, val);
            } else {
                Insert(node, node->right, key, val);
            }
            FixHeight(node);
            BalanceNode(node);
        }

        static void BalanceNode(std::shared_ptr<Node<KeyT, ValT>>& node) {
            if (BFactor(node) == 2) {
                if (BFactor(node->right) == -1) {
                    RotateRight(node->right);
                }
                RotateLeft(node);
            } else if (BFactor(node) == -2) {
                if (BFactor(node->left) == 1) {
                    RotateLeft(node->left);
                }
                RotateRight(node);
            }
        }

        static int BFactor(std::shared_ptr<Node<KeyT, ValT>> node) {
            return node == nullptr ? 0 : height(node->right) - height(node->left);
        }

        static int height(std::shared_ptr<Node<KeyT, ValT>> node) {
            return node == nullptr ? 0 : node->height;
        }

        static void FixHeight(std::shared_ptr<Node<KeyT, ValT>> node) {
            if (node == nullptr) {
                return;
            }
            node->height = 1 + std::max(height(node->left), height(node->right));
        }

        static void RotateLeft(std::shared_ptr<Node<KeyT, ValT>>& p) {
            std::shared_ptr<Node<KeyT, ValT>> q = p->right;
            p->right = q->left;
            if (q->left != nullptr) {
                q->left->parent = p;
            }
            q->left = p;
            FixHeight(p);
            q->parent = p->parent;
            p->parent = q;
            p = q;
            FixHeight(q);
        }

        static void RotateRight(std::shared_ptr<Node<KeyT, ValT>>& q) {
            std::shared_ptr<Node<KeyT, ValT>> p = q->left;
            q->left = p->right;
            if (p->right != nullptr) {
                p->right->parent = q;
            }
            p->right = q;
            FixHeight(p);
            p->parent = q->parent;
            q->parent = p;
            q = p;
            FixHeight(q);
        }

        std::shared_ptr<Node<KeyT, ValT>> MakeCopy(std::weak_ptr<const Node<KeyT, ValT>> root) {
            auto root_ptr = root.lock();
            if (root_ptr == nullptr) {
                return nullptr;
            }
            auto new_node = std::allocate_shared<Node<KeyT, ValT>>(
                    allocator_, root_ptr->key, root_ptr->value
            );
            new_node->left = MakeCopy(root_ptr->left);
            new_node->right = MakeCopy(root_ptr->right);
            if (new_node->right != nullptr) {
                new_node->right->parent = new_node;
            }
            if (new_node->left != nullptr) {
                new_node->left->parent = new_node;
            }
            new_node->height = root_ptr->height;
            return new_node;
        }

        std::shared_ptr<Node<KeyT, ValT>> root_;
        Allocator allocator_;
    };

}

int main() {
    TreeLib::AVLMap<int, int> mymap;
    mymap.Insert(0,0);
    mymap.Insert(1,-1);
    mymap.Insert(2,-101);
    mymap.Insert(3,10);
    mymap.Insert(4,10);
    mymap.Insert(5,30);

    return 0;
}