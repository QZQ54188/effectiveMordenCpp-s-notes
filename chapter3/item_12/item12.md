## item12:使用override声明重写函数

[item12原文](https://cntransgroup.github.io/EffectiveModernCppChinese/3.MovingToModernCpp/item12.html)

派生类的虚函数重写基类同名函数是实现运行时多态的重要方法，正是由于虚函数重写机制的存在，才使我们可以通过基类的接口调用派生类的成员函数：
```C++
class Base {
public:
    virtual void doWork();          //基类虚函数
    …
};

class Derived: public Base {
public:
    virtual void doWork();          //重写Base::doWork
    …                               //（这里“virtual”是可以省略的）
}; 

std::unique_ptr<Base> upb =         //创建基类指针指向派生类对象
    std::make_unique<Derived>();    //关于std::make_unique
…                                   //请参见Item21

    
upb->doWork();                      //通过基类指针调用doWork，
                                    //实际上是派生类的doWork
                                    //函数被调用
```

要想在子类中重写父类中虚函数，需要满足以下规则：
* 基类函数必须是virtual
* 基类和派生类函数名必须完全一样（除非是析构函数）
* 基类和派生类函数形参类型必须完全一样
* 基类和派生类函数常量性constness必须完全一样
* 基类和派生类函数的返回值和异常说明（exception specifications）必须兼容

除了这些C++98就存在的约束外，C++11又添加了一个：
函数的引用限定符（reference qualifiers）必须完全一样。引用限定符可以限制成员函数只可用于左值或者右值。

如果基类的虚函数有引用限定符，派生类的重写就必须具有相同的引用限定符。如果没有，那么新声明的函数还是属于派生类，但是不会重写父类的任何函数。
```C++
class Widget {
public:
    …
    void doWork() &;    //只有*this为左值的时候才能被调用
    void doWork() &&;   //只有*this为右值的时候才能被调用
}; 
…
Widget makeWidget();    //工厂函数（返回右值）
Widget w;               //普通对象（左值）
…
w.doWork();             //调用被左值引用限定修饰的Widget::doWork版本
                        //（即Widget::doWork &）
makeWidget().doWork();  //调用被右值引用限定修饰的Widget::doWork版本
                        //（即Widget::doWork &&）
```

这么多的重写需求意味着哪怕一个小小的错误也会造成巨大的不同。代码中包含重写错误通常是有效的，但它的意图不是你想要的。因此你不能指望当你犯错时编译器能通知你。比如，下面的代码是完全合法的，咋一看，还很有道理，但是它没有任何虚函数重写——没有一个派生类函数联系到基类函数。如果你想让编译器帮你发现函数是否成功被重写，只需要在重写函数声明override
```C++
class Base {
public:
    virtual void mf1() const;
    virtual void mf2(int x);
    virtual void mf3() &;
    void mf4() const;
};

class Derived: public Base {
public:
    virtual void mf1();
    virtual void mf2(unsigned int x);
    virtual void mf3() &&;
    void mf4() const;
};
/*mf1在Base基类声明为const，但是Derived派生类没有这个常量限定符

mf2在Base基类声明为接受一个int参数，但是在Derived派生类声明为接受unsigned int参数

mf3在Base基类声明为左值引用限定，但是在Derived派生类声明为右值引用限定

mf4在Base基类没有声明为virtual虚函数*/
```

由于正确声明派生类的重写函数很重要，但很容易出错，C++11提供一个方法让你可以显式地指定一个派生类函数是基类版本的重写：将它声明为override。还是上面那个例子，我们可以这样做：
```C++
class Derived: public Base {
public:
    virtual void mf1() override;
    virtual void mf2(unsigned int x) override;
    virtual void mf3() && override;
    virtual void mf4() const override;
};
```
这样代码由于不满足重写函数的规范，而且override声明，所以编译器会报错，便于我们检测错误。

使用override的代码编译时看起来就像这样（假设我们的目的是Derived派生类中的所有函数重写Base基类的相应虚函数）:
```C++
class Base {
public:
    virtual void mf1() const;
    virtual void mf2(int x);
    virtual void mf3() &;
    virtual void mf4() const;
};

class Derived: public Base {
public:
    virtual void mf1() const override;
    virtual void mf2(int x) override;
    virtual void mf3() & override;
    void mf4() const override;          //可以添加virtual，但不是必要
}; 
```

C++既有很多关键字，C++11引入了两个上下文关键字（contextual keywords），override和final（向虚函数添加final可以防止派生类重写。final也能用于类，这时这个类不能用作基类）。这两个关键字的特点是它们是保留的，它们只是位于特定上下文才被视为关键字。对于override，它只在成员函数声明结尾处才被视为关键字。这意味着如果你以前写的代码里面已经用过override这个名字，那么换到C++11标准你也无需修改代码：
```C++
class Warning {         //C++98潜在的传统类代码
public:
    …
    void override();    //C++98和C++11都合法（且含义相同）
    …
};
```

如果我们想写一个函数只接受左值实参，我们声明一个non-const左值引用形参：
```C++
void doSomething(Widget& w);    //只接受左值Widget对象
```

如果我们想写一个函数只接受右值实参，我们声明一个右值引用形参：
```C++
void doSomething(Widget&& w);   //只接受右值Widget对象
```

成员函数的引用限定可以很容易的区分一个成员函数被哪个对象（即\*this）调用。它和在成员函数声明尾部添加一个const很相似，暗示了调用这个成员函数的对象（即*this）是const的。

对成员函数添加引用限定不常见，但是可以见。举个例子，假设我们的Widget类有一个std::vector数据成员，我们提供一个访问函数让客户端可以直接访问它：
```C++
class Widget {
public:
    using DataType = std::vector<double>;   //“using”的信息参见Item9
    …
    DataType& data() { return values; }
    …
private:
    DataType values;
};
```

考虑用户对这段代码进行这样的调用：
```C++
Widget w;
…
auto vals1 = w.data();  //拷贝w.values到vals1
```
Widget::data函数的返回值是一个左值引用（准确的说是std::vector<double>&）, 因为左值引用是左值，所以vals1是从左值初始化的。因此vals1由w.values拷贝构造而得。

现在假设我们有一个创建Widgets的工厂函数，我们想用makeWidget返回的Widget里的std::vector初始化一个变量：
```C++
Widget makeWidget();
auto vals2 = makeWidget().data();   //拷贝Widget里面的值到vals2
```
再说一次，Widgets::data返回的是左值引用，还有，左值引用是左值。所以，我们的对象（vals2）得从Widget里的values拷贝构造。这一次，Widget是makeWidget返回的临时对象（即右值），所以将其中的std::vector进行拷贝纯属浪费。最好是移动，但是因为data返回左值引用，C++的规则要求编译器不得不生成一个拷贝。

我们需要的是指明当data被右值Widget对象调用的时候结果也应该是一个右值。现在就可以使用引用限定，为左值Widget和右值Widget写一个data的重载函数来达成这一目的：
```C++
class Widget {
public:
    using DataType = std::vector<double>;
    …
    DataType& data() &              //对于左值Widgets,
    { return values; }              //返回左值
    
    DataType data() &&              //对于右值Widgets,
    { return std::move(values); }   //返回右值
    …

private:
    DataType values;
};

auto vals1 = w.data();              //调用左值重载版本的Widget::data，
                                    //拷贝构造vals1
auto vals2 = makeWidget().data();   //调用右值重载版本的Widget::data, 
                                    //移动构造vals2
```

**请记住：**
* 为重写函数加上override
* 成员函数引用限定让我们可以区别对待左值对象和右值对象（即*this)