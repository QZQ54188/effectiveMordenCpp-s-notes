## item22:当使用Pimpl惯用法，请在实现文件中定义特殊成员函数

[item22原文](https://cntransgroup.github.io/EffectiveModernCppChinese/4.SmartPointers/item22.html)

凭借Pimpl惯用法，你可以将类数据成员替换成一个指向包含具体实现的类（或结构体）的指针，并将放在主类（primary class）的数据成员们移动到实现类（implementation class）去，而这些数据成员的访问将通过指针间接访问。 

举例，假如有一个Widget类看起来如下：
```C++
class Widget() {                    //定义在头文件“widget.h”
public:
    Widget();
    …
private:
    std::string name;
    std::vector<double> data;
    Gadget g1, g2, g3;              //Gadget是用户自定义的类型
};
```

因为类Widget的数据成员包含有类型std::string，std::vector和Gadget， 定义有这些类型的头文件在类Widget编译的时候，必须被包含进来，这意味着类Widget的使用者必须要#include <string>，<vector>以及gadget.h。 这些头文件将会增加类Widget使用者的编译时间，并且让这些使用者依赖于这些头文件。 如果一个头文件的内容变了，类Widget使用者也必须要重新编译。 标准库文件<string>和<vector>不是很常变，但是gadget.h可能会经常修订。

在C++98中使用Pimpl惯用法，可以把Widget的数据成员替换成一个原始指针，指向一个已经被声明过却还未被定义的结构体：
```C++
class Widget                        //仍然在“widget.h”中
{
public:
    Widget();
    ~Widget();                      //析构函数在后面会分析
    …

private:
    struct Impl;                    //声明一个 实现结构体
    Impl *pImpl;                    //以及指向它的指针
};
```

此时，Widget类中不再有td::string，std::vector以及Gadget，Widget的使用者不再需要为了这些类型而引入头文件。这可以加速编译，并且意味着，如果这些头文件中有所变动，Widget的使用者不会受到影响。

其中Impl结构体已经被声明，但是还没有实现，是一个不完整类型。这种类型可以做的事情很有限，但是声明一个指向它的指针是可以的。Pimpl惯用法利用了这一点。

Pimpl惯用法的第一步，是声明一个数据成员，它是个指针，指向一个不完整类型。 第二步是动态分配和回收一个对象，该对象包含那些以前在原来的类中的数据成员。 内存分配和回收的代码都写在实现文件里，比如，对于类Widget而言，写在Widget.cpp里:
```C++
#include "widget.h"             //以下代码均在实现文件“widget.cpp”里
#include "gadget.h"
#include <string>
#include <vector>

struct Widget::Impl {           //含有之前在Widget中的数据成员的
    std::string name;           //Widget::Impl类型的定义
    std::vector<double> data;
    Gadget g1,g2,g3;
};

Widget::Widget()                //为此Widget对象分配数据成员
: pImpl(new Impl)
{}

Widget::~Widget()               //销毁数据成员
{ delete pImpl; }
```
对于std::string，std::vector和Gadget的头文件的整体依赖依然存在。 然而，这些依赖从头文件widget.h（它被所有Widget类的使用者包含，并且对他们可见）移动到了widget.cpp（该文件只被Widget类的实现者包含，并只对他可见）。我们可以使用智能指针代替原始指针。
```C++
class Widget {                      //在“widget.h”中
public:
    Widget();
    …

private:
    struct Impl;
    std::unique_ptr<Impl> pImpl;    //使用智能指针而不是原始指针
};
```

实现文件：
```C++
#include "widget.h"                 //在“widget.cpp”中
#include "gadget.h"
#include <string>
#include <vector>

struct Widget::Impl {               //跟之前一样
    std::string name;
    std::vector<double> data;
    Gadget g1,g2,g3;
};

Widget::Widget()                    //根据条款21，通过std::make_unique
: pImpl(std::make_unique<Impl>())   //来创建std::unique_ptr
{}
```
Widget的析构函数不存在了。这是因为我们没有代码加在里面了。 std::unique_ptr在自身析构时，会自动销毁它所指向的对象，所以我们自己无需手动销毁任何东西。这就是智能指针的众多优点之一：它使我们从手动资源释放中解放出来。

上述代码能编译，但是，最普通的Widget用法却会导致编译出错：
```C++
#include "widget.h"

Widget w;                           //错误！
```
报错的信息与不完整类型相关。在对象w被析构时（例如离开了作用域），问题出现了。在这个时候，它的析构函数被调用。我们在类的定义里使用了std::unique_ptr，所以我们没有声明一个析构函数，因为我们并没有任何代码需要写在里面。根据编译器自动生成的特殊成员函数的规则（见 Item17），编译器会自动为我们生成一个析构函数。 在这个析构函数里，编译器会插入一些代码来调用类Widget的数据成员pImpl的析构函数。 pImpl是一个std::unique_ptr<Widget::Impl>，也就是说，一个使用默认删除器的std::unique_ptr。 默认删除器是一个函数，它使用delete来销毁内置于std::unique_ptr的原始指针。然而，在使用delete之前，通常会使默认删除器使用C++11的特性static_assert来确保原始指针指向的类型不是一个不完整类型。 当编译器为Widget w的析构生成代码时，它会遇到static_assert检查并且失败，这通常是错误信息的来源。

为了解决这个问题，你只需要确保在编译器生成销毁std::unique_ptr<Widget::Impl>的代码之前， Widget::Impl已经是一个完整类型（complete type）。 当编译器“看到”它的定义的时候，该类型就成为完整类型了。 但是 Widget::Impl的定义在widget.cpp里。成功编译的关键，就是在widget.cpp文件内，让编译器在“看到” Widget的析构函数实现之前（也即编译器插入的，用来销毁std::unique_ptr这个数据成员的代码的，那个位置），先定义Widget::Impl。
```C++
#include "widget.h"                 //跟之前一样，在“widget.cpp”中
#include "gadget.h"
#include <string>
#include <vector>

struct Widget::Impl {               //跟之前一样，定义Widget::Impl
    std::string name;
    std::vector<double> data;
    Gadget g1,g2,g3;
}

Widget::Widget()                    //跟之前一样
: pImpl(std::make_unique<Impl>())
{}

Widget::~Widget()                   //析构函数的定义（译者注：这里高亮）
{}
```

使用了Pimpl惯用法的类自然适合支持移动操作，因为编译器自动生成的移动操作正合我们所意：对其中的std::unique_ptr进行移动。 正如Item17所解释的那样，声明一个类Widget的析构函数会阻止编译器生成移动操作，所以如果你想要支持移动操作，你必须自己声明相关的函数。考虑到编译器自动生成的版本会正常运行，你可能会很想按如下方式实现它们：
```C++
class Widget {                                  //仍然在“widget.h”中
public:
    Widget();
    ~Widget();

    Widget(Widget&& rhs) = default;             //思路正确，
    Widget& operator=(Widget&& rhs) = default;  //但代码错误
    …

private:                                        //跟之前一样
    struct Impl;
    std::unique_ptr<Impl> pImpl;
};
```

这样的做法会导致同样的错误，和之前的声明一个不带析构函数的类的错误一样，并且是因为同样的原因。 编译器生成的移动赋值操作符，在重新赋值之前，需要先销毁指针pImpl指向的对象。然而在Widget的头文件里，pImpl指针指向的是一个不完整类型。移动构造函数的情况有所不同。 移动构造函数的问题是编译器自动生成的代码里，包含有抛出异常的事件，在这个事件里会生成销毁pImpl的代码。然而，销毁pImpl需要Impl是一个完整类型。

所以你可以用上面相同的处理方式解决：
```C++
#include <string>                   //跟之前一样，仍然在“widget.cpp”中
…
    
struct Widget::Impl { … };          //跟之前一样

Widget::Widget()                    //跟之前一样
: pImpl(std::make_unique<Impl>())
{}

Widget::~Widget() = default;        //跟之前一样

Widget::Widget(Widget&& rhs) = default;             //这里定义
Widget& Widget::operator=(Widget&& rhs) = default;
```

**请记住：**
* Pimpl惯用法通过减少在类实现和类使用者之间的编译依赖来减少编译时间。
* 对于std::unique_ptr类型的pImpl指针，需要在头文件的类里声明特殊的成员函数，但是在实现文件里面来实现他们。即使是编译器自动生成的代码可以工作，也要这么做。
* 以上的建议只适用于std::unique_ptr，不适用于std::shared_ptr。