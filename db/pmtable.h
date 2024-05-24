//
// Created by hxy on 24-4-2.
//

#ifndef LEVELDB_PMTABLE_H
#define LEVELDB_PMTABLE_H
#include <string>
#include <unordered_map>

#include "db/dbformat.h"
#include "db/skiplist.h"
#include "leveldb/db.h"
#include "util/arena.h"
#include "util/nvm_arena.h"
namespace leveldb {

class InternalKeyComparator;
class PmTableIterator;
class PartitionIndexLayer;
class PartitionNode;
class PmTable {
 public:
  enum Role{
    pmtable,
    immuPmtable,
    other_immuPmtable,
  };
  enum PmTable_Status{
    IN_RECEVIE,
    IN_LOW_QUQUE,
    IN_HIGH_QUEUE,
    IN_TOP_QUEUE,
    IN_FOLLOW,
    IN_HEAD,
    IN_COMPACTIONING,
    IN_COMPACTIONED,
  };
  friend  class PartitionIndexLayer;
  friend class PartitionNode;
  // MemTables are reference counted.  The initial reference count
  // is zero and the caller must call Ref() at least once.
  explicit PmTable(const InternalKeyComparator& comparator,PartitionNode *partitionNode,PmLogHead *pmLogHead);
  explicit PmTable(const InternalKeyComparator& comparator,PmLogHead *pmLogHead);
  void init(PartitionNode *partitionNode);
  PmTable(const PmTable&) = delete;
  PmTable& operator=(const PmTable&) = delete;
  void FreePmtable();//设置magoc number为无效
  // Increase reference count.
  void Ref() { ++refs_; }

  // Drop reference count.  Delete if no more references exist.
  void Unref() {
    --refs_;
    assert(refs_ >= 0);
    if (refs_ <= 0) {
      delete this;
    }
  }

  // Returns an estimate of the number of bytes of data in use by this
  // data structure. It is safe to call when MemTable is being modified.
  size_t ApproximateMemoryUsage();

  // Return an iterator that yields the contents of the memtable.
  //
  // The caller must ensure that the underlying MemTable remains live
  // while the returned iterator is live.  The keys returned by this
  // iterator are internal keys encoded by AppendInternalKey in the
  // db/format.{h,cc} module.
  Iterator* NewIterator();

  // Add an entry into memtable that maps key to value at the
  // specified sequence number and with the specified type.
  // Typically value will be empty if type==kTypeDeletion.
  bool Add(SequenceNumber seq, ValueType type, const Slice& key,
           const Slice& value);
  void add(char *buffer);
  std::string &GetMinKey();
  std::string &GetMaxKey();
  // If memtable contains a value for key, store it in *value and return true.
  // If memtable contains a deletion for key, store a NotFound() error
  // in *status and return true.
  // Else, return false.
  bool Get(const LookupKey& key, std::string* value, Status* s);
  void SetRole(Role role);
  Role GetRole();
  void SetLeftFather(PartitionNode *left_father);
  void SetRightFather(PartitionNode *right_father);
  void SetPmTableStatus(PmTable_Status pmTableStatus);
  PmTable_Status GetPmTableStatus();
  PartitionNode * GetLeftFather();
  PartitionNode * GetRightFather();
  void SetNext(PmTable *pmTable);
 private:
  friend class PmTableIterator;
  friend class PmTableBackwardIterator;
  friend class DBImpl;
  friend class VersionSet;
  friend class Version;
  struct KeyComparator {
    const InternalKeyComparator comparator;
    explicit KeyComparator(const InternalKeyComparator& c) : comparator(c) {}
    int operator()(const char* a, const char* b) const;
  };

  typedef SkipList<const char*, KeyComparator> Table;

  ~PmTable();  // Private since only Unref() should be used to delete it

  KeyComparator comparator_;
  int refs_;
  Arena arena_;
  Table table_;
  PmLogHead *pmLogHead_;
  NvmArena nvmArena_;
  Role role_;
  PmTable_Status status_;//immu才有效
  PartitionNode * left_father_;
  PartitionNode * right_father_;
  std::string min_key_;
  std::string max_key_;
  PmTable *next_;

};

class PmtableQueue{
 public:
  class ListNode{
   public:

    ListNode(PmTable *pmTable);
    PmTable *pmTable_;
    ListNode *pre,*next;

  };
  PmtableQueue();
  ~PmtableQueue();
  ListNode *GetHead();
  void InsertPmtable(PmTable *pmtable);
  void RemovePmtable(PmTable *pmTable);
  size_t capacity();
 private:
  std::unordered_map<PmTable*,ListNode*>mp_;
  ListNode *head_;


};
}  // namespace leveldb
#endif  // LEVELDB_PMTABLE_H
