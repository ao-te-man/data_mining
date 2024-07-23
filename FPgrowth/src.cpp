#include <bits/stdc++.h>
#define MAX_ITEM 16470    // 商品总数
#define MAX_LINE 88162    // 交易总数
using namespace std;

vector<vector<int>> Transactions;         // 用于存储retail.dat
int min_support = 0;                      // 最小支持度
int L_sum = 0;                            // L 的总数
vector<map<vector<int>, int>> final_L;    // 所有频繁项集

// --------------- preparation --------------
// 读取 retail.dat
void getRetail() {
    ifstream file("../retail.dat");
    string line;
    while (getline(file, line)) {
        istringstream iss(line);
        string ele;
        vector<int> trans;
        while (iss >> ele) {
            trans.push_back(stoi(ele));
        }
        Transactions.push_back(trans);
    }
}

void getMinSupport() {
    cout << "以百分比的形式输入最小支持度: ";
    double min_support_percentage;
    cin >> min_support_percentage;
    min_support = int(ceil((min_support_percentage * 1.0 / 100.0) * MAX_LINE));
}

// 计算频繁 1 项集
void getL1(map<int, int>& L1) {
    map<int, int> C1;
    for (auto& trans : Transactions) {
        for (auto& ele : trans) {
            C1[ele]++;
        }
    }
    for (auto& item : C1) {
        if (item.second >= min_support) {
            L1[item.first] = item.second;
        }
    }
}

// 把最初的 Transaction 转化成 map<vector<int>, int> 形式
map<vector<int>, int> convertTrans2map(
    const vector<vector<int>>& Transactions) {
    map<vector<int>, int> optimizedTrans;
    for (auto& trans : Transactions) {
        vector<int> temp_trans;
        for (auto& ele : trans) {
            temp_trans.push_back(ele);
        }
        optimizedTrans[temp_trans]++;
    }
    return optimizedTrans;
}

// --------------- core function --------------
class Node {
   public:
    int item;
    int count;
    shared_ptr<Node> father;
    vector<shared_ptr<Node>> children;

    Node() {
        item = -1;
        count = -1;
        father = nullptr;
        children.clear();
    }
};

class FPTree {
   public:
    shared_ptr<Node> root;
    unordered_map<int, vector<shared_ptr<Node>>> header_table;
    vector<pair<int, int>>
        sorted_items;    // 用于后续排序，为<商品序号，出现次数> 的 pair

    FPTree() { root = make_shared<Node>(); }
    // 判断该树是否为 single path
    bool singlePath() {
        shared_ptr<Node> cur = root;
        while (true) {
            if (cur->children.size() > 1)
                return false;
            else if (cur->children.size() == 1)
                cur = cur->children[0];
            else
                return true;
        }
    }
    // 根据 L 初始化 sorted_items
    // vector<pair<int, int>> sorted_items;
    void constructHeaderTable(const map<int, int>& L) {
        // 初始化 header table
        for (auto& item : L) {
            header_table[item.first].clear();
        }
        // 构造 sorted_items
        sorted_items.reserve(L.size());
        for (const auto& item : L) {
            sorted_items.emplace_back(item.first, item.second);
        }
        sort(sorted_items.begin(), sorted_items.end(),
             [](const pair<int, int>& a, const pair<int, int>& b) {
                 return a.second < b.second;
             });
    }
    // 根据（条件）模式集构建fp_tree
    void constructTree(map<vector<int>, int> Transactions) {
        for (auto& trans : Transactions) {
            shared_ptr<Node> cur = root;
            for (auto& item : trans.first) {
                shared_ptr<Node> next = nullptr;
                for (auto& child : cur->children) {
                    if (child->item == item) {
                        next = child;
                        break;
                    }
                }
                if (next == nullptr) {
                    next = make_shared<Node>();
                    next->item = item;
                    next->father = cur;
                    next->count = trans.second;
                    cur->children.push_back(next);
                    header_table[item].push_back(next);
                } else {
                    next->count += trans.second;
                }
                cur = next;
            }
        }
    }

    // 对于 single path 的情况，生成所有的 combinations
    void generateAllCombinations(vector<int> alpha) {
        vector<int> path;
        shared_ptr<Node> cur = root;
        while (cur->children.size() != 0) {
            cur = cur->children[0];
            path.push_back(cur->item);
        }
        int totalCombinations = (1 << path.size()) - 1;
        // generate pattern 'path U alpha'
        for (int bitmask = 1; bitmask <= totalCombinations; bitmask++) {
            // 生成 path 所有可能
            vector<int> subset;
            for (size_t i = 0; i < path.size(); i++) {
                if (bitmask & (1 << i))
                    subset.push_back(path[i]);
            }
            // 将 alpha 元素添加进 path
            if (!alpha.empty()) {
                for (auto& ele : alpha) {
                    if (std::find(subset.begin(), subset.end(), ele) ==
                        subset.end())
                        subset.push_back(ele);
                }
            }
            if (final_L.size() < subset.size() + 1)
                final_L.resize(subset.size() + 1);
            final_L[subset.size()][subset] = 0;    // calculate support
        }
    }
};

// 对于当前的conditional pattern base，找出频繁项
void findL(const map<vector<int>, int>& sub_trans, map<int, int>& L) {
    map<int, int> C;
    // sub_trans:{vector<int>path,min_sup}
    for (auto& trans : sub_trans) {
        for (auto& ele : trans.first) {
            C[ele] += trans.second;    // 根据 map 特性，判断存在可以省略
        }
    }
    for (auto& item : C) {
        if (item.second >= min_support) {
            L[item.first] = item.second;
        }
    }
}

// 优化 trans，并按照出现次数从高到低排序
map<vector<int>, int> optimizeTrans(const map<vector<int>, int>& sub_trans,
                                    const map<int, int>& L) {
    map<vector<int>, int> optimized_trans;
    for (auto& trans : sub_trans) {
        vector<int> new_trans;
        for (auto& ele : trans.first) {
            if (L.find(ele) != L.end()) {
                new_trans.push_back(ele);
            }
        }
        // if not empty, insert it to the map
        if (!new_trans.empty()) {
            sort(new_trans.begin(), new_trans.end(), [&](int a, int b) {
                if (L.at(a) == L.at(b))
                    return a < b;
                return L.at(a) > L.at(b);
            });
        }
        optimized_trans[new_trans] += trans.second;
    }
    return optimized_trans;
}

void FPgrowth(FPTree& tree, vector<int> alpha = {}) {
    if (tree.singlePath()) {
        tree.generateAllCombinations(alpha);
        return;
    } else {
        for (auto& item : tree.sorted_items) {
            // generate pattern beta = ai U alpha
            int ai = item.first;
            vector<int> beta = alpha;
            beta.push_back(ai);
            if (final_L.size() < beta.size() + 1)
                final_L.resize(beta.size() + 1);
            final_L[beta.size()].insert({beta, 0});

            map<vector<int>, int>
                sub_trans;    // remember to clear after optimizeTrans()
            // 构造当前元素的条件模式集
            for (auto& node : tree.header_table[item.first]) {
                vector<int> path;
                shared_ptr<Node> cur = node->father;
                while (cur->item != -1) {
                    path.push_back(cur->item);
                    cur = cur->father;
                }
                // reverse(path.begin(), path.end());
                if (!path.empty())
                    sub_trans.insert({path, node->count});
            }
            // if sub_trans is empty, continue
            if (sub_trans.empty())
                continue;

            // 找出频繁项 L
            map<int, int> L;
            findL(sub_trans, L);
            // 优化 sub_trans, 排除 L 中不存在的元素，根据出现的频率 sort
            map<vector<int>, int> optimized_trans = optimizeTrans(sub_trans, L);
            sub_trans.clear();
            if (optimized_trans.empty())
                continue;
            // 根据 L 和 optimized_trans 构建 FPtree
            FPTree subtree;
            subtree.constructHeaderTable(L);
            subtree.constructTree(optimized_trans);
            FPgrowth(subtree, beta);
        }
    }
}

// --------------- main function --------------
int main() {
    getMinSupport();
    getRetail();

    map<int, int> L1;
    getL1(L1);    // 频繁1-项集
    // 优化 Transactions
    map<vector<int>, int> optimizedTrans;
    optimizedTrans = optimizeTrans(convertTrans2map(Transactions), L1);
    Transactions.clear();

    auto start = std::chrono::high_resolution_clock::now();    // 开始计时
    FPTree tree;
    tree.constructHeaderTable(L1);
    tree.constructTree(optimizedTrans);
    // 根据 FPtree，进行 FPgrowth 函数
    FPgrowth(tree);
    auto end = std::chrono::high_resolution_clock::now();    // 结束计时
    cout << "L1 size is: " << L1.size() << endl;
    L_sum += L1.size();
    for (size_t i = 2; i < final_L.size(); i++) {
        L_sum += final_L[i].size();
        cout << "L" << i << " size is: " << final_L[i].size() << endl;
    }
    cout << "Mining " << L_sum << " Frequent itemsets" << endl;
    cout << "Time taken: "
         << chrono::duration_cast<chrono::milliseconds>(end - start).count()
         << "ms" << endl;

    return 0;
}
