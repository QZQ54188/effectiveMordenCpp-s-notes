## item4:学会查看类型推导结果
[item4原文](https://cntransgroup.github.io/EffectiveModernCppChinese/1.DeducingTypes/item4.html)

### IDE编辑器

在IDE中的代码编辑器通常可以显示程序代码中变量，函数，参数的类型，你只需要简单的把鼠标移到它们的上面。
```C++
const int theAnswer = 42;

auto x = theAnswer;
auto y = &theAnswer;
//IDE编辑器可以直接显示x推导的结果为int，y推导的结果为const int*。
```

为此，你的代码必须或多或少的处于可编译状态，因为IDE之所以能提供这些信息是因为一个C++编译器（或者至少是前端中的一个部分）运行于IDE中。


### 编译器诊断

另一个获得推导结果的方法是使用编译器出错时提供的错误消息。举个例子，假如我们想看到之前那段代码中x和y的类型，我们可以首先声明一个类模板但不定义。

```C++
template<typename T>                //只对TD进行声明
class TD;                           //TD == "Type Displayer"
//如果尝试实例化这个类模板就会引出一个错误消息，因为这里没有用来实例化的类模板定义。
TD<decltype(x)> xType;              //引出包含x和y
TD<decltype(y)> yType;              //的类型的错误消息
```

编译器报错(类模板定义没有完整定义)
```C++
error: aggregate 'TD<int> xType' has incomplete type and 
        cannot be defined
error: aggregate 'TD<const int *> yType' has incomplete type and
        cannot be defined
```


### 运行时输出

查看x和y的类型
```C++
std::cout << typeid(x).name() << '\n';  //显示x和y的类型
std::cout << typeid(y).name() << '\n';
```

这种方法对一个对象如x或y调用typeid产生一个std::type_info的对象，然后std::type_info里面的成员函数name()来产生一个C风格的字符串（即一个const char*）表示变量的名字。

**请记住：**

* 类型推断可以从IDE看出，从编译器报错看出，从Boost TypeIndex库的使用看出
* 这些工具可能既不准确也无帮助，所以理解C++类型推导规则才是最重要的