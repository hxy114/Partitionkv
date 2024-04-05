#ifndef PARTITION_NODE
#define PARTITION_NODE

//partition node节点
//主要用于在分区内索引

#include<iostream>
#include"pmtable.h"
#include"meta_node.h"
class Partition_Node{
private:
    char *node;
public:
    Partition_Node();
    ~Partition_Node();

    //设置该partition_node的key range
    void set_range(uint64_t &startkey,uint64_t &endkey);
    //设置指针，指针为null时，赋值为0
    void set_pmtable_pointer(const PMtable *pointer);
    void set_immupmtable_pointer(const PMtable *pointer);
    void set_otherimmupmtable_pointer(const PMtable *pointer);
    void set_metanode_pointer(const Meta_Node *pointer);

    //将partition_node持久化到Meta node中
    void persistence_storage();
    //获取key range（startkey，endkey）
    std::pair<uint64_t,uint64_t> get_key_range();
    //获取指针，如果各个位都为0,表示该指针是一个空指针
    PMtable* get_pmtable_pointer();
    PMtable* get_immupmtable_pointer();
    PMtable* get_otherimmupmtable_pointer();
    Meta_Node* get_metanode_pointer();
};

#endif