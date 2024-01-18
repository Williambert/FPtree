#include <algorithm>
#include <cassert>
#include <cstdint>
#include <utility>
#include<bits/stdc++.h>
#include <iostream>

#include "fptree.hpp"

// unroll-loops
#pragma GCC optimize("unroll-loops")


void construct_new_fptree(std::shared_ptr<FPNode>& node, FPTree& new_fptree){
    
    // DFS to make new haeader table
    if(node == nullptr){
        return;
    }
    for(auto& child:node->children){
        if(child->part_of_conditional_fptree == new_fptree.root->part_of_conditional_fptree){
            new_fptree.count[child->item] += child->conditional_frequency;
            if(new_fptree.header_table.count(child->item)){
                auto & first_fpnode = new_fptree.header_table[child->item];
                auto fpnode_last = first_fpnode->last.lock();
                auto curr =  std::make_shared<FPNodeWrapper>(child);
                fpnode_last->next = (curr);
                first_fpnode->last = (curr);
            }
            else {
                auto curr = std::make_shared<FPNodeWrapper>(child);
                new_fptree.header_table[child->item] = curr;
                curr->last = std::weak_ptr<FPNodeWrapper>(curr);
            }
            construct_new_fptree(child, new_fptree);
        }
    }
}
FPTree conditional_tree(const FPTree& fptree, const Item& item){

    FPTree new_fptree;
    new_fptree.root = fptree.root;
    new_fptree.minimum_support_threshold = fptree.minimum_support_threshold;

    auto path_starting_fpnode = fptree.header_table.find(item)->second;
    while ( path_starting_fpnode ) {

        auto curr_path_fpnode = path_starting_fpnode->node->parent.lock();
        if(curr_path_fpnode->part_of_conditional_fptree == path_starting_fpnode->node->part_of_conditional_fptree){
            curr_path_fpnode->conditional_frequency = 0;
        }
        curr_path_fpnode->conditional_frequency += path_starting_fpnode->node->conditional_frequency;
        curr_path_fpnode->part_of_conditional_fptree = item;
        while ( curr_path_fpnode->parent.lock()) {
            if(curr_path_fpnode->parent.lock()->part_of_conditional_fptree == path_starting_fpnode->node->part_of_conditional_fptree){
                curr_path_fpnode->parent.lock()->conditional_frequency = 0;
            }
            curr_path_fpnode->parent.lock()->part_of_conditional_fptree = item;
            curr_path_fpnode->parent.lock()->conditional_frequency += path_starting_fpnode->node->conditional_frequency;
            curr_path_fpnode = curr_path_fpnode->parent.lock();
        }
        
        path_starting_fpnode = path_starting_fpnode->next;
    }

    
    construct_new_fptree(new_fptree.root, new_fptree);
    for(auto &it : new_fptree.count){
        if(it.second < new_fptree.minimum_support_threshold && new_fptree.header_table.count(it.first)){
            new_fptree.header_table.erase(it.first);
        }
    }
    return new_fptree;
}
void copy_freq(const std::shared_ptr<FPNode>& node, const FPTree& fptree){
    //DFS from root
    if(node == nullptr || node->part_of_conditional_fptree == 0){
        return;
    }
    for(auto& child:node->children){
        copy_freq(child, fptree);
    }
    node->conditional_frequency = node->frequency;
    node->part_of_conditional_fptree = 0;
}


void recover_tree(const FPTree& fptree, const Item& item, const Item &prev){
    // for each path in the header_table (relative to the current item)
    auto path_starting_fpnode = fptree.header_table.find(item)->second;
    auto z = path_starting_fpnode->node->part_of_conditional_fptree;
    if(!z){
        copy_freq(fptree.root,fptree);
        return;
    }
    // recover_frequencies(fptree.root, fptree,z);
    while ( path_starting_fpnode ) {
        // construct the transformed prefix path

        auto curr_path_fpnode = path_starting_fpnode->node->parent.lock();
        uint64_t bubble_up = 0;
        if(curr_path_fpnode->part_of_conditional_fptree != path_starting_fpnode->node->part_of_conditional_fptree){
            curr_path_fpnode->conditional_frequency = 0;
            curr_path_fpnode->part_of_conditional_fptree = path_starting_fpnode->node->part_of_conditional_fptree;
            for(auto &it : curr_path_fpnode->children){
                if((it->part_of_conditional_fptree == path_starting_fpnode->node->part_of_conditional_fptree)|| (it->item == path_starting_fpnode->node->part_of_conditional_fptree && it->part_of_conditional_fptree == prev))
                    curr_path_fpnode->conditional_frequency += it->conditional_frequency;
            }
            bubble_up = curr_path_fpnode->conditional_frequency;
        }
        else{
            curr_path_fpnode->conditional_frequency += path_starting_fpnode->node->conditional_frequency;
            bubble_up = path_starting_fpnode->node->conditional_frequency;
        }
        // check if curr_path_fpnode is already the root of the fptree
        while ( curr_path_fpnode->parent.lock()) {
            auto parent = curr_path_fpnode->parent.lock();
            if(parent->part_of_conditional_fptree != path_starting_fpnode->node->part_of_conditional_fptree){
                parent->conditional_frequency = 0;
                parent->part_of_conditional_fptree = path_starting_fpnode->node->part_of_conditional_fptree;
                for(auto &it : parent->children){
                    if(((it->part_of_conditional_fptree == path_starting_fpnode->node->part_of_conditional_fptree) || (it->item == path_starting_fpnode->node->part_of_conditional_fptree && it->part_of_conditional_fptree == prev)) && (it->item != item))
                        parent->conditional_frequency += it->conditional_frequency;
                }
                bubble_up = parent->conditional_frequency;
            }
            else{
                parent->conditional_frequency += bubble_up;
            }
            
        
            // advance to the next node in the path
            curr_path_fpnode = parent;
        }
        
        // advance to the next path
        path_starting_fpnode = path_starting_fpnode->next;
    }
    
}


FPNode::FPNode(const Item& item, const std::shared_ptr<FPNode>& parent) :
    item( item ), frequency( 1 ),conditional_frequency(1), parent( parent ), children(),part_of_conditional_fptree(0)
{}

FPTree::FPTree(std::vector<Transaction>& transactions, uint64_t minimum_support_threshold) :
    root( std::make_shared<FPNode>( Item{}, nullptr ) ), header_table(),
    minimum_support_threshold( minimum_support_threshold ), complete_frequency_by_item()
{
    // scan the transactions counting the frequency of each item
    std::map<Item, uint64_t> frequency_by_item;
    for ( const Transaction& transaction : transactions ) {
        for ( const Item& item : transaction) {
            ++frequency_by_item[item];
        }
    }
    complete_frequency_by_item = frequency_by_item;
    // keep only items which have a frequency greater or equal than the minimum support threshold
    for ( auto it = frequency_by_item.cbegin(); it != frequency_by_item.cend(); ) {
        const uint64_t item_frequency = (*it).second;
        if ( item_frequency < minimum_support_threshold ) { frequency_by_item.erase( it++ ); }
        else { ++it; }
    }

    // order items by decreasing frequency
    struct frequency_comparator
    {
        bool operator()(const std::pair<Item, uint64_t> &lhs, const std::pair<Item, uint64_t> &rhs) const
        {
            return std::tie(lhs.second, lhs.first) > std::tie(rhs.second, rhs.first);
        }
    };
    std::set<std::pair<Item, uint64_t>, frequency_comparator> items_ordered_by_frequency(frequency_by_item.cbegin(), frequency_by_item.cend());

    // start tree construction

    // scan the transactions again
    for ( const Transaction& transaction : transactions ) {
        auto curr_fpnode = root;

        // select and sort the frequent items in transaction according to the order of items_ordered_by_frequency
        for ( const auto& pair : items_ordered_by_frequency ) {
            const Item& item = pair.first;

            // check if item is contained in the current transaction
            if ( std::find( transaction.cbegin(), transaction.cend(), item ) != transaction.cend() ) {
                // insert item in the tree
                count[item]++;

                // check if curr_fpnode has a child curr_fpnode_child such that curr_fpnode_child.item = item
                const auto it = std::find_if(
                    curr_fpnode->children.cbegin(), curr_fpnode->children.cend(),  [item](const std::shared_ptr<FPNode>& fpnode) {
                        return fpnode->item == item;
                } );
                if ( it == curr_fpnode->children.cend() ) {
                    // the child doesn't exist, create a new node
                    auto curr_fpnode_new_child = std::make_shared<FPNode>( item, curr_fpnode );

                    // add the new node to the tree
                    curr_fpnode->children.emplace_back( curr_fpnode_new_child );

                    // update the node-link structure
                    if ( header_table.count( curr_fpnode_new_child->item ) ) {
                        auto & first_fpnode = header_table[curr_fpnode_new_child->item];
                        auto fpnode_last = first_fpnode->last.lock();
                        auto curr =  std::make_shared<FPNodeWrapper>(curr_fpnode_new_child);
                        fpnode_last->next = (curr);
                        first_fpnode->last = std::weak_ptr<FPNodeWrapper>(curr);
                    }
                    else {
                        auto curr = std::make_shared<FPNodeWrapper>(curr_fpnode_new_child);
                        header_table[curr_fpnode_new_child->item] = curr;
                        curr->last = (curr);
                    }

                    // advance to the next node of the current transaction
                    curr_fpnode = curr_fpnode_new_child;
                }
                else {
                    // the child exist, increment its frequency
                    auto curr_fpnode_child = *it;
                    ++curr_fpnode_child->frequency;
                    ++curr_fpnode_child->conditional_frequency;
                    
                    // advance to the next node of the current transaction
                    curr_fpnode = curr_fpnode_child;
                }
            }
        }
    }
}

bool FPTree::empty() const
{
    return header_table.size() == 0;
}


bool contains_single_path(const std::shared_ptr<FPNode>& fpnode)
{
    if(fpnode == nullptr){
        return true;
    }
    if ( fpnode->children.size() == 0 ) { return true; }
    std::shared_ptr<FPNode> child = nullptr;
    for(auto &it : fpnode->children){
        if(it->part_of_conditional_fptree == fpnode->part_of_conditional_fptree){
            if ( child ) { return false; }
            child = it;
        }
    }
    return contains_single_path( child );
}

bool contains_single_path(const FPTree& fptree)
{
    return fptree.empty() || contains_single_path( fptree.root );
}

std::set<Pattern> fptree_growth(const FPTree& fptree, uint64_t prev_cond)
{
    if ( fptree.empty() ) { return {}; }

    if ( contains_single_path( fptree ) ) {
        // generate all possible combinations of the items in the tree
        std::set<Pattern> single_path_patterns;

        // for each node in the tree
        std::shared_ptr<FPNode> curr_fpnode = nullptr;
        for(auto &it : fptree.root->children){
            if(it->part_of_conditional_fptree == fptree.root->part_of_conditional_fptree){
                curr_fpnode = it;
                break;
            }
        }
        while ( curr_fpnode ) {
            const Item& curr_fpnode_item = curr_fpnode->item;
            const uint64_t curr_fpnode_frequency = curr_fpnode->conditional_frequency;
            if(curr_fpnode_frequency < fptree.minimum_support_threshold){
                curr_fpnode = nullptr;
                continue;
            }
            // add a pattern formed only by the item of the current node
            Pattern new_pattern{ { curr_fpnode_item }, curr_fpnode_frequency };
            single_path_patterns.insert( new_pattern );

            // create a new pattern by adding the item of the current node to each pattern generated until now
            for ( const Pattern& pattern : single_path_patterns ) {
                Pattern new_pattern{ pattern };
                new_pattern.first.insert( curr_fpnode_item );
                new_pattern.second = curr_fpnode_frequency;

                single_path_patterns.insert( new_pattern );
            }

            // advance to the next node until the end of the tree
            bool is_leaf = true;
            for(auto &it : curr_fpnode->children){
                if(it->part_of_conditional_fptree == fptree.root->part_of_conditional_fptree){
                    is_leaf = false;
                    curr_fpnode = it;
                    break;
                }
            }
            if(is_leaf){
                curr_fpnode = nullptr;
            }
        }

        return single_path_patterns;
    }
    else {

        std::set<Pattern> multi_path_patterns;

        for ( const auto& pair : fptree.header_table ) {
            const Item& curr_item = pair.first;

            auto cond = fptree.root->part_of_conditional_fptree;

            FPTree conditional_fptree = conditional_tree(fptree, curr_item);
            std::set<Pattern> conditional_patterns = fptree_growth( conditional_fptree, cond );


            recover_tree(fptree, curr_item,prev_cond);

            std::set<Pattern> curr_item_patterns;
            uint64_t curr_item_frequency = fptree.count.find( curr_item )->second;
            Pattern pattern{ { curr_item }, curr_item_frequency };
            curr_item_patterns.insert( pattern );
            for ( const Pattern& pattern : conditional_patterns ) {
                Pattern new_pattern{ pattern };
                new_pattern.first.insert( curr_item );
                new_pattern.second = pattern.second;

                curr_item_patterns.insert( { new_pattern } );
            }

            multi_path_patterns.insert( curr_item_patterns.cbegin(), curr_item_patterns.cend() );
        }

        return multi_path_patterns;
    }
}
