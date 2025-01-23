#include <iostream>

void fun(int a){}
void fun(const int a){}

int func1(int a){return a;}
int (*func1ptr)(int) = &func1;

using F = int(int);

template <typename T>
void f(T param){
    std::cout<<param<<std::endl;
}

int main(){
    const F *aaa = func1;
    F *const bbb = func1;
    int a = 10;
    f(a);
    int *aptr = &a;
    f(aptr);
    int &aref = a;
    f(aref);

    
    return 0;
}