#ifndef PARTITION_NODE
#define PARTITION_NODE
#include <cstdint>
#include <vector>
#include "pmtable.h"
#include "util/nvm_module.h"
//partition node节点
//主要用于在分区内索引

namespace leveldb{
class VersionSet;
class DBImpl;
class PartitionNode{
 public:
  enum Status{
    sucess,
    split,
    merge,
  };
  char *base_;
  std::string start_key;
  std::string end_key;
  PmTable *pmtable;
  //bool set_high_queue_;//把immupmtable放到高优先级队列
  PmTable *immuPmtable;
  //std::atomic<bool> has_immuPmtable_;
  PmTable *other_immuPmtable;
  //std::atomic<bool> has_otherimmuPmtable_;
  MetaNode *metaNode;
  int refs_;
  port::Mutex &mutex_;
  VersionSet *const versions_ GUARDED_BY(mutex_);
  InternalKeyComparator internal_comparator_;

  std::vector<uint64_t>cover_;
  size_t index_=0;

  port::CondVar &background_work_finished_signal_L0_;
  PmtableQueue &high_queue_;
  PmtableQueue &low_queue_;
  DBImpl *dbImpl_;
 public:
  Status needSplitOrMerge();
  void init(const std::string &startkey,const std::string &endkey);
  void FLush();
  PartitionNode(const std::string &start_key,
                const std::string &end_key,
                MetaNode *metaNode1,VersionSet *versions,
                port::Mutex &mutex,
                port::CondVar &background_work_finished_signal,
                InternalKeyComparator &internal_comparator,
                PmtableQueue &high_queue,
                PmtableQueue &low_queue,
                DBImpl *dbImpl);
  ~PartitionNode();
  void FreePartitionNode();//更改metanode头，并且持久化
  //设置该partition_node的key range
  void set_range( std::string &startkey,std::string &endkey);
  //设置指针
  void set_other_immupmtable(PmTable *otherImmuPmtable);
  void set_immuPmtable(PmTable *immuPmtable);
  void set_pmtable(PmTable *pmTable);
  void reset_other_immupmtable();
  void reset_immuPmtable();
  void reset_pmtable();
  bool has_other_immupmtable();
  //将partition_node持久化到Meta node中
  void persistence_storage();
  //返回end key
  std::string &get_end_key();
  //返回start key
  std::string &get_start_key();

  void Ref() { ++refs_; }

  // Drop reference count.  Delete if no more references exist.
  void Unref() {
    --refs_;
    assert(refs_ >= 0);
    if (refs_ <= 0) {
      delete this;
    }
  }
   Status Add(SequenceNumber s, ValueType type, const Slice& key,
           const Slice& value,bool is_force);//当is_force=true时候，不进行split和merge
};
}


#endif