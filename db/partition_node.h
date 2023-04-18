#ifndef PARTITION_NODE
#define PARTITION_NODE

//partition node节点
//主要用于在分区内索引

class Partition_Node{
private:
    char *partiton_node;
public:
    Partition_Node():partiton_node(nullptr){}
    ~Partition_Node(){
        if(partiton_node!=nullptr) delete partiton_node;
    }

    //设置该partition_node的key range
    void set_range(const unsigned int &startkey,const unsigned int &endkey);
    //设置指针
    void set_pmtable_pointer();
    void set_immupmtable_pointer();
    void set_otherimmupmtable_pointer();
    void set_metanode_pointer();

    //将partition_node持久化到Meta node中
    void persistence_storage();

};

#endif