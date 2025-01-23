## 区别使用()和{}创建对象
[item7原文](https://cntransgroup.github.io/EffectiveModernCppChinese/3.MovingToModernCpp/item7.html)
### {}的引入

```C++
struct A{
    A(int a){
        std::cout << "A(int a)" <<std::endl;
    }
    A(const A &a){
        std::cout<<"A(const int &a)"<<std::endl;
    }
}

int main(){
    A a = 10;   //一次构造一次拷贝
    A b(10);    //一次构造
    A c = (10); //一次构造一次拷贝
    A d{10};    //一次构造
    A e = {20}; //一次构造
    return 0;
}
```
* 由此可见赋值时采用=或者=()会额外产生一次拷贝，是效率很低的操作
> 《effective morden cpp》:在这个条款的剩下部分，我通常会忽略"="和花括号组合初始化的语法，因为C++通常把它视作和只有花括号一样。**(在C++14之后不一样)**


```C++
//在上述结构体A中新加入的构造函数
A(int a, int b){
    std::cout<<"A(int a, int b)"<<std::endl;
}

int main(){
    A f = (10,5);
    A g(10, 5);
    A h{10, 5};
    A i = {10, 5};
}
```
* **A a = 10和A f = (10, 5)问题**:前者只能接受一个参数，而且二者都会发生一次构造和一次拷贝
* **A b(10)和A g(10, 5)问题**:被用作函数形参或者返回值时还是会发生拷贝
    ```C++
    void func1(A arg){
    
    }
    A func2(){
        A a(10, 5);
        return a;
    }
    int main(){
        func1(A(10, 5));    //一次构造(临时对象创建)一次拷贝(作为函数参数)
        A q = func2();      //一次构造(函数内创建对象)两次拷贝(函数返回给临时右值)(临时右值返回给q)
    }
    ```
* **A a{10}和A h{10, 5}的优势**:{ }可以完美解决上面的问题，而且不允许缩窄转化。大大简化了聚合类的初始化。{ }对C++最头疼的解析问题天生免疫
    >ChatGpt：在 C++ 中，缩窄转换（Narrowing Conversion）指的是将一个较大类型的值转换为一个较小类型的值，可能会导致数据丢失或精度损失。例如，将 double 类型的值转换为 int 类型，或者将 long 转换为 short，如果原值超出了目标类型的表示范围，就会丢失部分信息。在这种情况下，double 到 int 的转换会丢失小数部分。C++11 引入了 禁止缩窄转换 的特性，尤其是在列表初始化（例如 std::initializer_list）中，如果发生缩窄转换，编译器会报错。
    ```C++
    A j = {10, 5};
    func1({10, 5});//只发生一次构造，因为这样写就相当于 A arg = {10, 5}
    func1(A{10, 5});//发生一次构造和一次拷贝，因为这样相当于A arg = A{10, 5}

    A func2(){
        // A a{10, 5};
        // return a;
        return {10, 5};
    }
    A i = func2();//一次构造(返回的临时右值={10, 5})一次拷贝(临时右值拷贝给i),若将func2注释内容替换return {10, 5},则发生一次构造两次拷贝
    ```

<mark>上述拷贝构造大多可以使用移动构造优化</mark>

#### 聚合类的定义
C++11:
1. 所有成员都是public
2. 没有类内初始化函数(基本类型随机初始化，即未定义)，C++17取消
3. 没有定义任何构造函数
4. 没有基类和虚函数

C++17:
* 可以有基类，但是必须是公有继承，且必须是非虚继承(基类可以不是聚合类)
```C++
class MystringWithIndex : public std::string{
    public: 
        int index = 0;
};
MystringWithIndex S {{"hello"}, 1};
```

#### 解析问题
```C++
void f(double value){
    int i(int(value));
}
```
这个函数有两种解析方式:
1. 视为声明变量，将传入的value从double转化为int，然后再将转化后的int变量的值赋值给int类型变量i
2. 声明一个函数int i(int value)，因为C++由C语言而来，C语言中允许变量被多余的括号包裹

```C++
struct timer{

};
struct timerKeeper{
    timerKeeper(timer T){};
};
timerKeeper timer_keeper(timer());  //同样被解析为函数,参数类型时函数退化的没有参数的，返回值为timer的函数指针
```

#### std::array
* 类型安全：std::array 在编译时提供类型和大小的检查，减少错误。
* 更丰富的接口：提供诸如 size()、at()、begin()、end() 等函数，使得使用更方便、安全。
* 与STL容器兼容：可以与其他 STL 容器和算法无缝配合使用。
* 性能优化：内存分配和管理由编译器处理，且不引入 std::vector 等容器的额外开销。


```C++
struct S{
    int x;
    int y;
};

S a1[3] = {{1,2}, {3,4}, {5,6}};
S a2[3] = {1,2,3,4,5,6};

std::array<S, 3> a3 = {{1,2}, {3,4}, {5,6}};    //报错
std::array<S, 3> a3 = {{{1,2}, {3,4}, {5,6}}};  //不报错
```

**reason:** std::array在自己内部维护了一个原生数组，所以需要再加一对{ }：
```C++
template <typename T, size_t N>
struct MyArray{
    T data[N];
}
```

#### 如何让一个容器支持列表初始化
定义列表初始化函数，以A为例
```C++
A(std::initializer_list<int> a){
    std::cout << "A(std::initializer_list<int> a)"<<std::endl;
    for(const int *item = a.begin(); item != a.end(); item++){
    }
}
```

1. 隐式缩窄转化
2. 除非万不得已，否则总是优先匹配列表初始化，甚至报错
    ```C++
    A(std::initializer_list<int> a){
        std::cout << "A(std::initializer_list<int> a)"<<std::endl;
        for(const int *item = a.begin(); item != a.end(); item++){

        }
    }
    A(int a, float b){
        std::cout<<"A(int a, int b)"<<std::endl;
    }
    A(int a, std::string b){
        std::cout<<"A(int a, std::string b)"<<std::endl;
    }
    A bbb{1, 2.f};  //报错，匹配的时列表初始化
    A ccc{1, "qzq"};//不报错，因为没有将std::string隐式转化为int的方法
    ```

### item7
```C++
Widget w1;              //调用默认构造函数

Widget w2 = w1;         //不是赋值运算，调用拷贝构造函数

w1 = w2;                //是赋值运算，调用拷贝赋值运算符（copy operator=）
```

不可拷贝的对象不可以使用“=”初始化，可以使用{ }和( )初始化
```C++
std::atomic<int> ai1{ 0 };      //没问题
std::atomic<int> ai2(0);        //没问题
std::atomic<int> ai3 = 0;       //错误！
```

C++规定任何可以被解析为一个声明的东西必须被解析为声明。这个规则的副作用是让很多程序员备受折磨：他们可能想创建一个使用默认构造函数构造的对象，却不小心变成了函数声明。问题的根源是如果你调用带参构造函数，你可以这样做：
```C++
Widget w1(10);                  //使用实参10调用Widget的一个构造函数
Widget w2();                    //最令人头疼的解析！声明一个函数w2，返回Widget
Widget w3{};                    //调用没有参数的构造函数构造对象
```

如果你想用空std::initializer来调用std::initializer_list构造函数，你就得创建一个空花括号作为函数实参——把空花括号放在圆括号或者另一个花括号内来界定你想传递的东西。
```C++
Widget w4({});                  //使用空花括号列表调用std::initializer_list构造函数
Widget w5{{}};                  //同上
```

std::vector有一个非std::initializer_list构造函数允许你去指定容器的初始大小，以及使用一个值填满你的容器。但它也有一个std::initializer_list构造函数允许你使用花括号里面的值初始化容器。
```C++
std::vector<int> v1(10, 20);    //使用非std::initializer_list构造函数
                                //创建一个包含10个元素的std::vector，
                                //所有的元素的值都是20
std::vector<int> v2{10, 20};    //使用std::initializer_list构造函数
                                //创建包含两个元素的std::vector，
                                //元素的值为10和20
```