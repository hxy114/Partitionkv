#ifndef PMTABLE
#define PMTABLE
#include"skiplist.h"


class PMLog{

};

class PMtable{
private:
    PMLog *PMLog;
    leveldb::SkipList<uint64_t,char*> *skiplist;
    
public:
    PMtable();
    void put();
    void get();

};


#endif