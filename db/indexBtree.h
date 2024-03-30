// @Misc{TLX,
//   title = 	 {{TLX}: Collection of Sophisticated {C++} Data Structures, Algorithms, and Miscellaneous Helpers},
//   author = 	 {Timo Bingmann},
//   year = 	 2018,
//   note = 	 {\url{https://panthema.net/tlx}, retrieved {Oct.} 7, 2020},
// }

//在索引层使用基于B+树实现的map作为索引

#ifndef INDEXBTREE
#define INDEXBTREE

#include<iostream>
#include<tlx/container/btree_map.hpp>
#include"partition_node.h"


class Partition_index_layer{
private:
    //存储分区 key range 的最大值，指针指向partitionnode
    tlx::btree_map<uint64_t,Partition_Node*> *bmap;

public:
    Partition_index_layer();
    ~Partition_index_layer();
    //根据key的值查找，当前KV应该写在哪个分区内
    Partition_Node* seek_partition(const uint64_t &key);
    //添加指向新的分区的索引
    bool add_new_partition(Partition_Node *partition_node);
    //移除指向该分区的索引
    bool remove_partition(Partition_Node *partition_node);
};


#endif