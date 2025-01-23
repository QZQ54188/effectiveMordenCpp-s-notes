## item21:优先考虑使用std::make_unique和std::make_shared,而非直接使用new

[item21原文](https://cntransgroup.github.io/EffectiveModernCppChinese/4.SmartPointers/item21.html)

std::make_shared是C++11标准的一部分，但很可惜的是，std::make_unique不是。它从C++14开始加入标准库。如果你在使用C++11，不用担心，一个基础版本的std::make_unique是很容易自己写出的，如下：
```C++
template<typename T, typename... Ts>
std::unique_ptr<T> make_unique(Ts&&... params)
{
    return std::unique_ptr<T>(new T(std::forward<Ts>(params)...));
}
```
对于std::forward<Ts>(params)...。params... 是一个参数包，表示接收的多个参数。而 std::forward<Ts>(params)... 会对每个参数进行完美转发：

* std::forward<Ts>(params)：对于每个参数 params，std::forward<Ts>(params) 会根据参数的值类别将其传递给目标函数或构造函数。Ts 是模板参数包中的类型，std::forward<Ts> 会确保正确地转发这些类型。
* ...（展开运算符）：... 是一个参数包展开的语法，用于将模板参数包展开成多个独立的参数。它会将每个参数（如 params）依次传递给 std::forward。

例如，如果你传入了两个参数 x 和 y，那么 std::forward<Ts>(params)... 会分别转发 x 和 y，并保留它们原始的值类别。

make_unique只是将它的参数完美转发到所要创建的对象的构造函数，从new产生的原始指针里面构造出std::unique_ptr，并返回这个std::unique_ptr。

即使通过用和不用make函数来创建智能指针的一个小小比较，也揭示了为何使用make函数更好的第一个原因。例如：
```C++
auto upw1(std::make_unique<Widget>());      //使用make函数
std::unique_ptr<Widget> upw2(new Widget);   //不使用make函数
auto spw1(std::make_shared<Widget>());      //使用make函数
std::shared_ptr<Widget> spw2(new Widget);   //不使用make函数
```
使用new的版本重复了类型，但是make函数的版本没有。这里高亮的是Widget，用new的声明语句需要写2遍Widget，make函数只需要写一次。重复写类型和软件工程里面一个关键原则相冲突：应该避免重复代码。源代码中的重复增加了编译的时间，会导致目标代码冗余，并且通常会让代码库使用更加困难。它经常演变成不一致的代码，而代码库中的不一致常常导致bug。

其次，使用make方法的原因和异常安全有关：
```C++
void processWidget(std::shared_ptr<Widget> spw, int priority);
int computePriority();          //计算函数优先级
```

并且我们在调用processWidget时使用了new而不是std::make_shared：
```C++
processWidget(std::shared_ptr<Widget>(new Widget),  //潜在的资源泄漏！
              computePriority());
```

答案和编译器将源码转换为目标代码有关。在运行时，一个函数的实参必须先被计算，这个函数再被调用，所以在调用processWidget之前，必须执行以下操作，processWidget才开始执行：
* 表达式“new Widget”必须计算，例如，一个Widget对象必须在堆上被创建
* 负责管理new出来指针的std::shared_ptr<Widget>构造函数必须被执行
* computePriority必须运行

编译器不需要按照执行顺序生成代码。“new Widget”必须在std::shared_ptr的构造函数被调用前执行，因为new出来的结果作为构造函数的实参，但computePriority可能在这之前，之后，或者之间执行。也就是说，编译器可能按照这个执行顺序生成代码：
1. 执行“new Widget”
2. 执行computePriority
3. 运行std::shared_ptr构造函数


如果按照这样生成代码，并且在运行时computePriority产生了异常，那么第一步动态分配的Widget就会泄漏。因为它永远都不会被第三步的std::shared_ptr所管理了。

使用std::make_unique和std::make_shared可以避免问题，因为他相当于一个原子调用，将在堆上创建对象和将指针指向对象一起执行，保证要么全都不完成要么全部完成：
```C++
processWidget(std::make_shared<Widget>(),   //没有潜在的资源泄漏
              computePriority());
```
在运行时，std::make_shared和computePriority其中一个会先被调用。如果是std::make_shared先被调用，在computePriority调用前，动态分配Widget的原始指针会安全的保存在作为返回值的std::shared_ptr中。如果computePriority产生一个异常，那么std::shared_ptr析构函数将确保管理的Widget被销毁。如果首先调用computePriority并产生一个异常，那么std::make_shared将不会被调用，因此也就不需要担心动态分配Widget（会泄漏）。

如果我们将std::shared_ptr，std::make_shared替换成std::unique_ptr，std::make_unique，同样的道理也适用。因此，在编写异常安全代码时，使用std::make_unique而不是new与使用std::make_shared（而不是new）同样重要。

std::make_shared相较于new方法更有效率，使用std::make_shared允许编译器生成更小，更快的代码，并使用更简洁的数据结构。
```C++
std::shared_ptr<Widget> spw(new Widget);
```
在这段代码中，需要进行内存分配，但它实际上执行了两次。每个std::shared_ptr指向一个控制块，其中包含被指向对象的引用计数，还有其他东西。这个控制块的内存在std::shared_ptr构造函数中分配。因此，直接使用new需要为Widget进行一次内存分配，为控制块再进行一次内存分配。

但是假如使用std::make_shared代替：
```C++
auto spw = std::make_shared<Widget>();
```
一次分配足矣。这是因为std::make_shared分配一块内存，同时容纳了Widget对象和控制块。这种优化减少了程序的静态大小，因为代码只包含一个内存分配调用，并且它提高了可执行代码的速度，因为内存只分配一次。此外，使用std::make_shared避免了对控制块中的某些簿记信息的需要，潜在地减少了程序的总内存占用。

更倾向于使用make函数而不是直接使用new的争论非常激烈。尽管它们在软件工程、异常安全和效率方面具有优势，但本条款的建议是，更倾向于使用make函数，而不是完全依赖于它们。这是因为有些情况下它们不能或不应该被使用。

第一点，make方法都不允许指定自定义删除器，因此假设要自定义指针的删除方法，我们只可以使用new：
```C++
std::unique_ptr<Widget, decltype(widgetDeleter)>
    upw(new Widget, widgetDeleter);

std::shared_ptr<Widget> spw(new Widget, widgetDeleter);
```

第二点，以下两种调用都创建了10个元素，每个值为20的std::vector。这意味着在make函数中，完美转发使用小括号，而不是花括号。坏消息是如果你想用花括号初始化指向的对象，你必须直接使用new。
```C++
auto upv = std::make_unique<std::vector<int>>(10, 20);
auto spv = std::make_shared<std::vector<int>>(10, 20);
```

第三点，与直接使用new相比，std::make_shared在大小和速度上的优势源于std::shared_ptr的控制块与指向的对象放在同一块内存中。当对象的引用计数降为0，对象被销毁（即析构函数被调用）。但是，因为控制块和对象被放在同一块分配的内存块中，直到控制块的内存也被销毁，对象占用的内存才被释放。

控制块还有第二个计数，记录多少个std::weak_ptrs指向控制块。第二个引用计数就是weak count。只要std::weak_ptrs引用一个控制块（即weak count大于零），该控制块必须继续存在。只要控制块存在，包含它的内存就必须保持分配。通过std::shared_ptr的make函数分配的内存，直到最后一个std::shared_ptr和最后一个指向它的std::weak_ptr已被销毁，才会释放。
```C++
class ReallyBigType { … };

auto pBigObj =                          //通过std::make_shared
    std::make_shared<ReallyBigType>();  //创建一个大对象
                    
…           //创建std::shared_ptrs和std::weak_ptrs
            //指向这个对象，使用它们

…           //最后一个std::shared_ptr在这销毁，
            //但std::weak_ptrs还在

…           //在这个阶段，原来分配给大对象的内存还分配着

…           //最后一个std::weak_ptr在这里销毁；
            //控制块和对象的内存被释放
```

但是如果使用new方法，因为控制块和对象是两次不同的分配，因此释放时机不必要相同：
```C++
class ReallyBigType { … };              //和之前一样

std::shared_ptr<ReallyBigType> pBigObj(new ReallyBigType);
                                        //通过new创建大对象

…           //像之前一样，创建std::shared_ptrs和std::weak_ptrs
            //指向这个对象，使用它们
            
…           //最后一个std::shared_ptr在这销毁,
            //但std::weak_ptrs还在；
            //对象的内存被释放

…           //在这阶段，只有控制块的内存仍然保持分配

…           //最后一个std::weak_ptr在这里销毁；
            //控制块内存被释放
```

**请记住：**
* 和直接使用new相比，make函数消除了代码重复，提高了异常安全性。对于std::make_shared和std::allocate_shared，生成的代码更小更快。
* 不适合使用make函数的情况包括需要指定自定义删除器和希望用花括号初始化。
* 对于std::shared_ptrs，其他不建议使用make函数的情况包括(1)有自定义内存管理的类；(2)特别关注内存的系统，非常大的对象，以及std::weak_ptrs比对应的std::shared_ptrs活得更久。

