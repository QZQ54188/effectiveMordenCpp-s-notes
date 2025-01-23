## item28:理解引用折叠

[item28原文](https://cntransgroup.github.io/EffectiveModernCppChinese/5.RRefMovSemPerfForw/item28.html)

考虑一下函数模板：
```C++
template<typename T>
void func(T&& param);
```
当左值实参被传入时，T被推导为左值引用(int &)。当右值被传入时，T被推导为非引用(int)。
```C++
Widget widgetFactory();     //返回右值的函数
Widget w;                   //一个变量（左值）
func(w);                    //用左值调用func；T被推导为Widget&
func(widgetFactory());      //用右值调用func；T被推导为Widget
```

考虑下，如果一个左值传给接受通用引用的模板函数会发生什么：
```C++
template<typename T>
void func(T&& param);       //同之前一样

func(w);                    //用左值调用func；T被推导为Widget&
```
如果我们用T推导出来的类型（即Widget&）初始化模板，会得到：
```C++
void func(Widget& && param);
//编译器如何将上述函数签名转化为如下所示
void func(Widget& param);
```
此时发生了引用折叠，虽然编译器禁止声明引用的引用，但是编译器会在特定的上下文中产生这些，模板实例化就是其中一种情况。当编译器生成引用的引用时，引用折叠指导下一步发生什么。

存在两种类型的引用（左值和右值），所以有四种可能的引用组合（左值的左值，左值的右值，右值的右值，右值的左值）。

>如果任一引用为左值引用，则结果为左值引用。否则（即，如果引用都是右值引用），结果为右值引用。

在我们上面的例子中，将推导类型Widget&替换进模板func会产生对左值引用的右值引用，然后引用折叠规则告诉我们结果就是左值引用。

引用折叠是std::forward工作的一种关键机制。std::forward通常配合通用引用使用：
```C++
template<typename T>
void f(T&& fParam)
{
    …                                   //做些工作
    someFunc(std::forward<T>(fParam));  //转发fParam到someFunc
}
```

std::forward可以这样实现：
```C++
template<typename T>                                //在std命名空间
T&& forward(typename
                remove_reference<T>::type& param)
{
    return static_cast<T&&>(param);
}
```
假设传入到f的实参是Widget的左值类型。T被推导为Widget&，然后调用std::forward将实例化为std::forward<Widget&>。Widget&带入到上面的std::forward的实现中：
```C++
Widget& && forward(typename 
                       remove_reference<Widget&>::type& param)
{ return static_cast<Widget& &&>(param); }
```

std::remove_reference<Widget&>::type这个type trait产生Widget（查看Item9），所以std::forward成为：
```C++
Widget& && forward(Widget& param)
{ return static_cast<Widget& &&>(param); }
```

根据引用折叠规则，返回值和强制转换可以化简，最终版本的std::forward调用就是：
```C++
Widget& forward(Widget& param)
{ return static_cast<Widget&>(param); }
```

现在假设一下，传递给f的实参是一个Widget的右值。在这个例子中，f的类型参数T的推导类型就是Widget。f内部的std::forward调用因此为std::forward<Widget>，std::forward实现中把T换为Widget得到：
```C++
Widget&& forward(typename
                     remove_reference<Widget>::type& param)
{ return static_cast<Widget&&>(param); }
```

将std::remove_reference引用到非引用类型Widget上还是相同的类型（Widget），所以std::forward变成：
```C++
Widget&& forward(Widget& param)
{ return static_cast<Widget&&>(param); }
```

上述函数传参是左值与右值的情况均符合std::forward的语义。

在C++14中，std::remove_reference_t的存在使得实现变得更简洁：
```C++
template<typename T>                        //C++14；仍然在std命名空间
T&& forward(remove_reference_t<T>& param)
{
  return static_cast<T&&>(param);
}
```

引用折叠发生在四种情况下。第一，也是最常见的就是模板实例化。第二，是auto变量的类型生成，具体细节类似于模板，因为auto变量的类型推导基本与模板类型推导雷同：
```C++
Widget widgetFactory();     //返回右值的函数
Widget w;                   //一个变量（左值）
func(w);                    //用左值调用func；T被推导为Widget&
func(widgetFactory());      //用右值调用func；T被推导为Widget
//在auto的写法中，规则是类似的。
auto&& w1 = w;
```

用一个左值初始化w1，因此为auto推导出类型Widget&。把Widget&代回w1声明中的auto里，产生了引用的引用：
```C++
Widget& && w1 = w;
//应用引用折叠规则，就是
Widget& w1 = w;   //结果就是w1是一个左值引用。
```

另一方面，这个声明：
```C++
auto&& w2 = widgetFactory();
```

使用右值初始化w2，为auto推导出非引用类型Widget。把Widget代入auto得到：
```C++
Widget&& w2 = widgetFactory();
//没有引用的引用，这就是最终结果，w2是个右值引用。
```

对此，我们可以对通用引用有更加深刻的理解：
* 类型推导区分左值和右值。T类型的左值被推导为T&类型，T类型的右值被推导为T。
* 发生引用折叠。

**请记住：**
* 引用折叠发生在四种情况下：模板实例化，auto类型推导，typedef与别名声明的创建和使用，decltype。
* 当编译器在引用折叠环境中生成了引用的引用时，结果就是单个引用。有左值引用折叠结果就是左值引用，否则就是右值引用。
* 通用引用就是在特定上下文的右值引用，上下文是通过类型推导区分左值还是右值，并且发生引用折叠的那些地方。