#include <vector>
#include <string>
#include <array>
#include <iostream>

struct people{
    int age;
    std::string name;
    float height;
};

struct A
{
    A(){
        std::cout<<"A()"<<std::endl;
    }

    A(int a){
        std::cout << "A(int a)" <<std::endl;
    }

    A(const A &a){
        std::cout<<"A(const int &a)"<<std::endl;
    }

    A(int a, int b){
        std::cout<<"A(int a, int b)"<<std::endl;
    }
    A(std::initializer_list<int> a){
        std::cout << "A(std::initializer_list<int> a)"<<std::endl;
        for(const int *item = a.begin(); item != a.end(); item++){

        }
    }
    A(int a, float b){
        std::cout<<"A(int a, int b)"<<std::endl;
    }
};

void func1(A arg){
    
}

A func2(){
    // A a{10, 5};
    // return a;
    return {10, 5};
}

void f(double value){
    int i(int(value));
}

struct S{
    int x;
    int y;
};

int main(){
    // A a = 10;   //一次构造一次拷贝
    // A b(10);    //一次构造
    // A c = (10); //一次构造一次拷贝
    // A d{10};    //一次构造
    // A e = {20}; //一次构造

    // A f = (10,5);
    // A g(10, 5);
    // A h{10, 5};
    // func1(A(10, 5));
    // A i = func2();
    // people aaa = {1, "sz", 12.f};
    // A j = {10, 5};
    // func1({10, 5});

    S a1[3] = {{1,2}, {3,4}, {5,6}};
    S a2[3] = {1,2,3,4,5,6};

    // std::array<S, 3> a3 = {{1,2}, {3,4}, {5,6}};
    std::array<S, 3> a3 = {{{1,2}, {3,4}, {5,6}}};

    // A bbb{1, 2.f};

    return 0;
}
