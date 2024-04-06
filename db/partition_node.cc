#include"partition_node.h"
#include<iostream>
#include <string.h>
#include <cassert>
Partition_Node::Partition_Node(leveldb::Arena *arena):arena(arena),node(arena->AllocateAligned(48)){
    //将node内的数据全部置为0
    memset(node,0,48);
}

std::pair<uint64_t,uint64_t> Partition_Node::get_key_range(){
    uint8_t *startkey=reinterpret_cast<uint8_t*>(node);
    uint64_t min=startkey[0]|(startkey[1]<<8)|(startkey[2]<<16)|(startkey[3]<<24)
                |(startkey[4]<<32)|(startkey[5]<<40)|(startkey[6]<<48)|(startkey[7]<<56);
    uint8_t *endkey=reinterpret_cast<uint8_t*>(node+8);
    uint64_t max=endkey[0]|(endkey[1]<<8)|(endkey[2]<<16)|(endkey[3]<<24)
                |(endkey[4]<<32)|(endkey[5]<<40)|(endkey[6]<<48)|(endkey[7]<<56);
    return std::make_pair(min,max);
}


//小端存储
void Partition_Node::set_range(uint64_t &startkey,uint64_t &endkey){
    assert(endkey>=startkey);
    uint8_t *key=reinterpret_cast<uint8_t*>(&startkey);
    memcpy(node,key,sizeof(uint64_t));
    key=reinterpret_cast<uint8_t*>(&endkey);
    memcpy(node+8,key,sizeof(uint64_t));
}

void Partition_Node::set_pmtable_pointer(const PMtable *pointer){
    if(pointer==nullptr) memset(node+16,0,8);
    else memcpy(node+16,pointer,8);
}

void Partition_Node::set_immupmtable_pointer(const PMtable *pointer){
    if(pointer==nullptr) memset(node+24,0,8);
    else memcpy(node+24,pointer,8);
}

void Partition_Node::set_otherimmupmtable_pointer(const PMtable *pointer){
    if(pointer==nullptr) memset(node+32,0,8);
    else memcpy(node+32,pointer,8);
}

void Partition_Node::set_metanode_pointer(const Meta_Node *pointer){
    if(pointer==nullptr) memset(node+40,0,8);
    else memcpy(node+40,pointer,8);
}

PMtable* Partition_Node::get_pmtable_pointer(){
    uint8_t value;
    memcpy(&value,node+16,8);
    return value==0?nullptr:reinterpret_cast<PMtable*>(node+16);
}
PMtable* Partition_Node::get_immupmtable_pointer(){
    uint8_t value;
    memcpy(&value,node+24,8);
    return value==0?nullptr:reinterpret_cast<PMtable*>(node+24);
}
PMtable* Partition_Node::get_otherimmupmtable_pointer(){
    uint8_t value;
    memcpy(&value,node+32,8);
    return value==0?nullptr:reinterpret_cast<PMtable*>(node+32);
}
Meta_Node* Partition_Node::get_metanode_pointer(){
    uint8_t value;
    memcpy(&value,node+40,8);
    return value==0?nullptr:reinterpret_cast<Meta_Node*>(node+40);
}