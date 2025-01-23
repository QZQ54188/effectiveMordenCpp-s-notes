#include <string>
#include <vector>
#include <iostream>

struct test1{
    static int SubType;
};

struct test2{
    typedef int SubType;
};

template <typename T>
class MyClass{
public:
    void foo(){
        // int ptr = 10;
        typename T::SubType *ptr;
    }
};

template <class T>
using myVector = std::vector<T>;

template <typename T>
struct myVector2{
    typedef std::vector<T> type;
};

template <typename T>
class Widget{
public:
    myVector<T> list;
};

template <typename T>
class Widget2{
public:
    typename myVector2<T>::type list;
};

int main(){
//     MyClass<test1> t1;    /*编译器将其视为乘法，相当于int类型的数据成员
//    SubType乘以ptr，但是ptr在之前没有声明，所以会报错*/ 
//     t1.foo();
    // MyClass<test2> t2;    //编译器将其视为基本类型int，创建一个指向int的指针
    // t2.foo();
}

