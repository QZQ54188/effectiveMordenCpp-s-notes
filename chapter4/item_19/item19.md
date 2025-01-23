## item19:对于共享资源使用std::shared_ptr

[item19原文](https://cntransgroup.github.io/EffectiveModernCppChinese/4.SmartPointers/item19.html)

C++11中的share_ptr将像是自动垃圾回收和销毁可预测二者结合了起来。一个通过std::shared_ptr访问的对象其生命周期由指向它的有共享所有权（shared ownership）的指针们来管理。没有特定的std::shared_ptr拥有该对象。相反，所有指向它的std::shared_ptr都能相互合作确保在它不再使用的那个点进行析构。当最后一个指向某对象的std::shared_ptr不再指向那（比如因为std::shared_ptr被销毁或者指向另一个不同的对象），std::shared_ptr会销毁它所指向的对象。

因此在垃圾回收方面，用户不再关心指向对象的生命周期，而对象的析构是确定性的。

std::shared_ptr通过引用计数（reference count）来确保它是否是最后一个指向某种资源的指针，引用计数关联资源并跟踪有多少std::shared_ptr指向该资源。通常来说，std::shared_ptr构造函数增加函数的引用计数，析构函数递减值，拷贝赋值运算符做前面这两个工作。（如果sp1和sp2是std::shared_ptr并且指向不同对象，赋值“sp1 = sp2;”会使sp1指向sp2指向的对象。直接效果就是sp1引用计数减一，sp2引用计数加一。）

std::shared_ptr中的引用计数存在性能问题：
* std::shared_ptr大小是原始指针的两倍，因为它内部包含一个指向资源的原始指针，还包含一个指向资源的引用计数值的原始指针。(**control_block**)
* 引用计数的内存必须动态分配。即control_block分配在堆上
* 递增递减引用计数必须是原子性的，因为多个reader、writer可能在不同的线程。比如，指向某种资源的std::shared_ptr可能在一个线程执行析构（于是递减指向的对象的引用计数），在另一个不同的线程，std::shared_ptr指向相同的对象，但是执行的却是拷贝操作（因此递增了同一个引用计数）。

创建一个指向对象的std::shared_ptr就产生了又一个指向那个对象的std::shared_ptr，并不一定会增加引用计数，下面以移动构造函数为例子解释。

原因是移动构造函数的存在。从另一个std::shared_ptr移动构造新std::shared_ptr会将原来的std::shared_ptr设置为null，那意味着老的std::shared_ptr不再指向资源，同时新的std::shared_ptr指向资源。这样的结果就是不需要修改引用计数值。因此移动std::shared_ptr会比拷贝它要快：拷贝要求递增引用计数值，移动不需要。

类似std::unique_ptr，std::shared_ptr使用delete作为资源的默认销毁机制，但是它也支持自定义的删除器。这种支持有别于std::unique_ptr。对于std::unique_ptr来说，删除器类型是智能指针类型的一部分。对于std::shared_ptr则不是：
```C++
auto loggingDel = [](Widget *pw)        //自定义删除器
                  {                     //（和条款18一样）
                      makeLogEntry(pw);
                      delete pw;
                  };

std::unique_ptr<                        //删除器类型是
    Widget, decltype(loggingDel)        //指针类型的一部分
    > upw(new Widget, loggingDel);
std::shared_ptr<Widget>                 //删除器类型不是
    spw(new Widget, loggingDel);        //指针类型的一部分
```

std::shared_ptr的设计更加灵活，考虑有两个std::shared_ptr<Widget>，每个自带不同的删除器
```C++
auto customDeleter1 = [](Widget *pw) { … };     //自定义删除器，
auto customDeleter2 = [](Widget *pw) { … };     //每种类型不同
std::shared_ptr<Widget> pw1(new Widget, customDeleter1);
std::shared_ptr<Widget> pw2(new Widget, customDeleter2);
```

因为pw1和pw2有相同的类型，所以它们都可以放到存放那个类型的对象的容器中，它们也能相互赋值，也可以传入一个形参为std::shared_ptr<Widget>的函数。但是自定义删除器类型不同的std::unique_ptr就不行，因为std::unique_ptr把删除器视作类型的一部分。：
```C++
std::vector<std::shared_ptr<Widget>> vpw{ pw1, pw2 };
```

另一个不同于std::unique_ptr的地方是，指定自定义删除器不会改变std::shared_ptr对象的大小。不管删除器是什么，一个std::shared_ptr对象都是两个指针大小。但是自定义删除器可以是函数对象，函数对象可以包含任意多的数据。它意味着函数对象是任意大的。std::shared_ptr怎么能引用一个任意大的删除器而不使用更多的内存？它不能。它必须使用更多的内存。然而，那部分内存不是std::shared_ptr对象的一部分。那部分在堆上面。

个std::shared_ptr管理的对象都有个相应的控制块。控制块除了包含引用计数值外还有一个自定义删除器的拷贝，当然前提是存在自定义删除器。如果用户还指定了自定义分配器，控制块也会包含一个分配器的拷贝。控制块可能还包含一些额外的数据，一个次级引用计数weak count。
![控制块](https://cntransgroup.github.io/EffectiveModernCppChinese/4.SmartPointers/media/item19_fig1.png)

当指向对象的std::shared_ptr一创建，对象的控制块就建立了。对于一个创建指向对象的std::shared_ptr的函数来说不可能知道是否有其他std::shared_ptr早已指向那个对象，所以控制块的创建会遵循下面几条规则：
* std::make_shared总是创建一个控制块。它创建一个要指向的新对象，所以可以肯定std::make_shared调用时对象不存在其他控制块。
* 当从独占指针上构造出std::shared_ptr时会创建控制块。独占指针没有使用控制块，所以指针指向的对象没有关联控制块。（作为构造的一部分，std::shared_ptr侵占独占指针所指向的对象的独占权，所以独占指针被设置为null）
* 当从原始指针上构造出std::shared_ptr时会创建控制块。(创建原始指针pw指向动态分配的对象很糟糕，因为它完全背离了这章的建议：倾向于使用智能指针而不是原始指针。)

这些规则造成的后果就是从原始指针上构造超过一个std::shared_ptr就会让你走上未定义行为的快车道，因为指向的对象有多个控制块关联。多个控制块意味着多个引用计数值，多个引用计数值意味着对象将会被销毁多次（每个引用计数一次）。那意味着像下面的代码是有问题的:
```C++
auto pw = new Widget;                           //pw是原始指针
…
std::shared_ptr<Widget> spw1(pw, loggingDel);   //为*pw创建控制块
…
std::shared_ptr<Widget> spw2(pw, loggingDel);   //为*pw创建第二个控制块
```

现在，传给spw1的构造函数一个原始指针，它会为指向的对象创建一个控制块（因此有个引用计数值）。这种情况下，指向的对象是\*pw（即pw指向的对象）。就其本身而言没什么问题，但是将同样的原始指针传递给spw2的构造函数会再次为*pw创建一个控制块（所以也有个引用计数值）。因此*pw有两个引用计数值，每一个最后都会变成零，然后最终导致*pw销毁两次。第二个销毁会产生未定义行为。

std::shared_ptr给我们上了两堂课。第一，避免传给std::shared_ptr构造函数原始指针。通常替代方案是使用std::make_shared。第二，如果你必须传给std::shared_ptr构造函数原始指针，直接传new出来的结果，不要传指针变量。如果上面代码第一部分这样重写：
```C++
std::shared_ptr<Widget> spw1(new Widget,    //直接使用new的结果
                             loggingDel);
```

会少了很多从原始指针上构造第二个std::shared_ptr的诱惑。相应的，创建spw2也会很自然的用spw1作为初始化参数（即用std::shared_ptr拷贝构造函数），那就没什么问题了：
```C++
std::shared_ptr<Widget> spw2(spw1);         //spw2使用spw1一样的控制块
```

在通常情况下，使用默认删除器和默认分配器，使用std::make_shared创建std::shared_ptr，产生的控制块只需三个word大小。它的分配基本上是无开销的。对std::shared_ptr解引用的开销不会比原始指针高。执行需要原子引用计数修改的操作需要承担一两个原子操作开销，这些操作通常都会一一映射到机器指令上，所以即使对比非原子指令来说，原子指令开销较大，但是它们仍然只是单个指令上的。

大多数时候，比起手动管理，使用std::shared_ptr管理共享性资源都是非常合适的。如果你还在犹豫是否能承受std::shared_ptr带来的开销，那就再想想你是否需要共享所有权。如果独占资源可行或者可能可行，用std::unique_ptr是一个更好的选择。它的性能表现更接近于原始指针，并且从std::unique_ptr升级到std::shared_ptr也很容易，因为std::shared_ptr可以从std::unique_ptr上创建。

std::shared_ptr不能处理的另一个东西是数组。和std::unique_ptr不同的是，std::shared_ptr的API设计之初就是针对单个对象的，没有办法std::shared_ptr<T[]>。（译者注: 自 C++17 起 std::shared_ptr 可以用于管理动态分配的数组，使用 std::shared_ptr<T[]>

程序员踌躇于是否该使用std::shared_ptr<T>指向数组，然后传入自定义删除器来删除数组（即delete []）。这可以通过编译，但是是一个糟糕的主意。一方面，std::shared_ptr没有提供operator[]，所以数组索引操作需要借助怪异的指针算术。另一方面，std::shared_ptr支持转换为指向基类的指针，这对于单个对象来说有效，但是当用于数组类型时相当于在类型系统上开洞。（出于这个原因，std::unique_ptr<T[]> API禁止这种转换。）更重要的是，C++11已经提供了很多内置数组的候选方案（比如std::array，std::vector，std::string）。

**请记住：**
* std::shared_ptr为有共享所有权的任意资源提供一种自动垃圾回收的便捷方式。
* 较之于std::unique_ptr，std::shared_ptr对象通常大两倍，控制块会产生开销，需要原子性的引用计数修改操作。
* 默认资源销毁是通过delete，但是也支持自定义删除器。删除器的类型是什么对于std::shared_ptr的类型没有影响。
* 避免从原始指针变量上创建std::shared_ptr。
