## item26:避免在通用引用上重载

[item26原文](https://cntransgroup.github.io/EffectiveModernCppChinese/5.RRefMovSemPerfForw/item26.html)

假定你需要写一个函数，它使用名字作为形参，打印当前日期和时间到日志中，然后将名字加入到一个全局数据结构中。你可能写出来这样的代码：
```C++
std::multiset<std::string> names;           //全局数据结构
void logAndAdd(const std::string& name)
{
    auto now =                              //获取当前时间
        std::chrono::system_clock::now();
    log(now, "logAndAdd");                  //志记信息
    names.emplace(name);                    //把name加到全局数据结构中；
}                                           //emplace的信息见条款42
```

这段代码的效率不高，考虑以下三个调用：
```C++
std::string petName("Darla");
logAndAdd(petName);                     //传递左值std::string
logAndAdd(std::string("Persephone"));	//传递右值std::string
logAndAdd("Patty Dog");                 //传递字符串字面值
```
在第一个调用中，logAndAdd的形参name绑定到变量petName。在logAndAdd中name最终传给names.emplace。因为name是左值，会拷贝到names中。没有方法避免拷贝，因为是左值（petName）传递给logAndAdd的。

在第二个调用中，形参name被绑定到右值（显式从“Persephone”创建的临时std::string）。name本身是个左值，所以它被拷贝到names中，但是我们意识到，原则上，它的值可以被移动到names中。本次调用中，我们有个拷贝代价。

在第三个调用中，形参name也绑定一个右值，但是这次是通过“Patty Dog”隐式创建的临时std::string变量。就像第二个调用中，name被拷贝到names，但是这里，传递给logAndAdd的实参是一个字符串字面量。效率和第二个调用一样，如果我们可以直接将右值或者字面量传递给emplace_back，那么我们可以避免一次拷贝或者移动。

我们可以使用通用引用改写这个函数提高效率，按照[Item25](https://cntransgroup.github.io/EffectiveModernCppChinese/5.RRefMovSemPerfForw/item25.html)的说法，std::forward转发这个引用到emplace。
```C++
template<typename T>
void logAndAdd(T&& name)
{
    auto now = std::chrono::system_clock::now();
    log(now, "logAndAdd");
    names.emplace(std::forward<T>(name));
}

std::string petName("Darla");           //跟之前一样
logAndAdd(petName);                     //跟之前一样，拷贝左值到multiset
logAndAdd(std::string("Persephone"));	//移动右值而不是拷贝它
logAndAdd("Patty Dog");                 //在multiset直接创建std::string
                                        //而不是拷贝一个临时std::string
```

客户不总是有直接访问logAndAdd要求的名字的权限。有些客户只有索引，logAndAdd拿着索引在表中查找相应的名字。为了支持这些客户，logAndAdd需要重载为：
```C++
std::string nameFromIdx(int idx);   //返回idx对应的名字

void logAndAdd(int idx)             //新的重载
{
    auto now = std::chrono::system_clock::now();
    log(now, "logAndAdd");
    names.emplace(nameFromIdx(idx));
}
//之后两个调用按照预期工作
std::string petName("Darla");           //跟之前一样

logAndAdd(petName);                     //跟之前一样，
logAndAdd(std::string("Persephone")); 	//这些调用都去调用
logAndAdd("Patty Dog");                 //T&&重载版本

logAndAdd(22);                          //调用int重载版本
```

事实上，这只能基本按照预期工作，假定一个客户将short类型索引传递给logAndAdd：
```C++
short nameIdx;
…                                       //给nameIdx一个值
logAndAdd(nameIdx);                     //错误！
```
有两个重载的logAndAdd。使用通用引用的那个推导出T的类型是short，因此可以精确匹配。对于int类型参数的重载也可以在short类型提升后匹配成功。根据正常的重载解决规则，精确匹配优先于类型提升的匹配，所以被调用的是通用引用的重载。

在通用引用那个重载中，name形参绑定到要传入的short上，然后name被std::forward给names（一个std::multiset<std::string>）的emplace成员函数，然后又被转发给std::string构造函数。std::string没有接受short的构造函数，所以logAndAdd调用里的multiset::emplace调用里的std::string构造函数调用失败。这一切的原因就是对于short类型通用引用重载优先于int类型的重载。

使用通用引用的函数在C++中是最贪婪的函数。它们几乎可以精确匹配任何类型的实参。这也是把重载和通用引用组合在一块是糟糕主意的原因：通用引用的实现会匹配比开发者预期要多得多的实参类型。

一个更容易掉入这种陷阱的例子是写一个完美转发构造函数：
```C++
class Person {
public:
    template<typename T>
    explicit Person(T&& n)              //完美转发的构造函数，初始化数据成员
    : name(std::forward<T>(n)) {}

    explicit Person(int idx)            //int的构造函数
    : name(nameFromIdx(idx)) {}
    …

private:
    std::string name;
};
```
由类中构造和析构函数的自动生成可知，编译器会自动生成拷贝构造和移动构造函数：
```C++
class Person {
public:
    template<typename T>            //完美转发的构造函数
    explicit Person(T&& n)
    : name(std::forward<T>(n)) {}

    explicit Person(int idx);       //int的构造函数

    Person(const Person& rhs);      //拷贝构造函数（编译器生成）
    Person(Person&& rhs);           //移动构造函数（编译器生成）
    …
};
```

这里我们试图通过一个Person实例创建另一个Person，显然应该调用拷贝构造即可。
```C++
Person p("Nancy"); 
auto cloneOfP(p);                   //从p创建新Person；这通不过编译！
```
但是这份代码不是调用拷贝构造函数，而是调用完美转发构造函数。然后，完美转发的函数将尝试使用Person对象p初始化Person的std::string数据成员，编译器就会报错。

cloneOfP被non-const左值p初始化，这意味着模板化构造函数可被实例化为采用Person类型的non-const左值。实例化之后，Person类看起来是这样的：
```C++
class Person {
public:
    explicit Person(Person& n)          //由完美转发模板初始化
    : name(std::forward<Person&>(n)) {}

    explicit Person(int idx);           //同之前一样

    Person(const Person& rhs);          //拷贝构造函数（编译器生成的）
    …
};
```

在报错的那段语句中，p被传递给拷贝构造函数或者完美转发构造函数。调用拷贝构造函数要求在p前加上const的约束来满足函数形参的类型，而调用完美转发构造不需要加这些东西。从模板产生的重载函数是更好的匹配，所以编译器按照规则：调用最佳匹配的函数。“拷贝”non-const左值类型的Person交由完美转发构造函数处理，而不是拷贝构造函数。

如果我们将本例中的传递的对象改为const的，会得到完全不同的结果。因为被拷贝的对象是const，是拷贝构造函数的精确匹配。虽然模板化的构造函数可以被实例化为有完全一样的函数签名:
```C++
const Person cp("Nancy");   //现在对象是const的
auto cloneOfP(cp);          //调用拷贝构造函数！

class Person {
public:
    explicit Person(const Person& n);   //从模板实例化而来
  
    Person(const Person& rhs);          //拷贝构造函数（编译器生成的）
    …
};
```

但是没啥影响，因为重载规则规定当模板实例化函数和非模板函数（或者称为“正常”函数）匹配优先级相当时，优先使用“正常”函数。拷贝构造函数（正常函数）因此胜过具有相同签名的模板实例化函数。

当继承纳入考虑范围时，完美转发的构造函数与编译器生成的拷贝、移动操作之间的交互会更加复杂。尤其是，派生类的拷贝和移动操作的传统实现会表现得非常奇怪。
```C++
class SpecialPerson: public Person {
public:
    SpecialPerson(const SpecialPerson& rhs) //拷贝构造函数，调用基类的
    : Person(rhs)                           //完美转发构造函数！
    { … }

    SpecialPerson(SpecialPerson&& rhs)      //移动构造函数，调用基类的
    : Person(std::move(rhs))                //完美转发构造函数！
    { … }
};
```
生类的拷贝和移动构造函数没有调用基类的拷贝和移动构造函数，而是调用了基类的完美转发构造函数！为了理解原因，要知道派生类将SpecialPerson类型的实参传递给其基类，然后通过模板实例化和重载解析规则作用于基类Person。最终，代码无法编译，因为std::string没有接受一个SpecialPerson的构造函数。

**请记住：**
* 对通用引用形参的函数进行重载，通用引用函数的调用机会几乎总会比你期望的多得多。
* 完美转发构造函数是糟糕的实现，因为对于non-const左值，它们比拷贝构造函数而更匹配，而且会劫持派生类对于基类的拷贝和移动构造函数的调用。

