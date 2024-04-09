#include"partition_node.h"
#include <iostream>
#include <string.h>
#include <cassert>
#include "db/version_edit.h"
#include "db/version_set.h"
#include "util/mutexlock.h"
#include "db_impl.h"
namespace  leveldb{
PartitionNode::PartitionNode(const std::string &start_key1,
                             const std::string &end_key1,
                             MetaNode *metaNode1,
                             VersionSet * versions,
                             port::Mutex &mutex,
                             port::CondVar &background_work_finished_signal_L0,
                             InternalKeyComparator &internal_comparator,
                             PmtableQueue &high_queue,
                             PmtableQueue &low_queue,
                             DBImpl *dbImpl)
              :versions_(versions),
              mutex_(mutex),base_(nvmManager->get_base()),
              metaNode(metaNode1),internal_comparator_(internal_comparator),
              background_work_finished_signal_L0_(background_work_finished_signal_L0),
              high_queue_(high_queue),
              low_queue_(low_queue),
              dbImpl_(dbImpl){
  init(start_key1,end_key1);
}

PartitionNode::~PartitionNode(){
  pmem_persist(metaNode,sizeof(MetaNode));
  nvmManager->free_meta_node(metaNode);

}
void PartitionNode::FreePartitionNode() {
  assert(immuPmtable== nullptr&&pmtable== nullptr&&other_immuPmtable== nullptr);
  metaNode->magic_number=INVALID;
  //pmem_persist(metaNode,sizeof(MetaNode));
}

void PartitionNode::FLush(){
  if(IS_FLUSH){
    pmem_persist(metaNode,PM_META_NODE_SIZE);
  }

}
void PartitionNode::init(const std::string &startkey,const std::string &endkey){
  assert(endkey>=startkey);
  //小端存储
  start_key=startkey;
  end_key=endkey;
  metaNode->start_key_size=start_key.size();
  pmem_memcpy_nodrain(&metaNode->start_key,startkey.c_str(),startkey.size());
  metaNode->end_key_size=end_key.size();
  pmem_memcpy_nodrain(&metaNode->start_key,endkey.c_str(),endkey.size());
  metaNode->magic_number=META_NODE_MAGIC;
  //pmem_drain();
}
void PartitionNode::set_range(std::string &startkey,std::string &endkey){
  assert(endkey>=startkey);
  //小端存储
  start_key=startkey;
  end_key=endkey;
  metaNode->start_key_size=start_key.size();
  pmem_memcpy_nodrain(&metaNode->start_key,startkey.c_str(),startkey.size());
  metaNode->end_key_size=end_key.size();
  pmem_memcpy_nodrain(&metaNode->start_key,endkey.c_str(),endkey.size());
  //pmem_drain();
}
void PartitionNode::set_other_immupmtable(PmTable *otherImmuPmtable1){
  assert(other_immuPmtable== nullptr&&otherImmuPmtable1!= nullptr);
  otherImmuPmtable1->Ref();
  other_immuPmtable=otherImmuPmtable1;
  metaNode->other_immu_pm_log=(uint64_t)other_immuPmtable->pmLogHead_-(uint64_t)base_;
  //pmem_drain();

}
void PartitionNode::set_immuPmtable(PmTable *immuPmtable1){
  assert(immuPmtable== nullptr&&immuPmtable1!= nullptr);
  immuPmtable1->Ref();
  immuPmtable=immuPmtable1;
  metaNode->immu_pm_log=(uint64_t)immuPmtable->pmLogHead_-(uint64_t)base_;
  //pmem_drain();

}
void PartitionNode::set_pmtable(PmTable *pmTable){
  assert(pmtable== nullptr&&pmTable!= nullptr);
  pmTable->Ref();
  pmtable=pmTable;
  metaNode->pm_log=(uint64_t)pmtable->pmLogHead_-(uint64_t)base_;
  //pmem_drain();
}
void PartitionNode::reset_other_immupmtable(){
  if(other_immuPmtable!= nullptr){
    other_immuPmtable->Unref();
  }
  other_immuPmtable= nullptr;
  metaNode->other_immu_pm_log=0;
  //pmem_drain();

}
void PartitionNode::reset_immuPmtable(){
  if(immuPmtable!= nullptr){
    immuPmtable->Unref();
  }
  immuPmtable= nullptr;
  metaNode->immu_pm_log=0;
  //pmem_drain();

}
bool PartitionNode::has_other_immupmtable(){
  return !(other_immuPmtable == nullptr);
}
void PartitionNode::reset_pmtable(){
  if(pmtable!= nullptr){
    pmtable->Unref();
  }
  pmtable= nullptr;
  metaNode->pm_log=0;
  //pmem_drain();

}
//返回end key
std::string & PartitionNode::get_end_key(){
  return end_key;
}
//返回start key
std::string & PartitionNode::get_start_key(){
  return start_key;
}
PartitionNode::Status PartitionNode::Add(SequenceNumber s, ValueType type, const Slice& key,
                                const Slice& value,bool is_force) {

  bool ret=pmtable->Add(s,type,key,value);
  if(!ret){
    if(is_force){
      mutex_.Lock();
      while(immuPmtable!= nullptr){
          background_work_finished_signal_L0_.Wait();
      }
      PmTable *immupmTable=pmtable;
      set_immuPmtable(immupmTable);
      reset_pmtable();
      immupmTable->role_=PmTable::immuPmtable;
      immupmTable->status_=PmTable::IN_LOW_QUQUE;
      low_queue_.InsertPmtable(immupmTable);
      dbImpl_->MaybeScheduleCompactionL0();
      PmTable *pmTable1=new PmTable(internal_comparator_,this);
      set_pmtable(pmTable1);
      mutex_.Unlock();
      FLush();
      pmtable->Add(s,type,key,value);
      return sucess;
    }else{
      mutex_.Lock();
      while(immuPmtable!= nullptr){
          background_work_finished_signal_L0_.Wait();
      }

      Status status=sucess;
      auto current=versions_->current();
      bool has_other_immupmtable= immuPmtable != nullptr;
      current->Ref();
      mutex_.Unlock();

      SequenceNumber max_snapshot = versions_->LastSequence(),min_snapshot=0;
      InternalKey start=InternalKey(start_key, max_snapshot, ValueType::kTypeValue) ;
      InternalKey end=InternalKey(end_key, min_snapshot, ValueType::kTypeValue) ;
      auto size=current->GetOverlappingSize(1,&start,&end);
      if(cover_.size()<K){
          cover_.push_back(size);
      }else{
          cover_[index_]=size;
          index_=(index_+1)%K;
      }

      if(!has_other_immupmtable){
          if(size>=SPLIT){
            status=split;
            mutex_.Lock();
            current->Unref();
            mutex_.Unlock();
            return status;
          }else if(size<=MERGE){
            status=merge;
            mutex_.Lock();
            current->Unref();
            mutex_.Unlock();
            return status;
          }else{
            if((status=needSplitOrMerge())==sucess){
              mutex_.Lock();
              PmTable *immupmTable=pmtable;
              set_immuPmtable(immupmTable);
              reset_pmtable();
              immupmTable->role_=PmTable::immuPmtable;
              immupmTable->status_=PmTable::IN_LOW_QUQUE;
              low_queue_.InsertPmtable(immupmTable);
              dbImpl_->MaybeScheduleCompactionL0();
              PmTable *pmTable1=new PmTable(internal_comparator_,this);
              set_pmtable(pmTable1);
              current->Unref();
              mutex_.Unlock();
              FLush();
              pmtable->Add(s,type,key,value);
              return sucess;
            }
          }

      }else{
          mutex_.Lock();
          PmTable *immupmTable=pmtable;
          set_immuPmtable(immupmTable);
          reset_pmtable();
          immupmTable->role_=PmTable::immuPmtable;
          immupmTable->status_=PmTable::IN_LOW_QUQUE;
          low_queue_.InsertPmtable(immupmTable);

          PmTable *pmTable1=new PmTable(internal_comparator_,this);
          set_pmtable(pmTable1);
          current->Unref();
          mutex_.Unlock();
          FLush();
          pmtable->Add(s,type,key,value);
          return sucess;

      }

    }
  }
  if(pmtable->ApproximateMemoryUsage()>PM_LOG_SIZE/2){
    mutex_.Lock();
    if(immuPmtable!= nullptr&&immuPmtable->GetPmTableStatus()==PmTable::PmTable_Status::IN_LOW_QUQUE){
      low_queue_.RemovePmtable(immuPmtable);
      high_queue_.InsertPmtable(immuPmtable);
    }
    mutex_.Unlock();
  }

  return PartitionNode::Status::sucess;

}
PartitionNode::Status PartitionNode::needSplitOrMerge(){
  Status status=sucess;
  size_t split_number=0,merge_number=0;
  for(int i=0;i<cover_.size();i++){
    if(cover_[i]>=PRE_SPLIT){
      split_number++;
    }else if(cover_[i]<=PRE_MERGE){
      merge_number++;
    }
  }
  if(split_number>=PRE_SPLIT_NUMBER){
    status=split;
    return status;
  }
  if(merge_number>=PRE_MERGE_NUMBER){
    status=merge;
    return status;
  }
  return status;

}

}

