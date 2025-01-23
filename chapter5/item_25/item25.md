## item25:对右值引用使用std::move，对通用引用使用std::forward

[item25原文](https://cntransgroup.github.io/EffectiveModernCppChinese/5.RRefMovSemPerfForw/item25.html)

当把右值引用转发给其他函数时，右值引用应该被无条件转换为右值（通过std::move），因为它们总是绑定到右值；当转发通用引用时，通用引用应该有条件地转换为右值（通过std::forward），因为它们只是有时绑定到右值。

我们应该避免对于右值引用使用std::forward，对通用引用使用std::move，这可能会意外改变左值（比如局部变量）：
```C++
class Widget {
public:
    template<typename T>
    void setName(T&& newName)       //通用引用可以编译，
    { name = std::move(newName); }  //但是代码太太太差了！
    …

private:
    std::string name;
    std::shared_ptr<SomeDataStructure> p;
};

std::string getWidgetName();        //工厂函数

Widget w;

auto n = getWidgetName();           //n是局部变量

w.setName(n);                       //把n移动进w！

…                                   //现在n的值未知
```
上面的例子，局部变量n被传递给w.setName，调用方可能认为这是对n的只读操作——这一点倒是可以被原谅。但是因为setName内部使用std::move无条件将传递的引用形参转换为右值，n的值被移动进w.name，调用setName返回时n最终变为未定义的值。

由于setname不会修改外部实参，所以我们可以使用const左值引用来接受参数，同时使用右值引用接受右值：
```C++
class Widget {
public:
    void setName(const std::string& newName)    //用const左值设置
    { name = newName; }
    
    void setName(std::string&& newName)         //用右值设置
    { name = std::move(newName); }
    
    …
};
```

这样的话，当然可以工作，但是有缺点。首先编写和维护的代码更多（两个函数而不是单个模板）；其次，效率下降。比如，考虑如下场景：
```C++
w.setName("Adela Novak");
```
使用通用引用的版本的setName，字面字符串“Adela Novak”可以被传递给setName，再传给w内部std::string的赋值运算符。w的name的数据成员通过字面字符串直接赋值，没有临时std::string对象被创建。但是，setName重载版本，会有一个临时std::string对象被创建，setName形参绑定到这个对象，然后这个临时std::string移动到w的数据成员中。一次setName的调用会包括std::string构造函数调用（创建中间对象），std::string赋值运算符调用（移动newName到w.name），std::string析构函数调用（析构中间对象）。这比调用接受const char*指针的std::string赋值运算符开销昂贵许多。增加的开销根据实现不同而不同，这些开销是否值得担心也跟应用和库的不同而有所不同，但是事实上，将通用引用模板替换成对左值引用和右值引用的一对函数重载在某些情况下会导致运行时的开销。

对于这种函数，对于左值和右值分别重载就不能考虑了：通用引用是仅有的实现方案：
```C++
template<class T, class... Args>                //来自C++11标准
shared_ptr<T> make_shared(Args&&... args);

template<class T, class... Args>                //来自C++14标准
unique_ptr<T> make_unique(Args&&... args);
```

在某些情况，你可能需要在一个函数中多次使用绑定到右值引用或者通用引用的对象，并且确保在完成其他操作前，这个对象不会被移动。这时，你只想在最后一次使用时，使用std::move（对右值引用）或者std::forward（对通用引用）。比如：
```C++
template<typename T>
void setSignText(T&& text)                  //text是通用引用
{
  sign.setText(text);                       //使用text但是不改变它
  
  auto now = 
      std::chrono::system_clock::now();     //获取现在的时间
  
  signHistory.add(now, 
                  std::forward<T>(text));   //有条件的转换为右值
}
```
这里，我们想要确保text的值不会被sign.setText改变，因为我们想要在signHistory.add中继续使用。因此std::forward只在最后使用。

如果你在按值返回的函数中，返回值绑定到右值引用或者通用引用上，需要对返回的引用使用std::move或者std::forward。
```C++
Matrix                              //按值返回
operator+(Matrix&& lhs, const Matrix& rhs)
{
    lhs += rhs;
    return std::move(lhs);	        //移动lhs到返回值中
}

Matrix                              //同之前一样
operator+(Matrix&& lhs, const Matrix& rhs)
{
    lhs += rhs;
    return lhs;                     //拷贝lhs到返回值中
}
```
通过在return中将其转化为右值，lhs可以移动到返回值中，但是如果省略了move，那么lhs仍是一个左值，会强制编译器拷贝他到返回值的内存空间，假定Matrix支持移动操作，并且比拷贝操作效率更高，在return语句中使用std::move的代码效率更高。

如果Matrix不支持移动操作，将其转换为右值不会变差，因为右值可以直接被Matrix的拷贝构造函数拷贝，如果Matrix随后支持了移动操作，operator+将在下一次编译时受益。就是这种情况，通过将std::move应用到按值返回的函数中要返回的右值引用上，不会损失什么。

使用通用引用和std::forward的情况类似。考虑函数模板reduceAndCopy收到一个未规约（unreduced）对象Fraction，将其规约，并返回一个规约后的副本。如果原始对象是右值，可以将其移动到返回值中（避免拷贝开销），但是如果原始对象是左值，必须创建副本，因此如下代码：
```C++
template<typename T>
Fraction                            //按值返回
reduceAndCopy(T&& frac)             //通用引用的形参
{
    frac.reduce();
    return std::forward<T>(frac);		//移动右值，或拷贝左值到返回值中
}
```

对于下面这段代码，使用move的方式将拷贝优化为移动的方式不好：
```C++
Widget makeWidget()                 //makeWidget的“拷贝”版本
{
    Widget w;                       //局部对象
    …                               //配置w
    return w;                       //“拷贝”w到返回值中
}

Widget makeWidget()                 //makeWidget的移动版本
{
    Widget w;
    …
    return std::move(w);            //移动w到返回值中（不要这样做！）
}
```
问题在哪？[参考视频](https://www.bilibili.com/video/BV15P411p7ri?t=0.0&p=2)

早就为人认识到的是，makeWidget的“拷贝”版本可以避免复制局部变量w的需要，通过在分配给函数返回值的内存中构造w来实现。这就是所谓的返回值优化（return value optimization，RVO），这在C++标准中已经实现了。

这意味着，如果对从按值返回的函数返回来的局部对象使用std::move，你并不能帮助编译器（如果不能实行拷贝消除的话，他们必须把局部对象看做右值），而是阻碍其执行优化选项（通过阻止RVO）。

**请记住：**
* 最后一次使用时，在右值引用上使用std::move，在通用引用上使用std::forward。
* 对按值返回的函数要返回的右值引用和通用引用，执行相同的操作。
* 如果局部对象可以被返回值优化消除，就绝不使用std::move或者std::forward。