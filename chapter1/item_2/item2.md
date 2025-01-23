## 理解auto类型推导
[item2原文](https://cntransgroup.github.io/EffectiveModernCppChinese/1.DeducingTypes/item2.html)
item1中模板类型推导使用的函数模板
```C++
template<typename T>
void f(ParmaType param);
```
当一个变量使用auto进行声明时，auto扮演了模板中T的角色，变量的类型说明符扮演了ParamType的角色。
```C++
auto x = 27;
const auto cx = x;
const auto &rx = x;
```

auto类型推导除了一个例外，其他情况都和模板类型推导一样。
```C++
const char name[] =             //name的类型是const char[13]
 "R. N. Briggs";

auto arr1 = name;               //arr1的类型是const char*
auto& arr2 = name;              //arr2的类型是const char (&)[13]

void someFunc(int, double);     //someFunc是一个函数，
                                //类型为void(int, double)

auto func1 = someFunc;          //func1的类型是void (*)(int, double)
auto& func2 = someFunc;         //func2的类型是void (&)(int, double)
```

### 不同点
```C++
int x1 = 27;    //int
int x2(27);     //int
int x3 = {27};  //类型是std::initializer_list<int>，
int x4{27};     //int
```

这就造成了auto类型推导不同于模板类型推导的特殊情况。当用auto声明的变量使用花括号进行初始化，auto类型推导推出的类型则为std::initializer_list。

如果这样的一个类型不能被成功推导（比如花括号里面包含的是不同类型的变量），编译器会拒绝这样的代码：
```C++
auto x5 = { 1, 2, 3.0 };        //错误！无法推导std::initializer_list<T>中的T
```
对我们来说认识到这里确实发生了两种类型推导是很重要的。一种是由于auto的使用：x5的类型不得不被推导。因为x5使用花括号的方式进行初始化，x5必须被推导为std::initializer_list。但是std::initializer_list是一个模板。std::initializer_list<T>会被某种类型T实例化，所以这意味着T也会被推导。 推导落入了这里发生的第二种类型推导——模板类型推导的范围。在这个例子中推导之所以失败，是因为在花括号中的值并不是同一种类型。


当使用auto声明的变量使用花括号的语法进行初始化的时候，会推导出std::initializer_list<T>的实例化，但是对于模板类型推导这样就行不通。然而如果在模板中指定T是std::initializer_list<T>而留下未知T,模板类型推导就能正常工作：
```C++
auto x = { 11, 23, 9 };         //x的类型是std::initializer_list<int>

template<typename T>            //带有与x的声明等价的
void f(T param);                //形参声明的模板

f({ 11, 23, 9 });               //错误！不能推导出T

template<typename T>
void f(std::initializer_list<T> initList);

f({ 11, 23, 9 });               //T被推导为int，initList的类型为
                                //std::initializer_list<int>
```

**因此auto类型推导和模板类型推导的真正区别在于，auto类型推导假定花括号表示std::initializer_list而模板类型推导不会这样**

C++14允许auto用于函数返回值并会被推导，而且C++14的lambda函数也允许在形参声明中使用auto。但是在这些情况下auto实际上使用模板类型推导的那一套规则在工作，而不是auto类型推导，所以说下面这样的代码不会通过编译：
```C++
auto createInitList()
{
    return { 1, 2, 3 };         //错误！不能推导{ 1, 2, 3 }的类型
}

std::vector<int> v;
…
auto resetV = 
    [&v](const auto& newValue){ v = newValue; };        //C++14
…
resetV({ 1, 2, 3 });            //错误！不能推导{ 1, 2, 3 }的类型
```