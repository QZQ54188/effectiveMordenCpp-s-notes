## item30:熟悉完美转发失败的情况

[item30原文](https://cntransgroup.github.io/EffectiveModernCppChinese/5.RRefMovSemPerfForw/item30.html)

“转发”仅表示将一个函数的形参传递——就是转发——给另一个函数。对于第二个函数（被传递的那个）目标是收到与第一个函数（执行传递的那个）完全相同的对象。这规则排除了按值传递的形参，因为它们是原始调用者传入内容的拷贝。我们希望被转发的函数能够使用最开始传进来的那些对象。指针形参也被排除在外，因为我们不想强迫调用者传入指针。

完美转发不仅表示我们需要传递对象，我们还需要传递对象的类型：是左值还是右值，是const还是volatile。结合到我们会处理引用形参，这意味着我们将使用通用引用，因为通用引用形参被传入实参时才确定是左值还是右值。

假定我们有一些函数f，然后想编写一个转发给它的函数（事实上是一个函数模板）：
```C++
template<typename T>
void fwd(T&& param)             //接受任何实参
{
    f(std::forward<T>(param));  //转发给f
}
```
从本质上说，转发函数是通用的。例如fwd模板，接受任何类型的实参，并转发得到的任何东西。这种通用性的逻辑扩展是，转发函数不仅是模板，而且是可变模板，因此可以接受任何数量的实参。fwd的可变形式如下：
```C++
template<typename... Ts>
void fwd(Ts&&... params)            //接受任何实参
{
    f(std::forward<Ts>(params)...); //转发给f
}
```

给定我们的目标函数f和转发函数fwd，如果f使用某特定实参会执行某个操作，但是fwd使用相同的实参会执行不同的操作，完美转发就会失败：
```C++
f( expression );        //调用f执行某个操作
fwd( expression );		//但调用fwd执行另一个操作，则fwd不能完美转发expression给f
```

### 花括号初始化器
考虑以下情况：
```C++
void f(const std::vector<int>& v);
f({ 1, 2, 3 });         //可以，“{1, 2, 3}”隐式转换为std::vector<int>
fwd({ 1, 2, 3 });       //错误！不能编译
```
在上面的例子中，从{ 1, 2, 3 }生成了临时std::vector<int>对象，因此f的形参v会绑定到std::vector<int>对象上。

当通过调用函数模板fwd间接调用f时，编译器不再把调用地传入给fwd的实参和f的声明中形参类型进行比较。而是推导传入给fwd的实参类型，然后比较推导后的实参类型和f的形参声明类型。当下面情况任何一个发生时，完美转发就会失败：
* 编译器不能推导出fwd的一个或者多个形参类型。 这种情况下代码无法编译。
* 编译器推导“错”了fwd的一个或者多个形参类型。 在这里，“错误”可能意味着fwd的实例将无法使用推导出的类型进行编译，但是也可能意味着使用fwd的推导类型调用f，与用传给fwd的实参直接调用f表现出不一致的行为。

在上述例子中，传参是使用花括号初始化的std::initializer_list，但是因为fwd的形参没有声明为std::initializer_list。对于fwd形参的推导类型被阻止，编译器只能拒绝该调用。

我们可以使用auto进行解决：
```C++
auto il = { 1, 2, 3 };  //il的类型被推导为std::initializer_list<int>
fwd(il);                //可以，完美转发il给f
```

### 0或者NULL作为空指针
[Item8](https://cntransgroup.github.io/EffectiveModernCppChinese/5.RRefMovSemPerfForw/item8.html)说明当你试图传递0或者NULL作为空指针给模板时，类型推导会出错，会把传来的实参推导为一个整型类型（典型情况为int）而不是指针类型。结果就是不管是0还是NULL都不能作为空指针被完美转发。解决方法非常简单，传一个nullptr而不是0或者NULL。

### 仅有声明的整型static const数据成员

```C++
class Widget {
public:
    static const std::size_t MinVals = 28;  //MinVal的声明
    …
};
…                                           //没有MinVals定义

std::vector<int> widgetData;
widgetData.reserve(Widget::MinVals);        //使用MinVals
```
这里，我们使用Widget::MinVals（或者简单点MinVals）来确定widgetData的初始容量，即使MinVals缺少定义。编译器通过将值28放入所有提到MinVals的位置来补充缺少的定义（就像它们被要求的那样）。没有为MinVals的值留存储空间是没有问题的。如果要使用MinVals的地址（例如，有人创建了指向MinVals的指针），则MinVals需要存储（这样指针才有可指向的东西），尽管上面的代码仍然可以编译，但是链接时就会报错，直到为MinVals提供定义。

假如fwd要转发参数的实际操作函数调用如下：
```C++
void f(std::size_t val);
//使用MinVals调用f是可以的，因为编译器直接将值28代替MinVals
f(Widget::MinVals);         //可以，视为“f(28)”
//但是如果直接使用fwd完美转发，会在链接处发生错误
fwd(Widget::MinVals);       //错误！不应该链接
```
尽管代码中没有使用MinVals的地址，但是fwd的形参是通用引用，而引用，在编译器生成的代码中，通常被视作指针。在程序的二进制底层代码中（以及硬件中）指针和引用是一样的。在这个水平上，引用只是可以自动解引用的指针。在这种情况下，通过引用传递MinVals实际上与通过指针传递MinVals是一样的，因此，必须有内存使得指针可以指向。通过引用传递的整型static const数据成员，通常需要定义它们，这个要求可能会造成在不使用完美转发的代码成功的地方，使用等效的完美转发失败。如果没有定义，完美转发就会失败。

### 重载函数的名称和模板名称
考虑如下函数f，它的作用是向其传递执行某些功能的函数来自定义其行为。假设这个函数接受和返回值都是int，f声明就像这样：
```C++
void f(int (*pf)(int));             //pf = “process function”
//也可以这么声明
void f(int pf(int));                //与上面定义相同的f
```

现在假设我们有了一个重载函数，processVal：
```C++
int processVal(int value);
int processVal(int value, int priority);
//我们可以传递processVal给f:
f(processVal);                      //可以
```
工作的基本机制是f的声明让编译器识别出哪个是需要的processVal。但是，fwd是一个函数模板，没有它可接受的类型的信息，使得编译器不可能决定出哪个函数应被传递。单用processVal是没有类型信息的，所以就不能类型推导，完美转发失败。

如果我们试图使用函数模板而不是（或者也加上）重载函数的名字，同样的问题也会发生。一个函数模板不代表单独一个函数，它表示一个函数族：
```C++
template<typename T>
T workOnVal(T param)                //处理值的模板
{ … }

fwd(workOnVal);                     //错误！哪个workOnVal实例？
```

要让像fwd的完美转发函数接受一个重载函数名或者模板名，方法是指定要转发的那个重载或者实例。比如，你可以创造与f相同形参类型的函数指针，通过processVal或者workOnVal实例化这个函数指针，然后传递指针给fwd：
```C++
using ProcessFuncType =                         //写个类型定义；见条款9
    int (*)(int);

ProcessFuncType processValPtr = processVal;     //指定所需的processVal签名

fwd(processValPtr);                             //可以
fwd(static_cast<ProcessFuncType>(workOnVal));   //也可以
```

**请记住：**
* 当模板类型推导失败或者推导出错误类型，完美转发会失败。
* 导致完美转发失败的实参种类有花括号初始化，作为空指针的0或者NULL，仅有声明的整型static const数据成员，模板和重载函数的名字，位域。
