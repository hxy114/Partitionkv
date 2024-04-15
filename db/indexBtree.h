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

namespace leveldb{
class VersionSet;
class Version;
class DBImpl;
class PartitionIndexLayer{
 private:

  //存储分区 key range 的最大值，指针指向partitionnode
  tlx::btree_map<std::string,PartitionNode*> *bmap GUARDED_BY(mutex_);
  port::Mutex &mutex_;
  VersionSet *const versions_ GUARDED_BY(mutex_);
  InternalKeyComparator internal_comparator_;
  port::CondVar &background_work_finished_signal_L0_;
  PmtableQueue &top_queue_;
  PmtableQueue &high_queue_;
  PmtableQueue &low_queue_;
  uint64_t  capacity_;
  DBImpl *dbImpl_;

  PartitionNode::Status merge(PartitionNode *partitionNode) ;
  PartitionNode::Status split(PartitionNode *partitionNode);
  PartitionNode::Status init_split(PartitionNode *partitionNode);
  PartitionNode * getAceeptNode(Version *current,PartitionNode *partitionNode);
 public:

  PartitionIndexLayer(VersionSet *const versions,
                      port::Mutex &mutex,
                      port::CondVar &background_work_finished_signal_L0_,
                      const InternalKeyComparator &internal_comparator,
                      PmtableQueue &top_queue,
                      PmtableQueue &high_queue,
                      PmtableQueue &low_queue,
                      DBImpl *dbImpl);
  ~PartitionIndexLayer();
  void Add(SequenceNumber s, ValueType type, const Slice& key,
                                  const Slice& value);
  //根据key的值查找，当前KV应该写在哪个分区内
  PartitionNode* seek_partition(const std::string &key);
  //添加指向新的分区的索引
  bool add_new_partition(PartitionNode *partition_node);
  //移除指向该分区的索引
  bool remove_partition(PartitionNode *partition_node);
  bool remove_partition_by_key(std::string &key);
  void init();

  void recover();
  tlx::btree_map<std::string,PartitionNode*> * get_bmap();
};

}




#endif