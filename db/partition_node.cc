#include"partition_node.h"
#include<iostream>
#include <string.h>
#include <cassert>
Partition_Node::Partition_Node(){
    this->node=new char[48];
}

Partition_Node::~Partition_Node(){
    delete []node;
}

//小端存储，数据的低位存在低位上
uint64_t Partition_Node::get_key_range_max(){
    uint8_t *endkey=reinterpret_cast<uint8_t*>(node[8]);
    return endkey[0]|(endkey[1]<<8)|(endkey[2]<<16)|(endkey[3]<<24)
        |(endkey[4]<<32)|(endkey[5]<<40)|(endkey[6]<<48)|(endkey[7]<<56);
}

uint64_t Partition_Node::get_key_range_min(){
    uint8_t *startkey=reinterpret_cast<uint8_t*>(node);
    return startkey[0]|(startkey[1]<<8)|(startkey[2]<<16)|(startkey[3]<<24)
        |(startkey[4]<<32)|(startkey[5]<<40)|(startkey[6]<<48)|(startkey[7]<<56);
}

void Partition_Node::set_range(uint64_t &startkey,uint64_t &endkey){
    assert(endkey>=startkey);
    //小端存储
    uint8_t *key=reinterpret_cast<uint8_t*>(&startkey);
    memcpy(node,key,sizeof(uint64_t));
    key=reinterpret_cast<uint8_t*>(&endkey);
    memcpy(node+8,key,sizeof(uint64_t));
}