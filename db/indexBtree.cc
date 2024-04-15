#include "indexBtree.h"
#include "db/version_set.h"
#include "util/mutexlock.h"
#include "db_impl.h"
namespace  leveldb{
PartitionIndexLayer::PartitionIndexLayer(VersionSet *const versions,
                                         port::Mutex &mutex,
                                         port::CondVar &background_work_finished_signal_L0,
                                         const InternalKeyComparator &internal_comparator,
                                         PmtableQueue &top_queue,
                                         PmtableQueue &high_queue,
                                         PmtableQueue &low_queue,
                                         DBImpl *dbImpl)
    :versions_(versions),
      mutex_(mutex),
      internal_comparator_(internal_comparator),
      background_work_finished_signal_L0_(background_work_finished_signal_L0),
      top_queue_(top_queue),
      high_queue_(high_queue),
      low_queue_(low_queue),
      capacity_(0),
      dbImpl_(dbImpl){
  this->bmap=new tlx::btree_map<std::string,PartitionNode*>();
}

PartitionIndexLayer::~PartitionIndexLayer(){
  delete bmap;
}

PartitionNode* PartitionIndexLayer::seek_partition(const std::string &key){
  mutex_.AssertHeld();
  auto find=bmap->lower_bound(key);
  bmap->begin();
  return find!=bmap->end()?find->second:nullptr;
}

bool PartitionIndexLayer::add_new_partition(PartitionNode *partition_node){
  mutex_.AssertHeld();
  assert(partition_node!=nullptr);

  if((bmap->insert2(partition_node->get_end_key(),partition_node)).second){
    capacity_++;
    return true;
  }

  return false;
}

//根据该分区的key range的最大值进行检索，删除该叶子节点
bool PartitionIndexLayer::remove_partition(PartitionNode *partition_node){
  mutex_.AssertHeld();
  assert(partition_node!=nullptr);

  auto it=bmap->find(partition_node->get_end_key());
  if(it!=bmap->end()){
    bmap->erase(it);
    capacity_--;
    return true;
  }
  return false;

}
bool PartitionIndexLayer::remove_partition_by_key(std::string &key){
  mutex_.AssertHeld();
  auto it=bmap->find(key);
  if(it!=bmap->end()){
    bmap->erase(it);
    capacity_--;
    return true;
  }
  return false;
}
void PartitionIndexLayer::Add(SequenceNumber s, ValueType type, const Slice& key,
                   const Slice& value) {

  {
    MutexLock l(&mutex_);
    const uint64_t start_micros = dbImpl_->env_->NowMicros();
    if (dbImpl_->versions_->NumLevelFiles(1)  >
        PM_META_NODE_NUMBER *4 * 1.3) {
      mutex_.Unlock();
      dbImpl_->env_->SleepForMicroseconds(1000);
      mutex_.Lock();
    }
    while(dbImpl_->versions_->NumLevelFiles(1) >
        PM_META_NODE_NUMBER  *4* 1.5){
      dbImpl_->background_work_finished_signal_.Wait();
    }
    const uint64_t cost = dbImpl_->env_->NowMicros() - start_micros;
    if(cost>2000){
      Log(dbImpl_->options_.info_log,"L1 big wait %ld",cost);
    }
  }


    auto partition_node=seek_partition(key.ToString());
    PartitionNode::Status status=partition_node->Add(s,type,key,value,false,capacity_);
    if(status==PartitionNode::Status::sucess){
      return;
    }else if(status==PartitionNode::Status::split){
      status=split(partition_node);

    }else if(status==PartitionNode::Status::merge){
      status=merge(partition_node);
    }
    if(status==PartitionNode::noop){
      mutex_.Lock();
      PmLogHead *pmlog= nullptr;
      while((pmlog=nvmManager->get_pm_log())== nullptr){
      background_work_finished_signal_L0_.Wait();
      }
      PmTable *newPmTable=new PmTable(internal_comparator_,partition_node,pmlog);

      partition_node->set_pmtable(newPmTable);
      mutex_.Unlock();
      partition_node->FLush();

    }else{
      partition_node=seek_partition(key.ToString());
    }
    partition_node->Add(s,type,key,value,true,capacity_);



}
PartitionNode * PartitionIndexLayer::getAceeptNode(Version *current,PartitionNode *partitionNode){
    auto find=bmap->lower_bound(partitionNode->end_key),pre=find,next=find;

    PartitionNode *leftPartitionNode= nullptr,*rightPartitionNode= nullptr,*acceptNode= nullptr;
    size_t size=INT64_MAX;
    if(find!=bmap->begin()){
      pre--;
      leftPartitionNode=pre->second;

    }
    next++;
    if(next!=bmap->end()){
      rightPartitionNode=next->second;
    }

    if(leftPartitionNode&&leftPartitionNode->other_immuPmtable== nullptr){
      SequenceNumber max_snapshot = versions_->LastSequence(),min_snapshot=0;
      InternalKey start=InternalKey(leftPartitionNode->start_key, max_snapshot, ValueType::kTypeValue) ;
      InternalKey end=InternalKey(leftPartitionNode->end_key, min_snapshot, ValueType::kTypeValue) ;
      size=current->GetOverlappingSize(1,&start,&end);
      acceptNode=leftPartitionNode;
    }
    if(rightPartitionNode&&rightPartitionNode->other_immuPmtable== nullptr){
      SequenceNumber max_snapshot = versions_->LastSequence(),min_snapshot=0;
      InternalKey start=InternalKey(rightPartitionNode->start_key, max_snapshot, ValueType::kTypeValue) ;
      InternalKey end=InternalKey(rightPartitionNode->end_key, min_snapshot, ValueType::kTypeValue) ;
      auto s=current->GetOverlappingSize(1,&start,&end);
      if(s<size){
        acceptNode=rightPartitionNode;
      }
    }
    return acceptNode;


}
PartitionNode::Status PartitionIndexLayer::merge(PartitionNode *partitionNode){
    //return  PartitionNode::noop;
    if(capacity_>MIN_PARTITION){
      mutex_.Lock();
      assert(partitionNode->other_immuPmtable== nullptr&&partitionNode->pmtable== nullptr);
      Version *current=versions_->current();
      current->Ref();
      mutex_.Unlock();
      PartitionNode *acceptPartitionNode=getAceeptNode(current,partitionNode);
      if(acceptPartitionNode!= nullptr){
        Log(dbImpl_->options_.info_log, "mergeing partitionNode:start:%s,end%s.acceptnode:start:%s,end:%s,capacity:%d",
            partitionNode->start_key.c_str(), partitionNode->end_key.c_str(),
            acceptPartitionNode->start_key.c_str(),acceptPartitionNode->end_key.c_str(),capacity_);
        std::string end_key=partitionNode->end_key;
        std::string accept_end_key=acceptPartitionNode->end_key;
        std::string new_start_key=std::min(partitionNode->start_key,acceptPartitionNode->start_key);
        std::string new_end_key=std::max(partitionNode->end_key,acceptPartitionNode->end_key);

        mutex_.Lock();

        PmTable *immutable_list=partitionNode->immuPmtable;
        if(immutable_list){
          immutable_list->SetRole(PmTable::other_immuPmtable);
          immutable_list->SetLeftFather(acceptPartitionNode);
          if(immutable_list->status_==PmTable::IN_LOW_QUQUE){
            low_queue_.RemovePmtable(immutable_list);
            immutable_list->status_=PmTable::IN_TOP_QUEUE;
            top_queue_.InsertPmtable(immutable_list);
          }else if(immutable_list->status_==PmTable::IN_HIGH_QUEUE){
            high_queue_.RemovePmtable(immutable_list);
            immutable_list->status_=PmTable::IN_TOP_QUEUE;
            top_queue_.InsertPmtable(immutable_list);
          }
          acceptPartitionNode->set_other_immupmtable(immutable_list);
        }


        acceptPartitionNode->set_range(new_start_key,new_end_key);


        partitionNode->reset_immuPmtable();

        remove_partition_by_key(end_key);
        remove_partition_by_key(accept_end_key);
        add_new_partition(acceptPartitionNode);

        partitionNode->FreePartitionNode();
        delete partitionNode;
        acceptPartitionNode->FLush();
        current->Unref();
        mutex_.Unlock();
        Log(dbImpl_->options_.info_log, "mergeed new partition:start:%s ,end:%s,capacity:%d",
            acceptPartitionNode->start_key.c_str(),acceptPartitionNode->end_key.c_str(),capacity_);
        return  PartitionNode::sucess;
      }else{
        mutex_.Lock();
        current->Unref();
        mutex_.Unlock();
      }
      return  PartitionNode::noop;

    }
    return  PartitionNode::noop;

}
PartitionNode::Status PartitionIndexLayer::init_split(PartitionNode *partitionNode){


}
PartitionNode::Status PartitionIndexLayer::split(PartitionNode *partitionNode){
    //return  PartitionNode::noop;
    if(capacity_<MAX_PARTITION){
      Log(dbImpl_->options_.info_log, "spliting partitionNode:start:%s,end%s,caption:%d",
          partitionNode->start_key.c_str(), partitionNode->end_key.c_str(),capacity_);
      mutex_.Lock();
      assert(partitionNode->other_immuPmtable== nullptr&&partitionNode->pmtable== nullptr);
      Version *current=versions_->current();
      current->Ref();
      mutex_.Unlock();
      std::vector<FileMetaData*>files;
      SequenceNumber max_snapshot = versions_->LastSequence(),min_snapshot=0;
      InternalKey start=InternalKey(partitionNode->start_key, max_snapshot, ValueType::kTypeValue) ;
      InternalKey end=InternalKey(partitionNode->end_key, min_snapshot, ValueType::kTypeValue) ;
      current->GetOverlappingInputs(1,&start,&end,&files);

      if(!files.empty()){

        std::string splitKey=files[files.size()/2]->smallest.user_key().ToString();


        mutex_.Lock();
        MetaNode *newMetaNode= nullptr;
        PmLogHead *pmLogHead1= nullptr,*pmLogHead2= nullptr;
        if((newMetaNode=nvmManager->get_meta_node())== nullptr){
          exit(2);
        }
        while((pmLogHead1=nvmManager->get_pm_log())== nullptr){
          background_work_finished_signal_L0_.Wait();
        }
        while((pmLogHead2=nvmManager->get_pm_log())== nullptr){
          background_work_finished_signal_L0_.Wait();
        }

        PmTable *immutable_list=partitionNode->immuPmtable;
        PartitionNode *newPartitionNode=new PartitionNode(partitionNode->start_key,splitKey,newMetaNode,versions_,mutex_,background_work_finished_signal_L0_,internal_comparator_,top_queue_,high_queue_,low_queue_,dbImpl_);
        if(immutable_list){
          newPartitionNode->set_other_immupmtable(immutable_list);
          partitionNode->set_other_immupmtable(immutable_list);

          immutable_list->SetLeftFather(newPartitionNode);
          immutable_list->SetRightFather(partitionNode);
          immutable_list->SetRole(PmTable::other_immuPmtable);

          if(immutable_list->status_==PmTable::IN_LOW_QUQUE){
            low_queue_.RemovePmtable(immutable_list);
            immutable_list->status_=PmTable::IN_TOP_QUEUE;
            top_queue_.InsertPmtable(immutable_list);
          }else if(immutable_list->status_==PmTable::IN_HIGH_QUEUE){
            high_queue_.RemovePmtable(immutable_list);
            immutable_list->status_=PmTable::IN_TOP_QUEUE;
            top_queue_.InsertPmtable(immutable_list);
          }
        }
        PmTable *pmTable1=new PmTable(internal_comparator_,newPartitionNode,pmLogHead1);
        newPartitionNode->set_pmtable(pmTable1);
        add_new_partition(newPartitionNode);

        partitionNode->set_range(splitKey,partitionNode->end_key);
        partitionNode->reset_cover();
        partitionNode->reset_immuPmtable();
        PmTable *pmTable2=new PmTable(internal_comparator_,partitionNode,pmLogHead2);
        partitionNode->set_pmtable(pmTable2);





        current->Unref();
        mutex_.Unlock();

        partitionNode->FLush();
        newPartitionNode->FLush();
        Log(dbImpl_->options_.info_log, "splited partitionNode1:start:%s,end%s.partitionNode2:start:%s,end:%s,capacity:%d",
            partitionNode->start_key.c_str(), partitionNode->end_key.c_str(),
            newPartitionNode->start_key.c_str(),newPartitionNode->end_key.c_str(),capacity_);
        return  PartitionNode::sucess;


      }else{
        mutex_.Lock();
        current->Unref();
        mutex_.Unlock();
        return PartitionNode::noop;
      }





    }
    return  PartitionNode::noop;


}

void PartitionIndexLayer::init(){
    MetaNode *newMetaNode=nvmManager->get_meta_node();
    if(newMetaNode!= nullptr){
      PartitionNode *newPartitionNode=new PartitionNode(MIN_KEY,MAX_KEY,newMetaNode,versions_,mutex_,background_work_finished_signal_L0_,internal_comparator_,top_queue_,high_queue_,low_queue_,dbImpl_);
      PmLogHead  *pmLogHead=nvmManager->get_pm_log();
      PmTable *pmTable=new PmTable(internal_comparator_,newPartitionNode,pmLogHead);
      newPartitionNode->set_pmtable(pmTable);
      add_new_partition(newPartitionNode);
    }else{
      exit(1);
    }


}
tlx::btree_map<std::string,PartitionNode*> * PartitionIndexLayer::get_bmap() {
    return bmap;
}
void PartitionIndexLayer::recover(){
    //TODO 恢复
}
}
