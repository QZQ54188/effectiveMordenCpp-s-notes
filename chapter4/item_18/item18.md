## item18:对于独占资源使用std::unique_ptr

[item18原文](https://cntransgroup.github.io/EffectiveModernCppChinese/4.SmartPointers/item18.html)

为什么要引入智能指针，因为原始指针存在许多缺陷，比如说：
1. 它的声明不能指示所指到底是单个对象还是数组。代码可读性较低。
2. 它的声明没有告诉你用完后是否应该销毁它，即指针是否拥有所指之物。
3. 如果你决定你应该销毁指针所指对象，没人告诉你该用delete还是其他析构机制（比如将指针传给专门的销毁函数）。
4. 如果你发现该用delete。 原因1说了可能不知道该用单个对象形式（“delete”）还是数组形式（“delete[]”）。如果用错了结果是未定义的。
5. 一般来说没有办法告诉你指针是否变成了悬空指针（dangling pointers），即内存中不再存在指针所指之物。在对象销毁后指针仍指向它们就会产生悬空指针。

智能指针（smart pointers）是解决这些问题的一种办法。智能指针包裹原始指针，它们的行为看起来像被包裹的原始指针，但避免了原始指针的很多陷阱。你应该更倾向于智能指针而不是原始指针。几乎原始指针能做的所有事情智能指针都能做，而且出错的机会更少。在C++11中存在四种智能指针：std::auto_ptr，std::unique_ptr，std::shared_ptr， std::weak_ptr。都是被设计用来帮助管理动态对象的生命周期，在适当的时间通过适当的方式来销毁对象，以避免出现资源泄露或者异常行为。

相较于原始指针，智能指针的大小与原始指针相同，而且对于大多数操作（包括取消引用），他们执行的指令完全相同。这意味着你甚至可以在内存和时间都比较紧张的情况下使用它。如果原始指针够小够快，那么std::unique_ptr一样可以。

std::unique_ptr体现了专有所有权（exclusive ownership）语义。一个non-null std::unique_ptr始终拥有其指向的内容。移动一个std::unique_ptr将所有权从源指针转移到目的指针。拷贝一个std::unique_ptr是不允许的，因为如果你能拷贝一个std::unique_ptr，你会得到指向相同内容的两个std::unique_ptr，每个都认为自己拥有（并且应当最后销毁）资源，销毁时就会出现重复销毁。因此，std::unique_ptr是一种只可移动类型（move-only type）。当析构时，一个non-null std::unique_ptr销毁它指向的资源。默认情况下，资源析构通过对std::unique_ptr里原始指针调用delete来实现。

std::unique_ptr的常见用法是作为继承层次结构中对象的工厂函数返回类型。
>工厂函数是指一种用于创建对象的函数，它通常返回一个新创建的对象的指针或智能指针。工厂函数的作用是封装对象的创建逻辑，使得客户端代码不需要直接关心对象的具体创建细节，尤其是在面对继承和多态时，工厂函数能够返回基类指针，而实际的对象类型可以是派生类。这种方式特别适用于 依赖注入 或 多态 场景，尤其是当对象的创建过程比较复杂时，可以将这些复杂性隐藏在工厂函数内部。

在继承层次结构中使用工厂函数返回 std::unique_ptr 类型的指针，可以确保对象的所有权被正确管理，同时避免了内存泄漏的风险。std::unique_ptr 是一个智能指针，能够在对象生命周期结束时自动销毁对象，并且它不能被复制，只能被移动，因此能够保证资源的唯一所有权。

假设我们有一个投资类型（比如股票、债券、房地产等）的继承结构，使用基类Investment。
```C++
class Investment { … };
class Stock: public Investment { … };
class Bond: public Investment { … };
class RealEstate: public Investment { … };
```
这种继承关系的工厂函数在堆上分配一个对象然后返回指针，调用方在不需要的时候有责任销毁对象。这使用场景完美匹配std::unique_ptr，因为调用者对工厂返回的资源负责（即对该资源的专有所有权），并且std::unique_ptr在自己被销毁时会自动销毁指向的内容。

因此工厂函数的返回值应该是std::unique_ptr<Investment>。
```C++
template<typename... Ts>            //返回指向对象的std::unique_ptr，
std::unique_ptr<Investment>         //对象使用给定实参创建
makeInvestment(Ts&&... params);
```
调用者应该在单独的作用域中使用返回的std::unique_ptr智能指针：
```C++
{
    …
    auto pInvestment =                  //pInvestment是
        makeInvestment( arguments );    //std::unique_ptr<Investment>类型
    …
}                                       //销毁 *pInvestment
```

除此以外，unique_ptr还可以在进行所有权转移的时候使用，例如将工厂函数返回的智能指针移动到容器中，然后再将其移入对象的数据成员中，在使用过后进行销毁。如果所有权链由于异常或者其他非典型控制流出现中断（比如提前从函数return或者循环中的break），则拥有托管资源的std::unique_ptr将保证指向内容的析构函数被调用，销毁对应资源。

默认情况下，销毁将通过delete进行，但是在构造过程中，std::unique_ptr对象可以被设置为使用（对资源的）自定义删除器：当资源需要销毁时可调用的任意函数（或者函数对象，包括lambda表达式）。

在上述工厂函数的例子中，如果通过工厂函数创建的对象不是仅仅被delete，还要输出一条日志信息，makeInvestment可以以如下方式实现。
```C++
auto delInvmt = [](Investment* pInvestment)         //自定义删除器
                {                                   //（lambda表达式）
                    makeLogEntry(pInvestment);
                    delete pInvestment; 
                };

template<typename... Ts>
std::unique_ptr<Investment, decltype(delInvmt)>     //更改后的返回类型
makeInvestment(Ts&&... params)
{
    std::unique_ptr<Investment, decltype(delInvmt)> //应返回的指针
        pInv(nullptr, delInvmt);
    if (/*一个Stock对象应被创建*/)
    {
        pInv.reset(new Stock(std::forward<Ts>(params)...));
    }
    else if ( /*一个Bond对象应被创建*/ )   
    {     
        pInv.reset(new Bond(std::forward<Ts>(params)...));   
    }   
    else if ( /*一个RealEstate对象应被创建*/ )   
    {     
        pInv.reset(new RealEstate(std::forward<Ts>(params)...));   
    }   
    return pInv;
}
```

delInvmt是从makeInvestment返回的对象的自定义的删除器。所有的自定义的删除行为接受要销毁对象的原始指针，然后执行所有必要行为实现销毁操作。在上面情况中，操作包括调用makeLogEntry然后应用delete。使用lambda创建delInvmt是方便的，而且，正如稍后看到的，比编写常规的函数更有效。

当使用自定义删除器时，删除器类型必须作为第二个类型实参传给std::unique_ptr。在上面情况中，就是delInvmt的类型，这就是为什么makeInvestment返回类型是std::unique_ptr<Investment, decltype(delInvmt)>。

makeInvestment的基本策略是创建一个空的std::unique_ptr，然后指向一个合适类型的对象，然后返回。为了将自定义删除器delInvmt与pInv关联，我们把delInvmt作为pInv构造函数的第二个实参。

尝试将原始指针（比如new创建）赋值给std::unique_ptr通不过编译，因为是一种从原始指针到智能指针的隐式转换。这种隐式转换会出问题，所以C++11的智能指针禁止这个行为。这就是通过reset来让pInv接管通过new创建的对象的所有权的原因。

使用new时，我们使用std::forward把传给makeInvestment的实参完美转发出去（查看[Item25](https://cntransgroup.github.io/EffectiveModernCppChinese/5.RRefMovSemPerfForw/item25.html)）。这使调用者提供的所有信息可用于正在创建的对象的构造函数。

自定义删除器的一个形参，类型是Investment*，不管在makeInvestment内部创建的对象的真实类型（如Stock，Bond，或RealEstate）是什么，它最终在lambda表达式中，作为Investment*对象被删除。这意味着我们通过基类指针删除派生类实例，为此，基类Investment必须有虚析构函数：
```C++
class Investment {
public:
    …
    virtual ~Investment();          //关键设计部分！
    …
};
```

在C++14中，函数返回类型推导的存在，意味着makeInvestment可以以更简单，更封装的方式实现：
```C++
template<typename... Ts>
auto makeInvestment(Ts&&... params)                 //C++14
{
    auto delInvmt = [](Investment* pInvestment)     //现在在
                    {                               //makeInvestment里
                        makeLogEntry(pInvestment);
                        delete pInvestment; 
                    };

    std::unique_ptr<Investment, decltype(delInvmt)> //同之前一样
        pInv(nullptr, delInvmt);
    if ( … )                                        //同之前一样
    {
        pInv.reset(new Stock(std::forward<Ts>(params)...));
    }
    else if ( … )                                   //同之前一样
    {     
        pInv.reset(new Bond(std::forward<Ts>(params)...));   
    }   
    else if ( … )                                   //同之前一样
    {     
        pInv.reset(new RealEstate(std::forward<Ts>(params)...));   
    }   
    return pInv;                                    //同之前一样
}
```

我之前说过，当使用默认删除器时（如delete），你可以合理假设std::unique_ptr对象和原始指针大小相同。当自定义删除器时，情况可能不再如此。函数指针形式的删除器，通常会使std::unique_ptr的大小从一个字（word）增加到两个。对于函数对象形式的删除器来说，变化的大小取决于函数对象中存储的状态多少，无状态函数（stateless function）对象（比如不捕获变量的lambda表达式）对大小没有影响，这意味当自定义删除器可以实现为函数或者lambda时，尽量使用lambda：
```C++
auto delInvmt1 = [](Investment* pInvestment)        //无状态lambda的
                 {                                  //自定义删除器
                     makeLogEntry(pInvestment);
                     delete pInvestment; 
                 };

template<typename... Ts>                            //返回类型大小是
std::unique_ptr<Investment, decltype(delInvmt1)>    //Investment*的大小
makeInvestment(Ts&&... args);

void delInvmt2(Investment* pInvestment)             //函数形式的
{                                                   //自定义删除器
    makeLogEntry(pInvestment);
    delete pInvestment;
}
template<typename... Ts>                            //返回类型大小是
std::unique_ptr<Investment, void (*)(Investment*)>  //Investment*的指针
makeInvestment(Ts&&... params);                     //加至少一个函数指针的大小
```

std::unique_ptr是C++11中表示专有所有权的方法，但是其最吸引人的功能之一是它可以轻松高效的转换为std::shared_ptr：
```C++
std::shared_ptr<Investment> sp =            //将std::unique_ptr
    makeInvestment(arguments);              //转为std::shared_ptr
```
这就是std::unique_ptr非常适合用作工厂函数返回类型的原因的关键部分。 工厂函数无法知道调用者是否要对它们返回的对象使用专有所有权语义，或者共享所有权（即std::shared_ptr）是否更合适。 通过返回std::unique_ptr，工厂为调用者提供了最有效的智能指针，但它们并不妨碍调用者用其更灵活的兄弟替换它。

**请记住：**
* std::unique_ptr是轻量级、快速的、只可移动（move-only）的管理专有所有权语义资源的智能指针
* 默认情况，资源销毁通过delete实现，但是支持自定义删除器。有状态的删除器和函数指针会增加std::unique_ptr对象的大小
* 将std::unique_ptr转化为std::shared_ptr非常简单