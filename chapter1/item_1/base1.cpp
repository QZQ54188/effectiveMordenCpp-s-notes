#include <iostream>
#include <vector>

int main(){
    // const int a = 10;
    // int b = a;

    // int *ptr = (int *)&a;
    // *ptr = 20;

    // std::cout<<*ptr<<std::endl;
    // std::cout<<a<<std::endl;
    std::vector<int> v;
    auto resetV = 
    [&v](const auto& newValue){ v = newValue; };        //C++14
    // resetV({ 1, 2, 3 });            //错误！不能推导{ 1, 2, 3 }的类型

    auto a = {1,2,3};

    // const int *const p = new int(10);

    // int *p1 = p;
    // int *const p2 = p;
    // const int *p3 = p;

    // const int &r1 = 20;
    // const int &r2 = b;
    // int &r3 = a;
    // int c = r1;
    return 0;
}