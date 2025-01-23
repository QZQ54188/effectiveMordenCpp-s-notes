#include <iostream>

void f(int){
    std::cout << "f(int)" << std::endl;
}


void f(bool){
    std::cout << "f(bool)" << std::endl;
}

void f(void *){
    std::cout << "f(void *)" << std::endl;
}

int main(){
    auto a = 0;
    // auto b = NULL;
    auto c = nullptr;

    f(0);
    // f(NULL);    //编译报错
    f(nullptr);
}