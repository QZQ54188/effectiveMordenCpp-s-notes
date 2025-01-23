## item5:优先考虑auto而非显式类型声明

[item5原文](https://cntransgroup.github.io/EffectiveModernCppChinese/2.Auto/item5.html)

### 前提知识

#### lambda表达式

```C++
size_t sz = 10;
auto SizeComp = [sz](const string &a) {return a.size() > sz};
```

编译器将上述lambda表达式转化为可调用的类，即在类中重载operator(), 使其像函数调用一样
```C++
class SizeComp{
public:
    SizeComp(size_t n) : sz(n) {};
    bool operator() (const std::string &s) const 
    {
        return s.size() > sz;
    }

private:
    size_t sz;  //参数根据lambda表达式中捕获列表的传参形式而定(传引用还是传值)
};
```

为什么operator() 是 const：
* 由于 lambda 捕获 sz 是按值捕获的，因此 sz 是不可修改的成员变量。编译器会推断 operator() 作为 const，意味着它不会修改任何捕获的变量（特别是按值捕获的变量）。
* C++ lambda 表达式的设计默认确保捕获的变量在 operator() 内部不会被修改，因此编译器会自动推断并加上 const 修饰符。

在类中，如果数据成员中定义了引用，那么其初始化必须在构造函数的成员初始化列表中
**原因**
* 一旦引用被初始化，它就必须绑定到一个有效的对象，并且之后不能再更改引用指向其他对象。
* 成员初始化列表用于在对象的构造过程中为数据成员提供初值。它发生在构造函数的主体执行之前。
* 构造函数体中的代码则是在成员初始化完成后才执行的。如果你在构造函数体内给引用成员赋值，实际上是试图修改一个已经绑定到某个对象的引用，而这在C++中是不允许的，因为引用一旦绑定后就不能重新绑定。


#### lambda表达式语法
```C++
[captures](params)specifies exception -> ret{body}
```

1. {body}:和普通函数一样
2. exception可选异常说明符：可以使用noexcept指明函数会不会抛出异常
3. specifies可选限定符：默认是const，相当于类中的const函数，如果想修改捕获列表中的参数的话，可以使用mutable
4. ->ret可选返回值类型：大部分情况下可以自己推导，但是初始化列表不行
5. (params)可选参数列表：从C++14之后可以使用auto进行推导
```C++
auto foo = [](auto a){return a};
int one = foo(1);
```
6. [captures]参数捕获列表：
* 只可以捕获非静态的局部变量，可以按照值或者引用传递。它们的生命周期跨越多个函数调用，因此 lambda 不需要显式地捕获它们。静态变量在整个程序生命周期内存在，因此 lambda 可以直接访问它们
* 捕获发生在lambda表达式定义的时候，而不是使用的时候。因为lambda表达式定义时就相当于构造一个可调用类
* C++14之后有广义捕获，至此捕获列表可以传右值[r = std::move(...)]
* 特殊的捕获方法

[this]\:捕获this指针，使得我们可以使用this类内的成员变量以及函数

[=]\:捕获所有局部变量的值，包括this指针

[&]\:捕获所有局部变量的引用，包括this指针

[*this]\:捕获this指向对象的副本(since C++17)


### item5

对一个局部变量使用解引用迭代器的方式初始化：
```C++
template<typename It>           //对从b到e的所有元素使用
void dwim(It b, It e)           //dwim（“do what I mean”）算法
{
    while (b != e) {
        typename std::iterator_traits<It>::value_type currValue = *b;
        …
    }
}
```

typename std::iterator_traits<It>::value_type 表示迭代器 It 所指向元素的类型。

auto变量从初始化表达式中推导出类型，所以我们必须初始化。
```C++
int x1;                         //潜在的未初始化的变量
    
auto x2;                        //错误！必须要初始化

auto x3 = 0;                    //没问题，x已经定义了
```

auto还可以表示一些只有编译器才知道的变量，例如声明lambda表达式的时候编译器将其隐式转化的可调用的类
```C++
auto derefUPLess = 
    [](const std::unique_ptr<Widget> &p1,       //用于std::unique_ptr
       const std::unique_ptr<Widget> &p2)       //指向的Widget类型的
    { return *p1 < *p2; };                      //比较函数

auto derefLess =                                //C++14版本
    [](const auto& p1,                          //被任何像指针一样的东西
       const auto& p2)                          //指向的值的比较函数
    { return *p1 < *p2; };
```


尽管这很酷，但是你可能会想我们完全不需要使用auto声明局部变量来保存一个闭包，因为我们可以使用std::function对象。
>std::function是一个C++11标准模板库中的一个模板，它泛化了函数指针的概念。与函数指针只能指向函数不同，std::function可以指向任何可调用对象，也就是那些像函数一样能进行调用的东西。当你声明函数指针时你必须指定函数类型（即函数签名），同样当你创建std::function对象时你也需要提供函数签名，由于它是一个模板所以你需要在它的模板参数里面提供。

```C++
bool(const std::unique_ptr<Widget> &,           //C++11
     const std::unique_ptr<Widget> &)           //std::unique_ptr<Widget>
                                                //比较函数的签名

std::function<bool(const std::unique_ptr<Widget> &,
                   const std::unique_ptr<Widget> &)> func;
```


因为lambda表达式能产生一个可调用对象，所以我们现在可以把闭包存放到std::function对象中。这意味着我们可以不使用auto写出C++11版的derefUPLess
```C++
std::function<bool(const std::unique_ptr<Widget> &,
                   const std::unique_ptr<Widget> &)>
derefUPLess = [](const std::unique_ptr<Widget> &p1,
                 const std::unique_ptr<Widget> &p2)
                { return *p1 < *p2; };
```

使用std::function和auto的对比：
* std::function方法比auto方法要更耗空间且更慢，还可能有out-of-memory异常。并且正如上面的例子，比起写std::function实例化的类型来，使用auto要方便得多。


auto还可以避免类型快捷方式的问题
```C++
std::vector<int> v;
…
unsigned sz = v.size();
```
>v.size()的标准返回类型是std::vector<int>::size_type，但是只有少数开发者意识到这点。std::vector<int>::size_type实际上被指定为无符号整型，所以很多人都认为用unsigned就足够了，写下了上述的代码。这会造成一些有趣的结果。举个例子，在Windows 32-bit上std::vector<int>::size_type和unsigned是一样的大小，但是在Windows 64-bit上std::vector<int>::size_type是64位，unsigned是32位。这意味着这段代码在Windows 32-bit上正常工作，但是当把应用程序移植到Windows 64-bit上时就可能会出现一些问题。

使用auto自动推导可以避免这些问题
```C++
auto sz =v.size();                      //sz的类型是std::vector<int>::size_type
```

使用auto还可以避免参数修饰符不一样而产生的拷贝
```C++
std::unordered_map<std::string, int> m;
…

for(const std::pair<std::string, int>& p : m)
{
    …                                   //用p做一些事
}
```
其中std::pair的类型不是std::pair<std::string, int>，而是std::pair<const std::string, int>。编译器会努力的找到一种方法把std::pair<const std::string, int>（即hash table中的东西）转换为std::pair<std::string, int>（p的声明类型）。它会通过拷贝m中的对象创建一个临时对象，这个临时对象的类型是p想绑定到的对象的类型，即m中元素的类型，然后把p的引用绑定到这个临时对象上。在每个循环迭代结束时，临时对象将会销毁。性能浪费巨大！

但是使用auto可以很好的避免上面的问题
```C++
for(const auto& p : m)
{
    …                                   //如之前一样
}
```
这样无疑更具效率，且更容易书写。而且，这个代码有一个非常吸引人的特性，如果你获取p的地址，你确实会得到一个指向m中元素的指针。在没有auto的版本中p会指向一个临时变量，这个临时变量在每次迭代完成时会被销毁。

然而auto也不是完美的。每个auto变量都从初始化表达式中推导类型，有一些表达式的类型和我们期望的大相径庭。因为auto走的是模板类型推导。

事实是显式指定类型通常只会引入一些微妙的错误，无论是在正确性还是效率方面。而且，如果初始化表达式的类型改变，则auto推导出的类型也会改变，这意味着使用auto可以帮助我们完成一些重构工作。举个例子，如果一个函数返回类型被声明为int，但是后来你认为将它声明为long会更好，调用它作为初始化表达式的变量会自动改变类型，但是如果你不使用auto你就不得不在源代码中挨个找到调用地点然后修改它们。


**请记住：**
* auto变量必须初始化，通常它可以避免一些移植性和效率性的问题，也使得重构更方便，还能让你少打几个字。
* auto类型推导在有些时候可能与我们想要的类型不一样