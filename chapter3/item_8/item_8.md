## item8:优先考虑nullptr而不是NULL和0

[item8原文](https://cntransgroup.github.io/EffectiveModernCppChinese/3.MovingToModernCpp/item8.html)


首先明确nullptr，NULL和0分别是什么类型
```C++
    auto a = 0;         //int
    auto b = NULL;      //long
    auto c = nullptr;   //std::nullptr_t
```

相较于使用NULL和0，使用nullptr可以正确调用指针版本的函数重载
```C++
void f(int){
    std::cout << "f(int)" << std::endl;
}


void f(bool){
    std::cout << "f(bool)" << std::endl;
}

void f(void *){
    std::cout << "f(void *)" << std::endl;
}

    f(0);
    f(NULL);    //编译报错
    f(nullptr);
```

nullptr的优点是它不是整型。老实说它也不是一个指针类型，但是你可以把它**认为是所有类型的指针。**nullptr的真正类型是std::nullptr_t，在一个完美的循环定义以后，std::nullptr_t又被定义为nullptr。std::nullptr_t可以隐式转换为指向任何内置类型的指针，这也是为什么nullptr表现得像所有类型的指针。


使用nullptr可以避免阅读代码中的歧义。它也可以使代码表意明确，尤其是当涉及到与auto声明的变量一起使用时。
```C++
auto result = findRecord( /* arguments */ );
if (result == 0) {
    …
} 
```

如果你不知道findRecord返回了什么（或者不能轻易的找出），那么你就不太清楚到底result是一个指针类型还是一个整型。毕竟，0（用来测试result的值的那个）也可以像我们之前讨论的那样被解析。
```C++
auto result = findRecord( /* arguments */ );

if (result == nullptr) {  
    …
}
```

这段代码没有歧义，result一定是指针类型。

在模板类型推导中，nullptr同样很有作用，例如
```C++
template<typename FuncType,
         typename MuxType,
         typename PtrType>
auto lockAndCall(FuncType func,                 
                 MuxType& mutex,                 
                 PtrType ptr) -> decltype(func(ptr))
{
    MuxGuard g(mutex);  
    return func(ptr); 
}
```

在这段代码中，如果传入的ptr为0，那么ptr的PtrType将会被推导为int而不是指针，不会调用func的指针重载版本。如果传入的ptr为NULL，那么同理。

然而，使用nullptr是调用没什么问题。当nullptr传给lockAndCall时，ptr被推导为std::nullptr_t。当ptr被传递给函数的时候，隐式转换使std::nullptr_t转换为对应指针，因为std::nullptr_t可以隐式转换为任何指针类型。


**请记住：**
* 优先考虑nullptr而非0和NULL
* 避免重载指针和整型