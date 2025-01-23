## item24: 区分通用引用与右值引用

[item24原文](https://cntransgroup.github.io/EffectiveModernCppChinese/5.RRefMovSemPerfForw/item24.html)

T类型的右值引用通常使用T &&表示：
```C++
void f(Widget&& param);             //右值引用
Widget&& var1 = Widget();           //右值引用
auto&& var2 = var1;                 //不是右值引用

template<typename T>
void f(std::vector<T>&& param);     //右值引用

template<typename T>
void f(T&& param);                  //不是右值引用
```

实际上，T &&有两种作用。第一种是右值引用，此时T &&只绑定到右值上，并且他们存在的主要原因就是为了识别可以移动的对象。

T &&的第二种意思是通用引用，此时T &&既可以绑定右值引用和左值引用，它们还可以绑定到const或者non-const的对象上，也可以绑定到volatile或者non-volatile的对象上，甚至可以绑定到既const又volatile的对象上。

那么我们应该怎么判断什么时候产生了通用引用呢，最常见的一种是函数模板形参：
```C++
template<typename T>
void f(T&& param);                  //param是一个通用引用
```

第二种情况是使用auto：
```C++
auto&& var2 = var1;                 //var2是一个通用引用
```

这两种情况的共同之处就是都存在类型推导（type deduction）。在模板f的内部，param的类型需要被推导，而在变量var2的声明中，var2的类型也需要被推导。因此可以说明之前的例子中的右值引用：
```C++
void f(Widget&& param);         //没有类型推导，
                                //param是一个右值引用
Widget&& var1 = Widget();       //没有类型推导，
                                //var1是一个右值引用
```

因为通用引用是引用，所以它们必须被初始化。一个通用引用的初始值决定了它是代表了右值引用还是左值引用。如果初始值是一个右值，那么通用引用就会是对应的右值引用，如果初始值是一个左值，那么通用引用就会是一个左值引用。对那些是函数形参的通用引用来说，初始值在调用函数的时候被提供：
```C++
template<typename T>
void f(T&& param);              //param是一个通用引用

Widget w;
f(w);                           //传递给函数f一个左值；param的类型
                                //将会是Widget&，也即左值引用

f(std::move(w));                //传递给f一个右值；param的类型会是
                                //Widget&&，即右值引用
```

对一个通用引用而言，类型推导是必要的，但是它还不够。引用声明的形式必须正确，并且该形式是被限制的。它必须恰好为“T&&”。再看看之前我们已经看过的代码示例：
```C++
template <typename T>
void f(std::vector<T>&& param);     //param是一个右值引用
```
当函数f被调用的时候，类型T会被推导（除非调用者显式地指定它，这种边缘情况我们不考虑）。但是param的类型声明并不是T&&，而是一个std::vector<T>&&。这排除了param是一个通用引用的可能性。

即使一个简单的const修饰符的出现，也足以使一个引用失去成为通用引用的资格:
```C++
template <typename T>
void f(const T&& param);        //param是一个右值引用
```

由于模板内部并不保证一定会发生类型推导。所以你所看到的T &&并不一定是通用引用：
```C++
template<class T, class Allocator = allocator<T>>   //来自C++标准
class vector
{
public:
    void push_back(T&& x);
    …
}
```

push_back函数在有一个特定的vector实例时不可能存在，所以实例化vector时已经确定了他的类型，不会发生推导，所以此时是右值引用。
```C++
std::vector<Widget> v;

class vector<Widget, allocator<Widget>> {
public:
    void push_back(Widget&& x);             //右值引用
    …
};
```

作为对比，std::vector内的概念上相似的成员函数emplace_back，却确实包含类型推导。因为类型参数Args是独立于vector的类型参数T的，所以Args会在每次emplace_back被调用的时候被推导：
```C++
template<class T, class Allocator = allocator<T>>   //依旧来自C++标准
class vector {
public:
    template <class... Args>
    void emplace_back(Args&&... args);
    …
};
```

**请记住：**
* 如果一个函数模板形参的类型为T&&，并且T需要被推导得知，或者如果一个对象被声明为auto&&，这个形参或者对象就是一个通用引用。
* 如果类型声明的形式不是标准的type&&，或者如果类型推导没有发生，那么type&&代表一个右值引用。
* 通用引用，如果它被右值初始化，就会对应地成为右值引用；如果它被左值初始化，就会成为左值引用。
  
