## 理解std::move和std::forward
[item23原文](https://cntransgroup.github.io/EffectiveModernCppChinese/5.RRefMovSemPerfForw/item23.html)

### std::move的实现
1. 传入参数的类型可以是左值或者右值,使用模板和万能引用
2. 本质就是类型转化 **static_cast<type &&>(xxx)**
3. 如何去掉引用，**std::remove_reference<T>::type**

初试编写std::move
```C++
template <typename T>
T &&mymove(T &&param){
    return static_cast<T &&>(param);
}

int main(){
    int aaa = 10;
    int &&bbb = mymove(10);     /*编译通过，因为10本身就是右值，所以T被推到为int，paramtype
    为int &&，正好可以接受10，而且返回的也是右值引用，故成立*/
    int &&ccc = mymove(aaa);    /*因为aaa是左值不可以使用int&&接收，所以T被推导为int &，所以上述模板函数中的T &&全变为int &，所以返回的是int &，不可以使用右值引用接收*/
    return 0;
}
```

将函数改为如此也会报错，因为即使T被推导为int &，T &还是会被折叠为int &而不是int &&
```C++
template <typename T>
T &mymove(T &&param){
    return static_cast<T &>(param);
}
```

尝试使用类型萃取器改写
```C++
template <typename T>
typename std::remove_reference<T>::type &&mymove(T &&param){
    using returnType = typename std::remove_reference<T>::type &&;
    return static_cast<returnType>(param);
}
//编译通过
int main(){
    int aaa = 10;
    int &&bbb = mymove(10);
    int &&ccc = mymove(aaa);
    return 0;
}
```

C++14内部实现
```C++
template <typename T>
decltype(auto) move(T &&param){
    using returnType = std::remove_refrence_t<T> &&;
    return static_cast<returnType>(param);
}
```

### std::move实际做的事情
转化实参为右值(_将亡值_)，对一个对象使用std::move就是告诉编译器，这个对象很**适合**被移动

```C++
struct A{
    int b;
    A(int value) : b(value){
        std::cout<<"create"<<std::endl;
    }

    A(const A&value){
        std::cout<<"copy"<<std::endl;
        b = value.b;
    }

    A(A &&value){
        std::cout<<"move"<<std::endl;
        b = value.b;
        value.b = 0;
    }
};

class Annotation{
public:
    explicit Annotation(const A a) : a_param(std::move(a)){}

private:
    A a_param;
};

int main(){
    Annotation a{10};
    return 0;
}
```

这个函数的执行结果是一次create和一次copy，因为A中定义了一个可以接受int类型的构造函数，所以{10}先去构造了一个A对象，然后再将构造的A对象传给Annotation的构造函数，但是Annotation的构造函数中将a转化为了右值，为什么没有调用move而是调用copy？

> Annotation构造函数接收的参数是const A，由上面的std::move源码可知，const A经过move的值是const A &&，无法匹配上Annotation构造函数中的A &&。这样是为了确保维持const属性的正确性。从一个对象中移动出某个值通常代表着修改该对象，所以语言不允许const对象被传递给可以修改他们的函数（例如移动构造函数）。

从这个例子中，可以总结出两点。第一，不要在你希望能移动对象的时候，声明他们为const。对const对象的移动请求会悄无声息的被转化为拷贝操作。第二点，std::move不仅不移动任何东西，而且它也不保证它执行转换的对象可以被移动。关于std::move，你能确保的唯一一件事就是将它应用到一个对象上，你能够得到一个右值。

### 初探std::forward

std::forward<T>是有条件的move，只有实参使用右值初始化时才转化为右值。
```C++
void process(const A &lval){
    std::cout<<"deal lval"<<std::endl;
}

void process(A &&rval){
    std::cout<<"deal rval"<<std::endl;
}

template <typename T>
void logAndProcess(T &&param){
    process(std::forwar<T>param);
}
/*这个函数会根据传入参数的paramtype不同而去调用不同的左值或者右值函数，如果传入左值就调用第一个函数，传入右值就调用第二个函数*/
```

```C++
Widget w;

logAndProcess(w);               //用左值调用
logAndProcess(std::move(w));    //用右值调用

```

在logAndProcess函数的内部，形参param被传递给函数process。函数process分别对左值和右值做了重载。当我们使用左值来调用logAndProcess时，自然我们期望该左值被当作左值转发给process函数，而当我们使用右值来调用logAndProcess函数时，我们期望process函数的右值重载版本被调用。

但是param，正如所有的其他函数形参一样，是一个左值。每次在函数logAndProcess内部对函数process的调用，都会因此调用函数process的左值重载版本。

为防如此，我们需要一种机制：当且仅当传递给函数logAndProcess的用以初始化param的实参是一个右值时，param会被转换为一个右值。这就是std::forward做的事情。这就是为什么std::forward是一个有条件的转换：它的实参用右值初始化时，转换为一个右值。

### item23

在本章的这些小节中，非常重要的一点是要牢记形参永远是左值，即使它的类型是一个右值引用。比如，假设
void f(Widget&& w);
形参w是一个左值，即使它的类型是一个rvalue-reference-to-Widget。


std::move不移动（move）任何东西，std::forward也不转发（forward）任何东西。在运行时，它们不做任何事情。它们不产生任何可执行代码，一字节也没有。

std::move和std::forward仅仅是执行转换（cast）的函数（事实上是函数模板）。std::move无条件的将它的实参转换为右值，而std::forward只在特定情况满足时下进行转换。

std::move的使用代表着无条件向右值的转换，而使用std::forward只对绑定了右值的引用进行到右值转换。这是两种完全不同的动作。前者是典型地为了移动操作，而后者只是传递（亦为转发）一个对象到另外一个函数，保留它原有的左值属性或右值属性。

**请记住**
* std::move执行到右值的无条件的转换，但就自身而言，它不移动任何东西。
* std::forward只有当它的参数被绑定到一个右值时，才将参数转换为右值。
* std::move和std::forward在运行期什么也不做。