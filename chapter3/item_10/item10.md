## item10:优先考虑限域enum而非未限域enum

[item10原文](https://cntransgroup.github.io/EffectiveModernCppChinese/3.MovingToModernCpp/item10.html)

在一般情况下，使用花括号声明一个对象会将他的作用域限制在花括号中。但是对于C++98中的非
限域enum来说，enum中声明的对象属于包含这个enum的作用域，也就是说在enum声明的作用域内
不可以有相同的名字。
```C++
enum Color { black, white, red };   //black, white, red在
                                    //Color所在的作用域
auto white = false;                 //错误! white早已在这个作用
                                    //域中声明
```

这些枚举名的名字泄漏进它们所被定义的enum在的那个作用域，
这个事实有一个官方的术语：未限域枚举(unscoped enum)。
在C++11中它们有一个相似物，限域枚举(scoped enum)，它不会导致枚举名泄漏：
限域enum通常使用enum class声明，所有他们有时候也被称为枚举类。
```C++
enum class Color { black, white, red }; //black, white, red
                                        //限制在Color域内
auto white = false;                     //没问题，域内没有其他“white”

Color c = white;                        //错误，域中没有枚举名叫white

Color c = Color::white;                 //没问题
auto c = Color::white;                  //也没问题（也符合Item5的建议）
```

因此我们可以使用限域enum减少命名空间的污染，与此同时，限域enum通常具有另一个
很好的优点：在它的作用域中，枚举名是强类型。
未限域enum中的枚举名会隐式转换为整型（现在，也可以转换为浮点类型）。

```C++
enum Color { black, white, red };       //未限域enum

std::vector<std::size_t>                //func返回x的质因子
  primeFactors(std::size_t x);

Color c = red;
…

if (c < 14.5) {                         // Color与double比较 (!)
    auto factors =                      // 计算一个Color的质因子(!)
      primeFactors(c);
    …
}
```

在enum后面写一个class就可以将非限域enum转换为限域enum，接下来就是完全不同的故事展开了。
现在不存在任何隐式转换可以将限域enum中的枚举名转化为任何其他类型：

```C++
enum class Color { black, white, red }; //Color现在是限域enum

Color c = Color::red;                   //和之前一样，只是
...                                     //多了一个域修饰符

if (c < 14.5) {                         //错误！不能比较
                                        //Color和double
    auto factors =                      //错误！不能向参数为std::size_t
      primeFactors(c);                  //的函数传递Color参数
    …
}
```

但是假如你真的想进行转化，那么使用static_cast进行强制转化即可。

限域enum相较于不限域enum还有第三个好处，那就是限域enum的默认类型总是已知的，默认
底层类型为int，你也可以为他指定底层类型。但是非限域enum的类型是编译器生成的，
所以编译器会为其底层类型选择一个最优解，但是这样会使得enum很有可能用于整个系统，
因此系统中每个包含这个头文件的组件都会依赖它。如果引入一个新状态值，那么可能
整个系统都得重新编译，即使只有一个子系统——或者只有一个函数——使用了新添加的枚举名。
这是大家都不希望看到的。

限域enum避免命名空间污染而且不接受荒谬的隐式类型转换，
但它并非万事皆宜，你可能会很惊讶听到至少有一种情况下非限域enum是很有用的。
那就是牵扯到C++11的std::tuple的时候。比如在社交网站中，
假设我们有一个tuple保存了用户的名字，email地址，声望值：

```C++
using UserInfo =                //类型别名，参见Item9
    std::tuple<std::string,     //名字
               std::string,     //email地址
               std::size_t> ;   //声望
```

虽然注释说明了tuple各个字段对应的意思，但当你在另一文件遇到下面的代码那之前的注释就不是那么有用了：

```C++
UserInfo uInfo;                 //tuple对象
…
auto val = std::get<1>(uInfo);	//获取第一个字段
```

作为一个程序员，你有很多工作要持续跟进。你应该记住第一个字段代表用户的email地址吗？我认为不。可以使用非限域enum将名字和字段编号关联起来以避免上述需求：
```C++
enum UserInfoFields { uiName, uiEmail, uiReputation };

UserInfo uInfo;                         //同之前一样
…
auto val = std::get<uiEmail>(uInfo);    //啊，获取用户email字段的值
```

之所以它能正常工作是因为UserInfoFields中的枚举名隐式转换成std::size_t了，其中std::size_t是std::get模板实参所需的。

对应的限域enum版本就很啰嗦了：
```C++
enum class UserInfoFields { uiName, uiEmail, uiReputation };

UserInfo uInfo;                         //同之前一样
…
auto val =
    std::get<static_cast<std::size_t>(UserInfoFields::uiEmail)>
        (uInfo);
```

为避免这种冗长的表示，我们可以写一个函数传入枚举名并返回对应的std::size_t值，但这有一点技巧性。std::get是一个模板（函数），需要你给出一个std::size_t值的模板实参（注意使用<>而不是()），因此将枚举名变换为std::size_t值的函数必须在编译期产生这个结果。如[Item15](https://cntransgroup.github.io/EffectiveModernCppChinese/3.MovingToModernCpp/item15.html)提到的，那必须是一个constexpr函数。

事实上，它也的确该是一个constexpr函数模板，因为它应该能用于任何enum。如果我们想让它更一般化，我们还要泛化它的返回类型。较之于返回std::size_t，我们更应该返回枚举的底层类型。这可以通过std::underlying_type这个type trait获得。（参见Item9关于type trait的内容）。最终我们还要再加上noexcept修饰（参见Item14），因为我们知道它肯定不会产生异常。根据上述分析最终得到的toUType函数模板在编译期接受任意枚举名并返回它的值：

```C++
template<typename E>
constexpr typename std::underlying_type<E>::type
    toUType(E enumerator) noexcept
{
    return
        static_cast<typename
                    std::underlying_type<E>::type>(enumerator);
}
```

在C++14中，toUType还可以进一步用std::underlying_type_t（参见Item9）代替typename std::underlying_type<E>::type打磨：
```C++
template<typename E>                //C++14
constexpr std::underlying_type_t<E>
    toUType(E enumerator) noexcept
{
    return static_cast<std::underlying_type_t<E>>(enumerator);
}
```

还可以再用C++14 auto（参见Item3）打磨一下代码：
```C++
template<typename E>                //C++14
constexpr auto
    toUType(E enumerator) noexcept
{
    return static_cast<std::underlying_type_t<E>>(enumerator);
}
```

不管它怎么写，toUType现在允许这样访问tuple的字段了：
```C++
auto val = std::get<toUType(UserInfoFields::uiEmail)>(uInfo);
```

**请记住：**
* C++98的enum即非限域enum。
* 限域enum的枚举名仅在enum内可见。要转换为其它类型只能使用cast。
* 非限域/限域enum都支持底层类型说明语法，限域enum底层类型默认是int。非限域enum没有默认底层类型。
* 限域enum总是可以前置声明。非限域enum仅当指定它们的底层类型时才能前置。