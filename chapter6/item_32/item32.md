## item32:使用初始化捕获来移动对象到闭包中

[item32原文](https://cntransgroup.github.io/EffectiveModernCppChinese/6.LambdaExpressions/item32.html)

缺少移动捕获被认为是C++11的一个缺点，直接的补救措施是将该特性添加到C++14中，但标准化委员会选择了另一种方法。他们引入了一种新的捕获机制，该机制非常灵活，移动捕获是它可以执行的技术之一。新功能被称作初始化捕获（init capture），C++11捕获形式能做的所有事它几乎可以做，甚至能完成更多功能。

使用初始化捕获可以：
* 从lambda生成的闭包类中的数据成员名称；
* 初始化该成员的表达式；

以下代码将std::unique_ptr移动到闭包中：
```C++
class Widget {                          //一些有用的类型
public:
    …
    bool isValidated() const;
    bool isProcessed() const;
    bool isArchived() const;
private:
    …
};

auto pw = std::make_unique<Widget>();   //创建Widget；使用std::make_unique
                                        //的有关信息参见条款21

…                                       //设置*pw

auto func = [pw = std::move(pw)]        //使用std::move(pw)初始化闭包数据成员
            { return pw->isValidated()
                     && pw->isArchived(); };
```
初始化捕获“=”的左侧是指定的闭包类中数据成员的名称，右侧则是初始化表达式。“=”左侧的作用域不同于右侧的作用域。左侧的作用域是闭包类，右侧的作用域和lambda定义所在的作用域相同。在上面的示例中，“=”左侧的名称pw表示闭包类中的数据成员，而右侧的名称pw表示在lambda上方声明的对象，即由调用std::make_unique去初始化的变量。因此，“pw = std::move(pw)”的意思是“在闭包中创建一个数据成员pw，并使用将std::move应用于局部变量pw的结果来初始化该数据成员”。

请记住，lambda表达式只是生成一个类和创建该类型对象的一种简单方式而已。没什么事是你用lambda可以做而不能自己手动实现的。 那么我们刚刚看到的C++14的示例代码可以用C++11重新编写，如下所示：
```C++
class IsValAndArch {                            //“is validated and archived”
public:
    using DataType = std::unique_ptr<Widget>;
    
    explicit IsValAndArch(DataType&& ptr)       //条款25解释了std::move的使用
    : pw(std::move(ptr)) {}
    
    bool operator()() const
    { return pw->isValidated() && pw->isArchived(); }
    
private:
    DataType pw;
};

auto func = IsValAndArch(std::make_unique<Widget>())();
```

如果你坚持要在C++11中使用移动捕获，你可以这样模拟：
1. 将要捕获的对象移动到由std::bind产生的函数对象中；
2. 将“被捕获的”对象的引用赋予给lambda。

假设你要创建一个本地的std::vector，在其中放入一组适当的值，然后将其移动到闭包中。在C++14中，这很容易实现：
```C++
std::vector<double> data;               //要移动进闭包的对象

…                                       //填充data

auto func = [data = std::move(data)]    //C++14初始化捕获
            { /*使用data*/ };
```
C++11的等效代码如下：
```C++
std::vector<double> data;               //同上

…                                       //同上

auto func =
    std::bind(                              //C++11模拟初始化捕获
        [](const std::vector<double>& data) //译者注：本行高亮
        { /*使用data*/ },
        std::move(data)                     //译者注：本行高亮
    );
```

如lambda表达式一样，std::bind产生函数对象。我将由std::bind返回的函数对象称为bind对象（bind objects）。std::bind的第一个实参是可调用对象，后续实参表示要传递给该对象的值。一个bind对象包含了传递给std::bind的所有实参的副本。对于每个左值实参，bind对象中的对应对象都是复制构造的。对于每个右值，它都是移动构造的。

当“调用”bind对象（即调用其函数调用运算符）时，其存储的实参将传递到最初传递给std::bind的可调用对象。在此示例中，这意味着当调用func（bind对象）时，func中所移动构造的data副本将作为实参传递给std::bind中的lambda。

