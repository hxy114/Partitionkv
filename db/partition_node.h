#ifndef PARTITION_NODE
#define PARTITION_NODE

//partition node节点
//主要用于在分区内索引

class Partition_Node{
private:
    char *node;
public:
    Partition_Node();
    ~Partition_Node();

    //设置该partition_node的key range
    void set_range(uint64_t &startkey,uint64_t &endkey);
    //设置指针
    void set_pmtable_pointer();
    void set_immupmtable_pointer();
    void set_otherimmupmtable_pointer();
    void set_metanode_pointer();

    //将partition_node持久化到Meta node中
    void persistence_storage();
    //返回end key
    uint64_t get_key_range_max();
    //返回start key
    uint64_t get_key_range_min();
};

#endif