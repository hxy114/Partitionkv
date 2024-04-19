//
// Created by hxy on 24-3-29.
//

#ifndef LEVELDB_PM_MODULE_H
#define LEVELDB_PM_MODULE_H
#include <cstdint>
#include <mutex>
#include <libpmem.h>
#include <map>
#include <vector>
#include "port/port.h"


namespace leveldb {
extern const size_t PM_SIZE;
extern const size_t PM_META_NODE_SIZE;//pmlog 大小
extern const size_t PM_LOG_HEAD_SIZE;//pm log大小
extern const size_t PM_LOG_SIZE;
extern const size_t PERSIST_SIZE;//非强制刷写大小
extern const size_t PM_META_NODE_NUMBER;//TODO
extern const size_t PM_LOG_NUMBER;//TODO
extern const char * PM_FILE_NAME;
extern const uint32_t META_NODE_MAGIC;
extern const uint32_t PM_LOG_MAGIC;
extern const uint32_t INVALID;

extern const std::string MIN_KEY;
extern const std::string MAX_KEY;


extern const uint64_t PRE_SPLIT;//TODO
extern const uint64_t SPLIT;//TODO

extern const uint64_t PRE_MERGE;//TODO
extern const uint64_t MERGE;//TODO

extern const uint64_t K;//TODO 记录K次覆盖记录
extern const uint64_t PRE_SPLIT_NUMBER;//TODO
extern const uint64_t PRE_MERGE_NUMBER;//TODO


extern const uint64_t MIN_PARTITION;
extern const uint64_t MAX_PARTITION;


extern const bool IS_FLUSH;

extern const uint64_t L0_THREAD_NUMBER;
//extern int  extra_pm_log;
//extern const int extra_pm_log_const;
typedef struct MetaNode{//size=64B
  uint32_t magic_number;
  uint16_t start_key_size;
  char start_key[16];
  uint16_t end_key_size;
  char end_key[16];
  uint64_t pm_log;
  uint64_t immu_pm_log;
  uint64_t other_immu_pm_log;
}MetaNode;
typedef  struct PmLogHead{//size=128B
  uint32_t magic_number;
  uint64_t file_size;
  uint64_t used_size;
  uint16_t start_key_size;
  char start_key[16];
  uint16_t end_key_size;
  char end_key[16];
  uint64_t  next;
  char padding[56];
}PmLogHead;
class NvmManager {
 public:
  NvmManager (bool is_recover_);
  ~NvmManager();
  MetaNode * get_meta_node();
  PmLogHead * get_pm_log();
  void free_meta_node(MetaNode * meta_node);
  void free_pm_log(PmLogHead * pm_log);
  std::vector<MetaNode *>&& get_recover_meta_nodes_();
  char *get_base();
  size_t get_free_pm_log_number();
  size_t get_free_meta_node_number();
 private:
  size_t pm_size_;//pm大小

  port::Mutex mutex_;
  char* base_;
  char *pm_log_base_;


  std::vector<MetaNode *>free_meta_node_list_;
  std::vector<PmLogHead *>free_pm_log_list_;
  std::vector<MetaNode *>recover_meta_nodes_;

};
void reset(MetaNode *meta_node);
void reset(PmLogHead *pm_log_head);
void set(MetaNode *meta_node);
void set(PmLogHead *pm_log_head);

extern NvmManager *nvmManager;
}
#endif  // LEVELDB_PM_MODULE_H
