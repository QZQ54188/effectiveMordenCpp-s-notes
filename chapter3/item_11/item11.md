## item11:优先考虑使用deleted函数而非使用未定义的私有声明

[item11原文](https://cntransgroup.github.io/EffectiveModernCppChinese/3.MovingToModernCpp/item11.html)

如果你写的代码要被其他人使用，你不想让他们调用某个特殊的函数，你通常不会声明这个函数。无声明，不函数。简简单单！但有时C++会给你自动声明一些函数，如果你想防止客户调用这些函数，事情就不那么简单了。

上述场景见于特殊的成员函数，即当有必要时C++自动生成的那些函数。[Item17](https://cntransgroup.github.io/EffectiveModernCppChinese/3.MovingToModernCpp/item17.html)详细讨论了这些函数，但是现在，我们只关心拷贝构造函数和拷贝赋值运算符重载。

在C++98中防止调用这些函数的方法是将它们声明为私有（private）成员函数并且不定义。
>举个例子，在C++ 标准库iostream继承链的顶部是模板类basic_ios。所有istream和ostream类都继承此类（直接或者间接）。拷贝istream和ostream是不合适的，因为这些操作应该怎么做是模棱两可的。比如一个istream对象，代表一个输入值的流，流中有一些已经被读取，有一些可能马上要被读取。如果一个istream被拷贝，需要拷贝将要被读取的值和已经被读取的值吗？解决这个问题最好的方法是不定义这个操作。直接禁止拷贝流。要使这些istream和ostream类不可拷贝，basic_ios在C++98中是这样声明的（包括注释）：

```C++
template <class charT, class traits = char_traits<charT> >
class basic_ios : public ios_base {
public:
    …

private:
    basic_ios(const basic_ios& );           // not defined
    basic_ios& operator=(const basic_ios&); // not defined
};
```
将它们声明为私有成员可以防止客户端调用这些函数。故意不定义它们意味着假如还是有代码用它们（比如成员函数或者类的友元friend），就会在链接时引发缺少函数定义（missing function definitions）错误。

在C++11中有一种更好的方式达到相同目的：用“= delete”将拷贝构造函数和拷贝赋值运算符标记为deleted函数，上述代码在C++11中的声明如下
```C++
template <class charT, class traits = char_traits<charT> >
class basic_ios : public ios_base {
public:
    …

    basic_ios(const basic_ios& ) = delete;
    basic_ios& operator=(const basic_ios&) = delete;
    …
};
```

deleted函数不能以任何方式被调用，即使你在成员函数或者友元函数里面调用deleted函数也不能通过编译。这是较之C++98行为的一个改进，C++98中不正确的使用这些函数在链接时才被诊断出来。

通常，deleted函数被声明为public而不是private。当客户端代码试图调用成员函数，C++会在检查deleted状态前检查它的访问性。当客户端代码调用一个私有的deleted函数，一些编译器只会给出该函数是private的错误（译注：而没有诸如该函数被deleted修饰的错误），即使函数的访问性不影响它是否能被使用。所以值得牢记，如果要将老代码的“私有且未定义”函数替换为deleted函数时请一并修改它的访问性为public，这样可以让编译器产生更好的错误信息。

deleted函数还有一个重要的优势是任何函数都可以标记为deleted，而只有成员函数可被标记为private。假如我们有一个非成员函数，它接受一个整型参数，检查它是否为幸运数：

```C++
bool isLucky(int number);
```

C++有沉重的C包袱，使得含糊的、能被视作数值的任何类型都能隐式转换为int，但是有一些调用可能是没有意义的：
```C++
if (isLucky('a')) …         //字符'a'是幸运数？
if (isLucky(true)) …        //"true"是?
if (isLucky(3.5)) …         //难道判断它的幸运之前还要先截尾成3？
```

如果幸运数必须真的是整型，我们该禁止这些调用通过编译。其中一种方法就是创建deleted重载函数，其参数就是我们想要过滤的类型：
```C++
bool isLucky(int number);       //原始版本
bool isLucky(char) = delete;    //拒绝char
bool isLucky(bool) = delete;    //拒绝bool
bool isLucky(double) = delete;  //拒绝float和double
```
虽然deleted函数不能被使用，但它们还是存在于你的程序中。也即是说，重载决议会考虑它们。这也是为什么上面的函数声明导致编译器拒绝一些不合适的函数调用。
```C++
if (isLucky('a')) …     //错误！调用deleted函数
if (isLucky(true)) …    //错误！
if (isLucky(3.5f)) …    //错误！
```

另一个deleted函数用武之地（private成员函数做不到的地方）是禁止一些模板的实例化。假如你要求一个模板仅支持原生指针
```C++
template<typename T>
void processPointer(T* ptr);
```

在指针的世界里有两种特殊情况。一是void*指针，因为没办法对它们进行解引用，或者加加减减等。另一种指针是char*，因为它们通常代表C风格的字符串，而不是正常意义下指向单个字符的指针。这两种情况要特殊处理，在processPointer模板里面，我们假设正确的函数应该拒绝这些类型。
```C++
template<>
void processPointer<void>(void*) = delete;

template<>
void processPointer<char>(char*) = delete;
```

现在如果使用void*和char*调用processPointer就是无效的，按常理说const void*和const char*也应该无效，所以这些实例也应该标注delete:
```C++
template<>
void processPointer<const void>(const void*) = delete;

template<>
void processPointer<const char>(const char*) = delete;
```
如果你想做得更彻底一些，你还要删除const volatile void*和const volatile char*重载版本，另外还需要一并删除其他标准字符类型的重载版本：std::wchar_t，std::char16_t和std::char32_t。

如果类里面有一个函数模板，你可能想用private（经典的C++98惯例）来禁止这些函数模板实例化，但是不能这样做，因为不能给特化的成员模板函数指定一个不同于主函数模板的访问级别。如果processPointer是类Widget里面的模板函数， 你想禁止它接受void*参数，那么通过下面这样C++98的方法就不能通过编译：
```C++
class Widget {
public:
    …
    template<typename T>
    void processPointer(T* ptr)
    { … }

private:
    template<>                          //错误！
    void processPointer<void>(void*);
    
};
```
问题是模板特例化必须位于一个命名空间作用域，而不是类作用域。deleted函数不会出现这个问题，因为它不需要一个不同的访问级别，且他们可以在类外被删除（因此位于命名空间作用域）：
```C++
class Widget {
public:
    …
    template<typename T>
    void processPointer(T* ptr)
    { … }
    …

};

template<>                                          //还是public，
void Widget::processPointer<void>(void*) = delete;  //但是已经被删除了
```

**请记住：**
* 比起声明函数为private但不定义，使用deleted函数更好
* 任何函数都能被删除（be deleted），包括非成员函数和模板实例（译注：实例化的函数）