## 优先考虑别名声明而非typedef
[item9原文](https://cntransgroup.github.io/EffectiveModernCppChinese/3.MovingToModernCpp/item9.html)
### typename关键字

用来澄清模板内部的“T”标识符代表的是某种类型，避免歧义

* 为什么会产生歧义？
> 当编译器遇到类似T::xxx这样的代码时，它不知道xxx是一个数据成员还是一个类型成员，直到实例初始化时才知道，但是为了处理模板，编译器必须知道名字是否表示一个类型，默认情况下，C++语言假设通过作用域运算符访问的名字不是类型

```C++
struct test1{
    static int SubType;
};

struct test2{
    typedef int SubType;
};

template <typename T>
class MyClass{
public:
    void foo(){
        T::SubType *ptr;
    }
};

int main(){
    MyClass<test1> t1.foo();    /*编译器将其视为乘法，相当于int类型的数据成员
   SubType乘以ptr，但是ptr在之前没有声明，所以会报错*/ 
    MyClass<test2> t2.foo();    //编译器将其视为基本类型int，创建一个指向int的指针
}
```

改进措施
```C++
template <typename T>
class MyClass{
public:
    void foo(){
        typename T::SubType *ptr;   //typename键字表示T::SubType一定是类型
    }
};
```

### 对于模板来说,using比typedef更好用

1. 对于模板使用using定义比使用typedef定义更加简单
    ```C++
    template <class T>
    using myVector = std::vector<T>;

    template <typename T>
    struct myVector2{
        typedef std::vector<T> type;
    };
    
    int main(){
        myVector<int> v1;
        myVector2<int>::type v2;
        return 0;
    }
    ```
    typedef 在模板中使用时需要额外的包裹，如结构体或类。

2. 使用using可以避免歧义，因为不需要使用作用域运算符
    ```C++
    template <class T>
    using myVector = std::vector<T>;

    template <typename T>
    struct myVector2{
        typedef std::vector<T> type;
    };


    template <typename T>
    class Widget{
    public:
        myVector<T> list;
    };

    template <typename T>
    class Widget2{
    public:
        typename myVector2<T>::type list;
    };
    ```
    因为如果使用typedef的话，我们必须使用作用域运算符，那么按照上面说的，C++会假设type是数据成员而不是数据类型，所以必须加typename修饰避免歧义。

### item9

使用using声明函数指针比使用typedef更加易于理解
```C++
/FP是一个指向函数的指针的同义词，它指向的函数带有
//int和const std::string&形参，不返回任何东西
typedef void (*FP)(int, const std::string&);    //typedef

//含义同上
using FP = void (*)(int, const std::string&);   //别名声明
```

因为别名声明可以被模板化，但是typedef不能，所以typedef只可以被嵌套在类或者结构体中，使用它们的模板化
```C++
template<typename T>                            //MyAllocList<T>是
using MyAllocList = std::list<T, MyAlloc<T>>;   //std::list<T, MyAlloc<T>>
                                                //的同义词

MyAllocList<Widget> lw;                         //用户代码


template<typename T>                            //MyAllocList<T>是
struct MyAllocList {                            //std::list<T, MyAlloc<T>>
    typedef std::list<T, MyAlloc<T>> type;      //的同义词  
};

MyAllocList<Widget>::type lw;                   //用户代码
```

#### 类型萃取器
用于添加或者删除模板T的修饰，需要包含头文件<type_traits>
```C++
std::remove_const<T>::type          //从const T中产出T
std::remove_reference<T>::type      //从T&和T&&中产出T
std::add_lvalue_reference<T>::type  //从T中产出T&
```

如果你在一个模板内部将他们施加到类型形参上，你也需要在它们前面加上typename。至于为什么要这么做是因为这些C++11的type traits是通过在struct内嵌套typedef来实现的。直到C++14它们才提供了使用别名声明的版本。

```C++
std::remove_const<T>::type          //C++11: const T → T 
std::remove_const_t<T>              //C++14 等价形式

std::remove_reference<T>::type      //C++11: T&/T&& → T 
std::remove_reference_t<T>          //C++14 等价形式

std::add_lvalue_reference<T>::type  //C++11: T → T& 
std::add_lvalue_reference_t<T>      //C++14 等价形式
```

C++14使用了别名声明嵌套了一层C++11中的typedef
```C++
template <class T> 
using remove_const_t = typename remove_const<T>::type;

template <class T> 
using remove_reference_t = typename remove_reference<T>::type;

template <class T> 
using add_lvalue_reference_t =
  typename add_lvalue_reference<T>::type; 
```

**请记住**
* typedef不支持模板化，但是别名声明支持。
* 别名模板避免了使用“::type”后缀，而且在模板中使用typedef还需要在前面加上typename
* C++14提供了C++11所有type traits转换的别名声明版本


