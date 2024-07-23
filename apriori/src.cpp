#include <bits/stdc++.h>
#define MAX_ITEM 16470    // 商品总数
#define MAX_LINE 88162    // 交易总数
using namespace std;

vector<vector<int>> Transactions;         // 用于存储retail.dat
int min_support;                          // 最小支持度
int L_sum = 0;                            // L 的总数
vector<map<vector<int>, int>> final_L;    // 所有频繁项集

// ------------------ preparation -----------------------
// 读取 retail.dat
void getRetail() {
    ifstream file("../retail.dat");
    string line;
    while (getline(file, line)) {
        istringstream iss(line);
        vector<int> trans;
        string ele;
        while (iss >> ele) {
            int num = stoi(ele);
            trans.push_back(num);
        }
        Transactions.push_back(trans);
    }
}
// get min support
void getMinSupport() {
    cout << "以百分比的形式输入最小支持度: ";
    double min_support_percentage;
    cin >> min_support_percentage;
    min_support = int(ceil((min_support_percentage * 1.0 / 100.0) * MAX_LINE));
}

// ----------------- core function ------------------------
bool generateCfromL(map<vector<int>, int>& C, map<vector<int>, int>& L) {
    C.clear();
    // 遍历 L，查询符合条件的组合
    for (auto it1 = L.begin(); it1 != L.end(); it1++) {
        auto it2 = it1;
        for (++it2; it2 != L.end(); it2++) {
            // 判断除最后一个元素外是否相同
            if (std::equal(it1->first.begin(), it1->first.end() - 1,
                           it2->first.begin())) {
                // step1：自连接
                vector<int> new_itemSet = it1->first;
                new_itemSet.push_back(it2->first.back());
                // step2：修剪, 检查new_itemset的(k-1)-subsets是否都存在于 Lk 中
                bool flag = true;
                for (int i = 0; i < new_itemSet.size(); i++) {
                    vector<int> subset = new_itemSet;
                    subset.erase(subset.begin() + i);
                    if (L.find(subset) == L.end()) {
                        flag = false;
                        break;
                    }
                }
                if (flag) {
                    C[new_itemSet] = 0;
                }
            }
        }
    }
    return !C.empty();
}

bool isSubset(const vector<int>& item, const vector<int>& transaction) {
    size_t i = 0, j = 0;
    while (i < item.size() && j < transaction.size()) {
        if (item[i] < transaction[j])
            return false;
        else if (item[i] == transaction[j])
            i++;
        j++;
    }
    return i == item.size();
}

bool generateLfromC(map<vector<int>, int>& C, map<vector<int>, int>& L) {
    // 计算 Ck 所有 key 的 value
    L.clear();
    for (auto& trans : Transactions) {
        for (auto& item : C) {
            if (isSubset(item.first, trans)) {
                item.second++;
            }
        }
    }
    // 从 Ck 中得到 Lk
    for (auto& item : C) {
        if (item.second >= min_support) {
            L[item.first] = item.second;
        }
    }
    return !L.empty();
}

// 根据 L，对 retail 里的数据进行优化，除去不在 L 中的元素(只用 L1 进行优化)
void optimizeRetail(map<vector<int>, int>& L) {
    unordered_set<int> L_set;
    for (auto& item : L) {
        for (auto& i : item.first) {
            L_set.insert(i);
        }
    }
    vector<vector<int>> optimized_transactions;
    for (auto& tran : Transactions) {
        vector<int> temp;
        for (auto& ele : tran) {
            if (L_set.find(ele) != L_set.end()) {
                temp.push_back(ele);
            }
        }
        if (!temp.empty())
            optimized_transactions.push_back(temp);
    }
    Transactions = optimized_transactions;
}

// ----------------- 最小置信度计算，强规则生成 ------------------------
// vector<map<vector<int>, int>> final_L;
void generate_strong_rule(double min_conf) {
    int count = 0;
    for (int i = 2; i < final_L.size(); i++) {
        for (auto& item : final_L[i]) {
            vector<int> set = item.first;
            int set_len = set.size();
            int totalSubsets = 1 << set_len;
            for (int j = 1; j < totalSubsets - 1; j++) {
                // get subset
                vector<int> subset;
                for (int k = 0; k < set_len; k++) {
                    if (j & (1 << k)) {
                        subset.push_back(set[k]);
                    }
                }
                // 计算 conf
                double conf =
                    (item.second * 1.0) / final_L[subset.size()].at(subset);
                // 判断并输出
                if (conf >= min_conf) {
                    count++;
                    cout << "{";
                    for (auto& element : subset) {
                        cout << element << " ";
                    }
                    cout << "} -> {";
                    for (auto& element : set) {
                        if (std::find(subset.begin(), subset.end(), element) ==
                            subset.end()) {
                            cout << element << " ";
                        }
                    }
                    cout << "}, confidence is: " << conf << endl;
                }
            }
        }
    }
    cout << "强规则总数:" << count << endl;
}

// ----------------- main function ------------------------
int main() {
    getMinSupport();
    getRetail();
    final_L.resize(1);    // final_L 从 1 开始计数

    auto start = std::chrono::high_resolution_clock::now();    // 开始计时
    // 生成 C1 和 L1
    map<vector<int>, int> C1;
    map<vector<int>, int> L1;
    for (auto& trans : Transactions) {
        for (int ele : trans) {
            C1[{ele}]++;
        }
    }
    for (auto& item : C1) {
        if (item.second >= min_support) {
            L1[item.first] = item.second;
        }
    }
    final_L.push_back(L1);
    optimizeRetail(L1);

    // 生成 C2 和 L2
    map<vector<int>, int> C2;
    map<vector<int>, int> L2;
    for (auto& trans : Transactions) {
        for (int i = 0; i < trans.size(); i++) {
            for (int j = i + 1; j < trans.size(); j++) {
                vector<int> temp;
                temp.push_back(trans[i]);
                temp.push_back(trans[j]);
                C2[temp]++;
            }
        }
    }
    for (auto& item : C2) {
        if (item.second >= min_support) {
            L2[item.first] = item.second;
        }
    }
    final_L.push_back(L2);

    // apriori 算法迭代寻找 Ln
    map<vector<int>, int> Ck;
    map<vector<int>, int> Lk;
    Ck = C2;
    Lk = L2;
    while (true) {
        if (!generateCfromL(Ck, Lk)) {
            break;
        }
        if (!generateLfromC(Ck, Lk)) {
            break;
        }
        final_L.push_back(Lk);
    }
    auto end = std::chrono::high_resolution_clock::now();    // 结束计时

    // 输出结果
    for (int i = 1; i < final_L.size(); i++) {
        cout << "L" << i << " size is:" << final_L[i].size() << endl;
        L_sum += final_L[i].size();
    }
    cout << "Frequent itemset: " << L_sum << " in total" << endl;
    auto duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(end - start)
            .count();
    cout << "Time taken: " << duration << " ms" << endl;

    // 生成强规则
    cout << "------------------------------------------" << endl;
    cout << "所有频繁模式集已生成，输入字符 C 或者 c 继续生成强规则: " << endl;
    char next_input;
    cin >> next_input;
    if (next_input == 'c' || next_input == 'C') {
        double min_conf;
        cout << "以百分比的形式输入最小置信度: ";
        cin >> min_conf;
        min_conf = min_conf / 100;
        generate_strong_rule(min_conf);
    }
    cout << "-----------------程序结束------------------" << endl;
    return 0;
}
