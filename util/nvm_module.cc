//
// Created by hxy on 24-3-29.
//
#include "nvm_module.h"
namespace leveldb {
NvmManager *nvmManager= nullptr;


const size_t PM_SIZE=8*1024*1024*1024UL+200*1024*1024UL;
//const size_t PM_SIZE=14*1024*1024*1024UL;
const size_t PM_META_NODE_SIZE=64;//pmlog 大小
const size_t PM_LOG_HEAD_SIZE=128;//pm log大小
const size_t PM_LOG_SIZE=64*1024*1024UL;
const size_t PERSIST_SIZE=4*1024*1024;//非强制刷写大小
const size_t PM_META_NODE_NUMBER=80;//TODO
const size_t PM_LOG_NUMBER=128;//TODO
//const size_t PM_LOG_NUMBER=200;//TODO
const char * PM_FILE_NAME="/mnt/pmemdir/pm_log";
const uint32_t META_NODE_MAGIC=0x0100;
const uint32_t PM_LOG_MAGIC=0x0101;
const uint32_t INVALID=0x00;

const std::string MIN_KEY="";
const std::string MAX_KEY="\xFF\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff";


const uint64_t PRE_SPLIT=7;//TODO
const uint64_t SPLIT=9;//TODO

const uint64_t PRE_MERGE=3;//TODO
const uint64_t MERGE=2;//TODO

const uint64_t K=7;//TODO 记录K次覆盖记录
const uint64_t PRE_SPLIT_NUMBER=3;//TODO
const uint64_t PRE_MERGE_NUMBER=3;//TODO


const uint64_t MIN_PARTITION=20;
const uint64_t AVG_PARTITION=40;
const uint64_t MAX_PARTITION=80;


const bool IS_FLUSH=true;

const uint64_t L0_THREAD_NUMBER=5;

const double NEW_SPLIT=1.0/80*2;
const double NEW_MERGE=1.0/80/2;
//const int extra_pm_log_const=20;
//int extra_pm_log=extra_pm_log_const;

MetaNode * NvmManager::get_meta_node() {
  mutex_.Lock();
  MetaNode *meta_node= nullptr;
  if(!free_meta_node_list_.empty()){
    meta_node=free_meta_node_list_.back();
    free_meta_node_list_.pop_back();
  }
  mutex_.Unlock();
  return meta_node;
}
PmLogHead * NvmManager::get_pm_log() {
  mutex_.Lock();
  PmLogHead *pm_log= nullptr;
  if(!free_pm_log_list_.empty()){
    pm_log=free_pm_log_list_.back();
    free_pm_log_list_.pop_back();
  }
  mutex_.Unlock();
  return pm_log;

}
void  NvmManager::free_meta_node(MetaNode * meta_node) {
  mutex_.Lock();
  free_meta_node_list_.emplace_back(meta_node);
  mutex_.Unlock();
}
void NvmManager::free_pm_log(PmLogHead* pm_log) {
  mutex_.Lock();
  free_pm_log_list_.emplace_back(pm_log);
  mutex_.Unlock();
}


NvmManager::NvmManager (bool is_recover_){
  size_t map_len;
  int is_pmem;
  if((base_= (char *)pmem_map_file(PM_FILE_NAME,PM_SIZE,PMEM_FILE_CREATE,0666,&map_len,&is_pmem))==NULL){
    perror("pmem_map_file");
    exit(1);
  }
  if(map_len!=PM_SIZE){
    perror("memory size not enough");
    pmem_unmap(base_,pm_size_);
    exit(1);
  }
  pm_log_base_=base_+PM_META_NODE_NUMBER*sizeof(MetaNode) ;
  if(is_recover_){
    //TODO recover
    for(int i=0;i<PM_META_NODE_NUMBER;i++){
       auto *meta_node=(MetaNode*)(base_+i*sizeof(MetaNode));
      if(meta_node->magic_number==META_NODE_MAGIC){
        recover_meta_nodes_.emplace_back(i*sizeof(MetaNode),meta_node);

      }else{
        reset(meta_node);
        free_meta_node_list_.emplace_back(meta_node);
      }

    }
    for(int i=0;i<PM_LOG_NUMBER;i++){
      PmLogHead *pm_log_head=(PmLogHead*)(pm_log_base_+i*PM_LOG_SIZE);
      if(pm_log_head->magic_number==PM_LOG_MAGIC){
        recover_pm_log_list_.emplace_back(PM_META_NODE_NUMBER*sizeof(MetaNode)+i*PM_LOG_SIZE,pm_log_head);
      }else{
        reset(pm_log_head);
        free_pm_log_list_.emplace_back(pm_log_head);
      }

    }
    L0_wait_=(free_pm_log_list_.size()-MAX_PARTITION)*0.3;
    L0_stop_=0;



  }else{
    for(int i=0;i<PM_META_NODE_NUMBER;i++){
      MetaNode *meta_node=(MetaNode*)(base_+i*sizeof(MetaNode));
      reset(meta_node);
      free_meta_node_list_.emplace_back(meta_node);
    }
    for(int i=0;i<PM_LOG_NUMBER;i++){
      PmLogHead *pm_log_head=(PmLogHead*)(pm_log_base_+i*PM_LOG_SIZE);
      reset(pm_log_head);
      free_pm_log_list_.emplace_back(pm_log_head);
    }
    L0_wait_=(free_pm_log_list_.size()-MAX_PARTITION)*0.3;
    L0_stop_=0;



  }
}
NvmManager::~NvmManager(){
  pmem_drain();
  pmem_unmap(base_,PM_SIZE);
}
std::vector<std::pair<uint64_t ,MetaNode *>>&& NvmManager::get_recover_meta_nodes_(){
  return std::move(recover_meta_nodes_);
}
std::vector<std::pair<uint64_t ,PmLogHead *>>&& NvmManager::get_recover_pm_log_nodes_(){
  return std::move(recover_pm_log_list_);
}
char * NvmManager::get_base() {
  return base_;
}
size_t NvmManager::get_free_pm_log_number(){
  return free_pm_log_list_.size();
}
size_t NvmManager::get_free_meta_node_number(){
  return free_meta_node_list_.size();
}
void reset(MetaNode *meta_node){
  pmem_memset_persist(meta_node,0,sizeof(MetaNode));
}
void reset(PmLogHead *pm_log_head){
  pmem_memset_persist(pm_log_head,0,sizeof(PmLogHead));
}

}