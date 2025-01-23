## item31:避免使用默认捕获模式

**lambda表达式：**
```C++
std::find_if(container.begin(), container.end(),
             [](int val){ return 0 < val && val < 10; });
```

**闭包：**是lambda创建的运行时对象。依赖捕获模式，闭包持有被捕获数据的副本或者引用。在上面的std::find_if调用中，闭包是作为第三个实参在运行时传递给std::find_if的对象。

**闭包类：**是从中实例化闭包的类。每个lambda都会使编译器生成唯一的闭包类。lambda中的语句成为其闭包类的成员函数中的可执行指令。

C++11中有两种默认的捕获模式：按引用捕获和按值捕获。但默认按引用捕获模式可能会带来悬空引用的问题，因为按引用捕获一个局部变量相当于使用指针指向这个局部对象。

按引用捕获会导致闭包中包含了对某个局部变量或者形参的引用，变量或形参只在定义lambda的作用域中可用。如果该lambda创建的闭包生命周期超过了局部变量或者形参的生命周期，那么闭包中的引用将会变成悬空引用。
```C++
using FilterContainer =                     //“using”参见条款9，
    std::vector<std::function<bool(int)>>;  //std::function参见条款2

FilterContainer filters;                    //过滤函数
```

我们可以添加一个过滤器，用来过滤掉5的倍数：
```C++
filters.emplace_back(                       //emplace_back的信息见条款42
    [](int value) { return value % 5 == 0; }
);
```

然而我们可能需要的是能够在运行期计算除数（divisor），即不能将5硬编码到lambda中。因此添加的过滤器逻辑将会是如下这样：
```C++
void addDivisorFilter()
{
    auto calc1 = computeSomeValue1();
    auto calc2 = computeSomeValue2();

    auto divisor = computeDivisor(calc1, calc2);

    filters.emplace_back(                               //危险！对divisor的引用
        [&](int value) { return value % divisor == 0; } //将会悬空！
    );
}
```

lambda对局部变量divisor进行了引用，但该变量的生命周期会在addDivisorFilter返回时结束，刚好就是在语句filters.emplace_back返回之后。因此添加到filters的函数添加完，该函数就死亡了。现在，同样的问题也会出现在divisor的显式按引用捕获。
```C++
filters.emplace_back(
    [&divisor](int value) 			    //危险！对divisor的引用将会悬空！
    { return value % divisor == 0; }
);
```
但通过显式的捕获，能更容易看到lambda的可行性依赖于变量divisor的生命周期。而且使用&divesor意味着我们需要确保lambda表达式的生命周期和引用对象的生命周期一样长。并且从长期来看，显式列出lambda依赖的局部变量和形参，是更加符合软件工程规范的做法。

一个解决问题的方法是，divisor默认按值捕获进去，也就是说可以按照以下方式来添加lambda到filters：
```C++
filters.emplace_back( 							    //现在divisor不会悬空了
    [=](int value) { return value % divisor == 0; }
);
```
这足以满足本实例的要求，但在通常情况下，按值捕获并不能完全解决悬空引用的问题。这里的问题是如果你按值捕获的是一个指针，你将该指针拷贝到lambda对应的闭包里，但这样并不能避免lambda外delete这个指针的行为，从而导致你的副本指针变成悬空指针。

假设在一个Widget类，可以实现向过滤器的容器添加条目：
```C++
class Widget {
public:
    …                       //构造函数等
    void addFilter() const; //向filters添加条目
private:
    int divisor;            //在Widget的过滤器使用
};

void Widget::addFilter() const
{
    filters.emplace_back(
        [=](int value) { return value % divisor == 0; }
    );
}	
```
这个做法看起来是安全的代码。lambda依赖于divisor，但默认的按值捕获确保divisor被拷贝进了lambda对应的所有闭包中，但是这段代码并不安全。

捕获只能应用于lambda被创建时所在作用域里的non-static局部变量，。在Widget::addFilter的视线里，divisor并不是一个局部变量，而是Widget类的一个成员变量。它不能被捕获。而如果默认捕获模式被删除，代码就不能编译了：
```C++
void Widget::addFilter() const
{
    filters.emplace_back(                               //错误！
        [](int value) { return value % divisor == 0; }  //divisor不可用
    ); 
} 
```

此外，如果尝试显式地捕获divisor变量，因为divisor不是一个局部变量或形参：
```C++
void Widget::addFilter() const
{
    filters.emplace_back(
        [divisor](int value)                //错误！没有名为divisor局部变量可捕获
        { return value % divisor == 0; }
    );
}
```
所以如果默认按值捕获不能捕获divisor，而不用默认按值捕获代码就不能编译：解释就是这里隐式使用了一个原始指针：this。每一个non-static成员函数都有一个this指针，每次你使用一个类内的数据成员时都会使用到这个指针。例如，在任何Widget成员函数中，编译器会在内部将divisor替换成this->divisor。在默认按值捕获的Widget::addFilter版本中：
```C++
void Widget::addFilter() const
{
    filters.emplace_back(
        [=](int value) { return value % divisor == 0; }
    );
}
```
真正被捕获的是Widget的this指针，而不是divisor。编译器会将上面的代码看成以下的写法：
```C++
void Widget::addFilter() const
{
    auto currentObjectPtr = this;

    filters.emplace_back(
        [currentObjectPtr](int value)
        { return value % currentObjectPtr->divisor == 0; }
    );
}
```
则lambda闭包内含有widget的this指针的拷贝。

在C++14中，一个更好的捕获成员变量的方式时使用通用的lambda捕获：
```C++
void Widget::addFilter() const
{
    filters.emplace_back(                   //C++14：
        [divisor = divisor](int value)      //拷贝divisor到闭包
        { return value % divisor == 0; }	//使用这个副本
    );
}
```

**请记住：**
* 默认的按引用捕获可能会导致悬空引用。
* 默认的按值捕获对于悬空指针很敏感（尤其是this指针），并且它会误导人产生lambda是独立的想法。
