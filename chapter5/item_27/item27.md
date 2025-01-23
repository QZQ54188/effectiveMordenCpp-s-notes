## item27:熟悉通用引用重载的替代方法

[item27原文](https://cntransgroup.github.io/EffectiveModernCppChinese/5.RRefMovSemPerfForw/item27.html)

在Item26中的第一个例子中，logAndAdd是许多函数的代表，这些函数可以使用不同的名字来避免在通用引用上的重载的弊端。例如两个重载的logAndAdd函数，可以分别改名为logAndAddName和logAndAddNameIdx。但是，这种方式不能用在第二个例子，Person构造函数中，因为构造函数的名字被语言固定了。此外谁愿意放弃重载呢？

通常在不增加复杂性的情况下提高性能的一种方法是，将按传引用形参替换为按值传递。假如有如下代码：
```C++
class Person {
public:
    explicit Person(std::string n)  //代替T&&构造函数，
    : name(std::move(n)) {}         //std::move的使用见条款41
  
    explicit Person(int idx)        //同之前一样
    : name(nameFromIdx(idx)) {}
    …

private:
    std::string name;
};
```
因为没有std::string构造函数可以接受整型参数，所有int或者其他整型变量（比如std::size_t、short、long等）都会使用int类型重载的构造函数。相似的，所有std::string类似的实参（还有可以用来创建std::string的东西，比如字面量“Ruth”等）都会使用std::string类型的重载构造函数。

传递lvalue-reference-to-const以及按值传递都不支持完美转发。如果使用通用引用的动机是完美转发，我们就只能使用通用引用了，没有其他选择。但是又不想放弃重载。所以如果不放弃重载又不放弃通用引用，如何避免在通用引用上重载呢？

```C++
std::multiset<std::string> names;       //全局数据结构

template<typename T>                    //志记信息，将name添加到数据结构
void logAndAdd(T&& name)
{
    auto now = std::chrono::system_clokc::now();
    log(now, "logAndAdd");
    names.emplace(std::forward<T>(name));
}
```
如何不通过重载，我们重新实现logAndAdd函数分拆为两个函数，一个针对整型值，一个针对其他。logAndAdd本身接受所有实参类型，包括整型和非整型。

这两个真正执行逻辑的函数命名为logAndAddImpl，即我们使用重载。其中一个函数接受通用引用。所以我们同时使用了重载和通用引用。但是每个函数接受第二个形参，表征传入的实参是否为整型。
```C++
template<typename T>
void logAndAdd(T&& name) 
{
    logAndAddImpl(std::forward<T>(name),
                  std::is_integral<T>());   //不那么正确
}
```
如果左值实参传递给通用引用name，对T类型推断会得到左值引用。所以如果左值int被传入logAndAdd，T将被推断为int&。这不是一个整型类型，因为引用不是整型类型。这意味着std::is_integral<T>对于任何左值实参返回false，即使确实传入了整型值。所以我们可以使用类型萃取器：std::remove_reference移除类型的引用说明符。
```C++
template<typename T>
void logAndAdd(T&& name)
{
    logAndAddImpl(
        std::forward<T>(name),
        std::is_integral<typename std::remove_reference<T>::type>()
    );
}
```

现在我们将注意力放在logAndAddImpl函数上，这个函数有两个重载类型，第一个：
```C++
template<typename T>                            //非整型实参：添加到全局数据结构中
void logAndAddImpl(T&& name, std::false_type)	//译者注：高亮std::false_type
{
    auto now = std::chrono::system_clock::now();
    log(now, "logAndAdd");
    names.emplace(std::forward<T>(name));
}
```
概念上，logAndAdd传递一个布尔值给logAndAddImpl表明是否传入了一个整型类型，但是true和false是运行时值，我们需要使用重载决议——编译时决策——来选择正确的logAndAddImpl重载。这意味着我们需要一个类型对应true，另一个不同的类型对应false。这个需要是经常出现的，所以标准库提供了这样两个命名std::true_type和std::false_type。logAndAdd传递给logAndAddImpl的实参是个对象，如果T是整型，对象的类型就继承自std::true_type，反之继承自std::false_type。最终的结果就是，当T不是整型类型时，这个logAndAddImpl重载是个可供调用的候选者。

第二个函数重载就是差不多形参列表的int函数重载：
```C++
std::string nameFromIdx(int idx);           //与条款26一样，整型实参：查找名字并用它调用logAndAdd
void logAndAddImpl(int idx, std::true_type) //译者注：高亮std::true_type
{
  logAndAdd(nameFromIdx(idx)); 
}
```

在这个设计中，类型std::true_type和std::false_type是“标签”（tag），其唯一目的就是强制重载解析按照我们的想法来执行。注意到我们甚至没有对这些参数进行命名。他们在运行时毫无用处，事实上我们希望编译器可以意识到这些标签形参没被使用，然后在程序执行时优化掉它们。（至少某些时候有些编译器会这样做。）通过创建标签对象，在logAndAdd内部将重载实现函数的调用“分发”（dispatch）给正确的重载。因此这个设计名称为：tag dispatch。

分发函数——logAndAdd——接受一个没有约束的通用引用参数，但是这个函数没有重载。实现函数——logAndAddImpl——是重载的，一个接受通用引用参数，但是重载规则不仅依赖通用引用形参，还依赖新引入的标签形参，标签值设计来保证有不超过一个的重载是合适的匹配。结果是标签来决定采用哪个重载函数。

创建一个没有重载的分发函数通常是容易的，但是Item26中所述第二个问题案例是Person类的完美转发构造函数，是个例外。编译器可能会自行生成拷贝和移动构造函数，所以即使你只写了一个构造函数并在其中使用tag dispatch，有一些对构造函数的调用也被编译器生成的函数处理，绕过了分发机制。

如同Item26中所述，提供具有通用引用的构造函数，会使通用引用构造函数在拷贝non-const左值时被调用（而不是拷贝构造函数）。那个条款还说明了当一个基类声明了完美转发构造函数，派生类实现自己的拷贝和移动构造函数时会调用那个完美转发构造函数，尽管正确的行为是调用基类的拷贝或者移动构造。

这种情况，采用通用引用的重载函数通常比期望的更加贪心，虽然不像单个分派函数一样那么贪心，而又不满足使用tag dispatch的条件。你需要另外的技术，可以让你确定允许使用通用引用模板的条件，你需要的就是std::enable_if。

std::enable_if可以给你提供一种强制编译器执行行为的方法，像是特定模板不存在一样。这种模板被称为被禁止（disabled）。默认情况下，所有模板是启用的（enabled），但是使用std::enable_if可以使得仅在std::enable_if指定的条件满足时模板才启用。在这个例子中，我们只在传递的类型不是Person时使用Person的完美转发构造函数。如果传递的类型是Person，我们要禁止完美转发构造函数（即让编译器忽略它），因为这会让拷贝或者移动构造函数处理调用，这是我们想要使用Person初始化另一个Person的初衷。

下面的代码是Person完美转发构造函数的声明：
```C++
class Person {
public:
    template<typename T,
             typename = typename std::enable_if<condition>::type>   //译者注：本行高亮，condition为某其他特定条件
    explicit Person(T&& n);
    …
};
```
这里我们想表示的条件是确认T不是Person类型，即模板构造函数应该在T不是Person类型的时候启用。

如果我们更精细考虑仅当T不是Person类型才启用模板构造函数，我们会意识到当我们查看T时，应该忽略：
* 是否是个引用。对于决定是否通用引用构造函数启用的目的来说，Person，Person&，Person&&都是跟Person一样的。
* 是不是const或者volatile。如上所述，const Person，volatile Person ，const volatile Person也是跟Person一样的。

这意味着我们需要一种方法消除对于T的引用，const，volatile修饰。再次，标准库提供了这样功能的type trait，就是std::decay。std::decay<T>::type与T是相同的，只不过会移除引用和cv限定符（cv-qualifiers，即const或volatile标识符）的修饰。
```C++
!std::is_same<Person, typename std::decay<T>::type>::value
```
即Person和T的类型不同，忽略了所有引用和cv限定符。

将其带回上面std::enable_if样板的代码中，加上调整一下格式，让各部分如何组合在一起看起来更容易，Person的完美转发构造函数的声明如下：
```C++
class Person {
public:
    template<
        typename T,
        typename = typename std::enable_if<
                       !std::is_same<Person, 
                                     typename std::decay<T>::type
                                    >::value
                   >::type
    >
    explicit Person(T&& n);
    …
};
```

item26中还有一种情况需要解决，假定从Person派生的类以常规方式实现拷贝和移动操作：
```C++
class SpecialPerson: public Person {
public:
    SpecialPerson(const SpecialPerson& rhs) //拷贝构造函数，调用基类的
    : Person(rhs)                           //完美转发构造函数！
    { … }
    
    SpecialPerson(SpecialPerson&& rhs)      //移动构造函数，调用基类的
    : Person(std::move(rhs))                //完美转发构造函数！
    { … }
    
    …
};
```
当我们拷贝或者移动一个SpecialPerson对象时，我们希望调用基类对应的拷贝和移动构造函数，来拷贝或者移动基类部分，但是这里，我们将SpecialPerson传递给基类的构造函数，因为SpecialPerson和Person类型不同（在应用std::decay后也不同），所以完美转发构造函数是启用的，会实例化为精确匹配SpecialPerson实参的构造函数。相比于派生类到基类的转化——这个转化对于在Person拷贝和移动构造函数中把SpecialPerson对象绑定到Person形参非常重要，生成的精确匹配是更优的，所以这里的代码，拷贝或者移动SpecialPerson对象就会调用Person类的完美转发构造函数来执行基类的部分。

派生类仅仅是按照常规的规则生成了自己的移动和拷贝构造函数，所以这个问题的解决还要落实在基类，尤其是控制是否使用Person通用引用构造函数启用的条件。现在我们意识到不只是禁止Person类型启用模板构造函数，而是禁止Person**/以及任何派生自Person**的类型启用模板构造函数。

你应该不意外在这里看到标准库中也有type trait判断一个类型是否继承自另一个类型，就是std::is_base_of。如果std::is_base_of<T1, T2>是true就表示T2派生自T1。类型也可被认为是从他们自己派生，所以std::is_base_of<T, T>::value总是true。这就很方便了，我们想要修正控制Person完美转发构造函数的启用条件，只有当T在消除引用和cv限定符之后，并且既不是Person又不是Person的派生类时，才满足条件。所以使用std::is_base_of代替std::is_same就可以了：
```C++
class Person {
public:
    template<
        typename T,
        typename = typename std::enable_if<
                       !std::is_base_of<Person, 
                                        typename std::decay<T>::type
                                       >::value
                   >::type
    >
    explicit Person(T&& n);
    …
};
```

本条款提到的前三个技术——放弃重载、传递const T&、传值——在函数调用中指定每个形参的类型。后两个技术——tag dispatch和限制模板适用范围——使用完美转发，因此不需要指定形参类型。这一基本决定（是否指定类型）有一定后果。

通常，完美转发更有效率，因为它避免了仅仅去为了符合形参声明的类型而创建临时对象。在Person构造函数的例子中，完美转发允许将“Nancy”这种字符串字面量转发到Person内部的std::string的构造函数，不使用完美转发的技术则会从字符串字面值创建一个临时std::string对象，来满足Person构造函数指定的形参要求。

**请记住：**
* 通用引用和重载的组合替代方案包括使用不同的函数名，通过lvalue-reference-to-const传递形参，按值传递形参，使用tag dispatch。
* 通过std::enable_if约束模板，允许组合通用引用和重载使用，但它也控制了编译器在哪种条件下才使用通用引用重载。
* 通用引用参数通常具有高效率的优势，但是可用性就值得斟酌。