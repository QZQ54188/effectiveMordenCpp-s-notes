## item13:优先考虑const_iterator而非iterator

[item13原文](https://cntransgroup.github.io/EffectiveModernCppChinese/3.MovingToModernCpp/item13.html)

STL中的const_iterator等价于指向常量的指针，他们都指向不能被修改的对象。在标准实践是能加上const就加上。这提示我们需要使用一个迭代器只要没必要修改迭代器指向的值，就应该使用const_iterator。

考虑如下vector插入代码
```C++
std::vector<int> values;
…
std::vector<int>::iterator it =
    std::find(values.begin(), values.end(), 1983);
values.insert(it, 1998);
```

这里使用iterator是一个不好的选择，因为这段代码不修改iterator指向的内容。用const_iterator重写这段代码是很平常的，但是在C++98中就不是了。下面是一种概念上可行但是不正确的方法：(C++11模仿C++98的代码)
```C++
typedef std::vector<int>::iterator IterT;               //typedef
typedef std::vector<int>::const_iterator ConstIterT;

std::vector<int> values;
…
ConstIterT ci =
    std::find(static_cast<ConstIterT>(values.begin()),  //cast
              static_cast<ConstIterT>(values.end()),    //cast
              1983);

values.insert(static_cast<IterT>(ci), 1998);    //可能无法通过编译，
                                                //原因见下
```

之所以std::find的调用会出现类型转换是因为在C++98中values是non-const容器，没办法简简单单的从non-const容器中获取const_iterator。严格来说类型转换不是必须的，因为用其他方法获取const_iterator也是可以的（比如你可以把values绑定到reference-to-const变量上，然后再用这个变量代替values），但不管怎么说，从non-const容器中获取const_iterator的做法都有点别扭。

当你费劲地获得了const_iterator，事情可能会变得更糟，因为C++98中，插入操作（以及删除操作）的位置只能由iterator指定，const_iterator是不被接受的。这也是我在上面的代码中，将const_iterator（我那么小心地从std::find搞出来的东西）转换为iterator的原因，因为向insert传入const_iterator不能通过编译。

这一切在C++11中改变了，现在const_iterator既容易获取又容易使用。容器的成员函数cbegin和cend产出const_iterator，甚至对于non-const容器也可用，那些之前使用iterator指示位置（如insert和erase）的STL成员函数也可以使用const_iterator了。

之前C++98的示例代码可以改写：
```C++
std::vector<int> values;                                //和之前一样
…
auto it =                                               //使用cbegin
    std::find(values.cbegin(), values.cend(), 1983);//和cend
values.insert(it, 1998);
```

在C++11中，使用cbegin，cend等作为非成员函数而不是成员函数时，代码会编译错误，因为C++11中没有对应非成员函数版本，在C++14中才补充，举个例子，我们可以泛化下面的findAndInsert：
```C++
template<typename C, typename V>
void findAndInsert(C& container,            //在容器中查找第一次
                   const V& targetVal,      //出现targetVal的位置，
                   const V& insertVal)      //然后在那插入insertVal
{
    using std::cbegin;
    using std::cend;

    auto it = std::find(cbegin(container),  //非成员函数cbegin
                        cend(container),    //非成员函数cend
                        targetVal);
    container.insert(it, insertVal);
}
```

如果你使用C++11，并且想写一个最大程度通用的代码，而你使用的STL没有提供缺失的非成员函数cbegin和它的朋友们，你可以简单的写下你自己的实现。比如，下面就是非成员函数cbegin的实现：
```C++
template <class C>
auto cbegin(const C& container)->decltype(std::begin(container))
{
    return std::begin(container);   //解释见下
}
```
你可能很惊讶非成员函数cbegin没有调用成员函数cbegin吧？我也是。但是请跟逻辑走。这个cbegin模板接受任何代表类似容器的数据结构的实参类型C，并且通过reference-to-const形参container访问这个实参。如果C是一个普通的容器类型（如std::vector<int>），container将会引用一个const版本的容器（如const std::vector<int>&）。对const容器调用非成员函数begin（由C++11提供）将产出const_iterator，这个迭代器也是模板要返回的。用这种方法实现的好处是就算容器只提供begin成员函数（对于容器来说，C++11的非成员函数begin调用这些成员函数）不提供cbegin成员函数也没问题。那么现在你可以将这个非成员函数cbegin施于只直接支持begin的容器。

如果C是原生数组，这个模板也能工作。这时，container成为一个const数组的引用。C++11为数组提供特化版本的非成员函数begin，它返回指向数组第一个元素的指针。一个const数组的元素也是const，所以对于const数组，非成员函数begin返回指向const的指针（pointer-to-const）。在数组的上下文中，所谓指向const的指针（pointer-to-const），也就是const_iterator了。

**请记住：**
* 优先考虑const_iterator而非iterator
* 在最大程度通用的代码中，优先考虑非成员函数版本的begin，end，rbegin等，而非同名成员函数
