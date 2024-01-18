#ifndef FPTREE_HPP
#define FPTREE_HPP

#include <cstdint>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>
#include <utility>


using Item = int;
using Transaction = std::vector<Item>;
using TransformedPrefixPath = std::pair<std::vector<Item>, uint64_t>;
using Pattern = std::pair<std::set<Item>, uint64_t>;


struct FPNodeWrapper;

struct FPNode {
    const Item item;
    uint64_t frequency;
    uint64_t conditional_frequency;
    uint64_t part_of_conditional_fptree;
    std::weak_ptr<FPNode> parent;
    std::vector<std::shared_ptr<FPNode>> children;

    FPNode(const Item&, const std::shared_ptr<FPNode>&);
};
struct FPNodeWrapper {
    std::shared_ptr<FPNode> node;
    std::shared_ptr<FPNodeWrapper> next;
    std::weak_ptr<FPNodeWrapper> last;

    FPNodeWrapper(const std::shared_ptr<FPNode>& node):next(nullptr){
        this->node = node;
    }
    FPNodeWrapper(): node(nullptr){}
};
struct FPTree {
    std::shared_ptr<FPNode> root;
    std::map<Item, std::shared_ptr<FPNodeWrapper>> header_table;
    std::map<uint64_t,uint64_t> count;
    uint64_t minimum_support_threshold;
    std::map<Item, uint64_t> complete_frequency_by_item;
    FPTree(){}
    FPTree(std::vector<Transaction>&, uint64_t);

    bool empty() const;
};


std::set<Pattern> fptree_growth(const FPTree&, uint64_t = 0);


#endif  // FPTREE_HPP
