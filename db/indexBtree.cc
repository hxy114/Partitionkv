#include "indexBtree.h"
#include "db/version_set.h"
#include "util/mutexlock.h"
#include "db_impl.h"
namespace  leveldb{
PartitionIndexLayer::PartitionIndexLayer(VersionSet *const versions,
                                         port::Mutex &mutex,
                                         port::CondVar &background_work_finished_signal_L0,
                                         const InternalKeyComparator &internal_comparator,
                                         PmtableQueue &high_queue,
                                         PmtableQueue &low_queue,
                                         DBImpl *dbImpl)
    :versions_(versions),
      mutex_(mutex),
      internal_comparator_(internal_comparator),
      background_work_finished_signal_L0_(background_work_finished_signal_L0),
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
  auto find=bmap->lower_bound(key);
  return find!=bmap->end()?find->second:nullptr;
}

bool PartitionIndexLayer::add_new_partition(PartitionNode *partition_node){
  assert(partition_node!=nullptr);

  if((bmap->insert2(partition_node->get_end_key(),partition_node)).second){
    capacity_++;
    return true;
  }

  return false;
}

//根据该分区的key range的最大值进行检索，删除该叶子节点
bool PartitionIndexLayer::remove_partition(PartitionNode *partition_node){
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


    auto partition_node=seek_partition(key.ToString());
    PartitionNode::Status status=partition_node->Add(s,type,key,value,false);
    if(status==PartitionNode::Status::sucess){
      return;
    }else if(status==PartitionNode::Status::split){
      split(partition_node);

    }else if(status==PartitionNode::Status::merge){
      merge(partition_node);
    }
    partition_node=seek_partition(key.ToString());
    partition_node->Add(s,type,key,value,true);



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
      rightPartitionNode=pre->second;
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
void PartitionIndexLayer::merge(PartitionNode *partitionNode){

    if(capacity_>MIN_PARTITION){
      mutex_.Lock();
      assert(partitionNode->other_immuPmtable== nullptr&&partitionNode->immuPmtable== nullptr);
      Version *current=versions_->current();
      current->Ref();
      mutex_.Unlock();
      PartitionNode *acceptPartitionNode=getAceeptNode(current,partitionNode);
      if(acceptPartitionNode!= nullptr){
        std::string end_key=partitionNode->end_key;
        std::string accept_end_key=acceptPartitionNode->end_key;
        std::string new_start_key=std::min(partitionNode->start_key,acceptPartitionNode->start_key);
        std::string new_end_key=std::max(partitionNode->end_key,acceptPartitionNode->end_key);

        mutex_.Lock();
        PmTable *pmTable=partitionNode->pmtable;
        pmTable->SetRole(PmTable::other_immuPmtable);
        pmTable->left_father_=acceptPartitionNode;
        high_queue_.InsertPmtable(pmTable);
        dbImpl_->MaybeScheduleCompactionL0();
        pmTable->SetPmTableStatus(PmTable::PmTable_Status::IN_HIGH_QUEUE);//转移pmtable

        acceptPartitionNode->set_range(new_start_key,new_end_key);
        acceptPartitionNode->set_other_immupmtable(pmTable);

        partitionNode->reset_pmtable();

        remove_partition_by_key(end_key);
        remove_partition_by_key(accept_end_key);
        add_new_partition(acceptPartitionNode);

        partitionNode->FreePartitionNode();
        delete partitionNode;
        acceptPartitionNode->FLush();
        current->Unref();
        mutex_.Unlock();
      }else{
        mutex_.Lock();
        current->Unref();
        mutex_.Unlock();
      }
    }

}
void PartitionIndexLayer::split(PartitionNode *partitionNode){
    if(capacity_<MAX_PARTITION){
      mutex_.Lock();
      assert(partitionNode->other_immuPmtable== nullptr&&partitionNode->immuPmtable== nullptr);
      Version *current=versions_->current();
      current->Ref();
      mutex_.Unlock();
      std::vector<FileMetaData*>files;
      SequenceNumber max_snapshot = versions_->LastSequence(),min_snapshot=0;
      InternalKey start=InternalKey(partitionNode->start_key, max_snapshot, ValueType::kTypeValue) ;
      InternalKey end=InternalKey(partitionNode->end_key, min_snapshot, ValueType::kTypeValue) ;
      current->GetOverlappingInputs(1,&start,&end,&files);
      std::string splitKey=files[files.size()/2]->smallest.user_key().ToString();

      PmTable *pmTable=partitionNode->pmtable;
      mutex_.Lock();
      MetaNode *newMetaNode=nvmManager->get_meta_node();
      if(newMetaNode!= nullptr){
        PartitionNode *newPartitionNode=new PartitionNode(partitionNode->start_key,splitKey,newMetaNode,versions_,mutex_,background_work_finished_signal_L0_,internal_comparator_,high_queue_,low_queue_,dbImpl_);
        newPartitionNode->set_other_immupmtable(pmTable);
        PmTable *pmTable1=new PmTable(internal_comparator_,newPartitionNode);
        newPartitionNode->set_pmtable(pmTable1);
        add_new_partition(newPartitionNode);

        partitionNode->set_range(splitKey,partitionNode->end_key);
        partitionNode->set_other_immupmtable(pmTable);
        partitionNode->reset_pmtable();
        PmTable *pmTable2=new PmTable(internal_comparator_,partitionNode);
        partitionNode->set_pmtable(pmTable2);

        pmTable->left_father_=newPartitionNode;
        pmTable->right_father_=partitionNode;
        pmTable->SetRole(PmTable::other_immuPmtable);
        low_queue_.InsertPmtable(pmTable);
        pmTable->SetPmTableStatus(PmTable::IN_LOW_QUQUE);

        dbImpl_->MaybeScheduleCompactionL0();
        current->Unref();
        mutex_.Unlock();

        partitionNode->FLush();
        newPartitionNode->FLush();

      }else{
        exit(2);
      }
    }


}

void PartitionIndexLayer::init(){
    MetaNode *newMetaNode=nvmManager->get_meta_node();
    if(newMetaNode!= nullptr){
      PartitionNode *newPartitionNode=new PartitionNode(MIN_KEY,MAX_KEY,newMetaNode,versions_,mutex_,background_work_finished_signal_L0_,internal_comparator_,high_queue_,low_queue_,dbImpl_);
      PmTable *pmTable=new PmTable(internal_comparator_,newPartitionNode);
      newPartitionNode->set_pmtable(pmTable);
      add_new_partition(newPartitionNode);
    }else{
      exit(1);
    }


}
void PartitionIndexLayer::recover(){
    //TODO 恢复
}
}
