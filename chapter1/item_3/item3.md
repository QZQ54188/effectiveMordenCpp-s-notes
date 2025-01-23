## item3:理解decltype类型推导

[item3原文](https://cntransgroup.github.io/EffectiveModernCppChinese/1.DeducingTypes/item3.html)

[参考blog(【深度C++】之“decltype”)](https://blog.csdn.net/u014609638/article/details/106987131)

1. decltype + 变量，所有的信息都会被保留，数组和函数也不会退化为指针

```C++
    int a = 10;
    decltype(a) b;      //b被推导为int
    int &aref = a;
    decltype(aref) bref = a;//bref被推导为int &
    const int &arefc = a;
    decltype(arefc) brefc = a;//brefc被推导为const int &
    const int *const aptr = &a;
    decltype(aptr) bptr = &a;//bptr被推导为const int const *
    const int &&carref = std::move(a);
    decltype(carref) cbrref;//cbrref被推导为const int &&
    int array[2] = {1, 2};
    decltype(array)arr;     //arr被推导为int [2]
```

2. decltype + 表达式，会返回表达式结果的类型(表达式不是左值就是右值)


**如果表达式是左值就得到该类型的左值引用，如果表达式是右值就得到该类型。为什么有这样的区别?**
因为decltype单独作用于对象，没有使用对象的表达式属性，而是直接得到该变量的类型，如果想使用对象的表达式属性，可以加括号

```C++
    int *aptr = &a;
    decltype(*aptr) b = a;//int&
    decltype(a) b1; //int
    decltype((a)) b2;   //int&

    int *const acptr = &a;
    decltype(*acptr) aaa;   //int &
    const int *captr = &a;
    decltype(*captr) bbb;   //const int &
    const int *const cacptr = &a;
    decltype(*cacptr) ccc;  //const int &
    //引用没有顶层const
    decltype(10) ddd;       //int

    decltype(auto) f1()
    {
        int x = 0;
        …
        return x;                            //decltype(x）是int，所以f1返回int
    }

    decltype(auto) f2()
    {
        int x = 0;
        return (x);                          //decltype((x))是int&，所以f2返回int&
    }
```
注意不仅f2的返回类型不同于f1，而且它还引用了一个局部变量！

3. decltype并不会实际计算表达式的值，编译器会分析表达式并且得到表达式类型。
函数调用也算一种表达式，因此不必担心在使用decltype时真正的执行了函数。当使用decltype(func_name)的形式时，decltype会返回对应的函数类型，不会自动转换成相应的函数指针。


4. decltype的使用场景

某些情况下，函数的返回值无法提前得到

对于std::vector<>的对象，使用operator[]通常会返回T&，但是对于std::vector<bool>却是一个例外(std::vector<bool>::reference value1)，所以此时的模板函数的返回值必须是自动推导。

```C++
template<typename Container, typename Index>    //可以工作，
auto testFun(Container& c, Index i)       //但是需要改良
    ->decltype(c[i])                        //C++11
{
    return c[i];
}

template<typename Container, typename Index>    //C++14版本，
decltype(auto)                                  //可以工作，
authAndAccess(Container& c, Index i)            //但是还需要
{                                               //改良
    return c[i];
}
```


### item3


```C++
const int i = 0;                //decltype(i)是const int

bool f(const Widget& w);        //decltype(w)是const Widget&
                                //decltype(f)是bool(const Widget&)

struct Point{
    int x,y;                    //decltype(Point::x)是int
};                              //decltype(Point::y)是int

Widget w;                       //decltype(w)是Widget

if (f(w))…                      //decltype(f(w))是bool

template<typename T>            //std::vector的简化版本
class vector{
public:
    …
    T& operator[](std::size_t index);
    …
};

vector<int> v;                  //decltype(v)是vector<int>
…
if (v[0] == 0)…                 //decltype(v[0])是int&
```


在C++11中，decltype最主要的用途就是用于声明函数模板，而这个函数返回类型依赖于形参类型。

假定我们写一个函数，一个形参为容器，一个形参为索引值，这个函数支持使用方括号的方式（也就是使用“[]”）访问容器中指定索引值的数据，然后在返回索引操作的结果前执行认证用户操作。函数的返回类型应该和索引操作返回的类型相同。(如何实现?)

使用decltype的初步实现
```C++
template<typename Container, typename Index>    //可以工作，
auto authAndAccess(Container& c, Index i)       //但是需要改良
    ->decltype(c[i])
{
    authenticateUser();
    return c[i];
}
```

函数名称前面的auto不会做任何的类型推导工作。相反的，他只是暗示使用了C++11的尾置返回类型语法，即在函数形参列表后面使用一个”->“符号指出函数的返回类型，尾置返回类型的好处是我们可以在函数返回类型中使用函数形参相关的信息。如果我们按照传统语法把函数返回类型放在函数名称之前，c和i就未被声明所以不能使用。


C++14改进
```C++
template<typename Container, typename Index>    //C++14版本，
auto authAndAccess(Container& c, Index i)       //不那么正确
{
    authenticateUser();
    return c[i];                                //从c[i]中推导返回类型
}
```

对于authAndAccess来说这意味着在C++14标准下我们可以忽略尾置返回类型，只留下一个auto。使用这种声明形式，auto标示这里会发生类型推导(走的模板推导那一套)。更准确的说，编译器将会从函数实现中推导出函数的返回类型。

正如我们之前讨论的，operator[]对于大多数T类型的容器会返回一个T&，但是Item1解释了在模板类型推导期间，表达式的引用性（reference-ness）会被忽略。基于这样的规则，考虑它会对下面用户的代码有哪些影响：

```C++
std::deque<int> d;
…
authAndAccess(d, 5) = 10;               //认证用户，返回d[5]，
                                        //然后把10赋值给它
                                        //无法通过编译！
```

在这里d[5]本该返回一个int&，**但是模板类型推导会剥去引用的部分，因此产生了int返回类型。**函数返回的那个int是一个右值，上面的代码尝试把10赋值给右值int，C++11禁止这样做，所以代码无法编译。


我们需要使用decltype类型推导来推导它的返回值，即指定authAndAccess应该返回一个和c[i]表达式类型一样的类型。C++14通过使用decltype(auto)说明符使得这成为可能。我们第一次看见decltype(auto)可能觉得非常的矛盾（到底是decltype还是auto？），实际上我们可以这样解释它的意义：auto说明符表示这个类型将会被推导，decltype说明decltype的规则将会被用到这个推导过程中。


```C++
template<typename Container, typename Index>    //C++14版本，
decltype(auto)                                  //可以工作，
authAndAccess(Container& c, Index i)            //但是还需要
{                                               //改良
    authenticateUser();
    return c[i];
}
```


decltype(auto)的使用不仅仅局限于函数返回类型，当你想对初始化表达式使用decltype推导的规则，你也可以使用：
```C++
Widget w;

const Widget& cw = w;

auto myWidget1 = cw;                    //auto类型推导
                                        //myWidget1的类型为Widget
decltype(auto) myWidget2 = cw;          //decltype类型推导
                                        //myWidget2的类型是const Widget&
```


C++14中authAndAccess的缺点

容器通过传引用的方式传递非常量左值引用（lvalue-reference-to-non-const），因为返回一个引用允许用户可以修改容器。但是这意味着在不能给这个函数传递右值容器，右值不能被绑定到左值引用上（除非这个左值引用是一个const（lvalue-references-to-const），但是这里明显不是）。


一个右值容器，是一个临时对象，通常会在authAndAccess调用结束被销毁，这意味着authAndAccess返回的引用将会成为一个悬置的（dangle）引用。但是使用向authAndAccess传递一个临时变量也并不是没有意义，有时候用户可能只是想简单的获得临时容器中的一个元素的拷贝，比如这样：
```C++
std::deque<std::string> makeStringDeque();      //工厂函数

//从makeStringDeque中获得第五个元素的拷贝并返回
auto s = authAndAccess(makeStringDeque(), 5);
```


我们可以使用重载或者通用引用解决问题，在这里，因为函数重载需要维护两个函数，我们选择使用通用引用
```C++
template<typename Container, typename Index>    //最终的C++14版本
decltype(auto)
authAndAccess(Container&& c, Index i)
{
    authenticateUser();
    return std::forward<Container>(c)[i];
}
```

**请记住**
* decltype总是不加修改的产生变量或者表达式的类型。
* 对于T类型的不是单纯的变量名的左值表达式，decltype总是产出T的引用即T&。
* C++14支持decltype(auto)，就像auto一样，推导出类型，但是它使用decltype的规则进行推导。