#include <iostream>
#include <memory>
#include <type_traits>

template <typename T>
typename std::remove_reference<T>::type &&mymove(T &&param){
    using returnType = typename std::remove_reference<T>::type &&;
    return static_cast<returnType>(param);
}

void process(const A &lval){
    std::cout<<"deal lval"<<std::endl;
}

void process(A &&rval){
    std::cout<<"deal rval"<<std::endl;
}

template <typename T>
void logAndProcess(T &&param){
    process(std::forwar<T>param);
}

struct A{
    int b;
    A(int value) : b(value){
        std::cout<<"create"<<std::endl;
    }

    A(const A&value){
        std::cout<<"copy"<<std::endl;
        b = value.b;
    }

    A(A &&value){
        std::cout<<"move"<<std::endl;
        b = value.b;
        value.b = 0;
    }
};

class Annotation{
public:
    explicit Annotation(const A a) : a_param(std::move(a)){}

private:
    A a_param;
};

int main(){
    // int aaa = 10;
    // int &&bbb = mymove(10);
    // int &&ccc = mymove(aaa);o

    Annotation a{10};

    return 0;
}