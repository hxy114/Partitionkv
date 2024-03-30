#include"indexBtree.h"

Partition_index_layer::Partition_index_layer(){
    this->bmap=new tlx::btree_map<uint64_t,Partition_Node*>();
}

Partition_index_layer::~Partition_index_layer(){
    delete bmap;
}

Partition_Node* Partition_index_layer::seek_partition(const uint64_t &key){
    auto find=bmap->lower_bound(key);
    return find!=bmap->end()?find->second:nullptr;
}

bool Partition_index_layer::add_new_partition(Partition_Node *partition_node){
    assert(partition_node!=nullptr);
    return (bmap->insert2(partition_node->get_key_range_max(),partition_node)).second;
}

//根据该分区的key range的最大值进行检索，删除该叶子节点
bool Partition_index_layer::remove_partition(Partition_Node *partition_node){
    assert(partition_node!=nullptr);
    auto it=bmap->find(partition_node->get_key_range_max());
    if(it!=bmap->end()) bmap->erase(it);
    return true;
}