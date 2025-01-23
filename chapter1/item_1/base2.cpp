#include <iostream>
#include <string.h>
#include <string>

class BigMemoryPool
{
private:
    char *pool_;
public:
    static const int PoolSize = 4096;
    BigMemoryPool() : pool_(new char[PoolSize]){
        std::cout<<"default"<<std::endl;
    }

    ~BigMemoryPool(){
        if(pool_ != nullptr){
            delete[] pool_;
        }
    }
    BigMemoryPool(BigMemoryPool &&other){
        std::cout << "move" << std::endl;
        pool_ = other.pool_;
        other.pool_ = nullptr;
    }
    BigMemoryPool(const BigMemoryPool &other) : pool_(new char[PoolSize]){
        std::cout<<"copy"<<std::endl;
        memcpy(pool_, other.pool_, PoolSize);
    }
};

BigMemoryPool getPool(){
    BigMemoryPool memorypool;

    return memorypool;
}

BigMemoryPool getPool(const BigMemoryPool &pool){
    std::cout<<"return pool"<<std::endl;
    return pool;
}

BigMemoryPool make_pool(){
    BigMemoryPool pool;
    std::cout<<"pool con"<<std::endl;
    return getPool(pool);
}

int gx = 10;
int get_gx(){
    return gx;
}
int get_x(){
    int x = 20;
    return x;
}

int main(){

    // &get_x();//右值
    // &get_gx();//右值

    // int x = 10;
    // int &&y = 10;
    // y = 20;
    BigMemoryPool aaa;
    std::cout<<"a has been constructed"<<std::endl;
    BigMemoryPool ccc = std::move(aaa);
    // std::cout<<"c refer to a"<<std::endl;
    // BigMemoryPool &&ddd = std::move(aaa);
    // std::cout<<"-----------"<<std::endl;
    // BigMemoryPool bbb = getPool();
    BigMemoryPool ccc = make_pool();

    return 0;
}

