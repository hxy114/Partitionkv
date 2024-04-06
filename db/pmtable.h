#ifndef PMTABLE
#define PMTABLE
#include"skiplist.h"
#include"arena.h"


class PMLog{

};

class PMtable{
private:
    PMLog *PMLog;
    leveldb::SkipList<uint64_t,char*>* const skiplist;
    leveldb::Arena* const arena;
public:
    PMtable();
    void put();
    void get();

};


#endif