## item6：auto推导若非己愿，使用显式类型初始化惯用法

[item6原文](https://cntransgroup.github.io/EffectiveModernCppChinese/2.Auto/item6.html)

[参考博客](https://blog.csdn.net/HaoBBNuanMM/article/details/109740504)

### 奇异递归模板模式(CRTP)
它的范式基本上都是：父类是模板，再定义一个类类型继承它。因为类模板不是类，只有实例化后的类模板才是实际的类类型，所以我们需要实例化它，**显式指明模板类型参数，而这个类型参数就是我们定义的类型，也就是子类了。**
```C++
template <class Dervied>
class Base {};

class X : public Base<X> {};
```

CRTP可以在父类中暴露接口，而在子类中实现接口，以此实现**编译期多态**
```C++
template <class Dervied>
class Base {
public:
    // 公开的接口函数 供外部调用
    void addWater(){
        // 调用子类的实现函数。要求子类必须实现名为 impl() 的函数
        // 这个函数会被调用来执行具体的操作
        static_cast<Dervied*>(this)->impl();
    }
};

class X : public Base<X> {
public:
    // 子类实现了父类接口
    void impl() const{
        std::cout<< "X 设备加了 50 毫升水\n";
    }
};
```


那么好，问题来了，为什么呢？ static_cast<Dervied*>(this) 是做了什么，它为什么可以这样？

* 很显然 static_cast<Dervied*>(this) 是进行了一个类型转换，将 this 指针（也就是父类的指针），转换为通过模板参数传递的类型，也就是子类的指针。

* 这个转换是安全合法的。因为 this 指针实际上指向一个 X 类型的对象，X 类型对象继承了 Base<X> 的部分，X 对象也就包含了 Base<X> 的部分，所以这个转换在编译期是有效的，并且是合法的。

* 当你调用 x.addWater() 时，实际上是 X 对象调用了父类 Base<X> 的成员函数。这个成员函数内部使用 static_cast<X*>(this)，将 this 从 Base<X>* 转换为 X*，然后调用 X 中的 impl() 函数。这种转换是合法且安全的，且 X 确实实现了 impl() 函数。

### Expression Templates

考虑下面的代码
```C++
Vec v0 = {1, 1, 1};
Vec v1 = {2, 2, 2};
Vec v2 = {3, 3, 3};
auto v4 = v0 + v1 + v2;
```
在上述加法中，我们需要在Vec类中重载+号运算符，因为+号运算符重载之后操作数只有两个，所提在上述代码中，先申请一块临时内存，将v0+v1的结果存储到临时内存中，然后再将这块临时内存返回出来继续与v2相加，最后将v4返回。

**如果数据量变大，那么上述代码将变得十分低效。**

假设有20个向量相加，每个向量中有1000个元素。最高效的方法是对索引循环1000次，每次循环将20个向量对应位置的索引全部相加，放到结果对应的索引位置。但是重载+号运算符只可以有两个操作数，所以只可以先对索引循环1000次，将v0和v1相加，放到申请的临时变量上，然后将临时变量返回出去，再与v2相加，同样需要循环索引和申请临时空间......不仅涉及到大量循环，还需要不断申请和释放临时空间，效率释放低效。


还有一种场景，我们还是假设有20个向量相加，每个向量中有1000个元素。这时我们只想得到结果向量中的特定几项，例如v[25],那么我们没必要去循环1000次，只需要简单的将20个向量中特定元素相加。
如果我们具有延时计算表达式的能力，那么我们在没使用v[25]之前就不会使用他，假如出现了要使用v[25]的代码
```C++
int aaa = v[25];
```
此时我们才需要计算v[25]，这样效率会提高很多。

如何实现上述说明的延迟计算和重载+号运算符多操作数加法？

```C++
//CRTP中的基类模板
template <typename E>
class VecExpression {
public:
    //通过将自己static_cast成为子类，调用子类的对应函数实现实现静态多态
    double operator[](size_t i) const { return static_cast<E const&>(*this)[i];     }
    size_t size()               const { return static_cast<E const&>(*this).size(); }
 
};
```

静态多态通常指的是在编译时决定调用哪个函数，这种决定在编译时已经确定，不依赖于运行时的动态分配。典型的实现方式有两种：
* 模板（Templates）：通过模板编程实现的多态，函数和类的选择在编译时根据传入的类型进行静态绑定。
* static_cast 强制类型转换：通过显式的类型转换，选择使用不同类中的方法。这种方式会强制转换类型，并依赖编译器的模板实例化或类型推断来决定调用的具体函数。

动态多态是通过 虚函数（virtual functions） 和 继承 机制实现的，它是在 运行时 通过虚表（vtable）来决定调用哪个函数。


静态多态与动态多态的对比：
* 静态多态在编译时决定，基于编译时的类型信息。动态多态是在运行时进行的，基于虚函数机制（vtable）和实际对象类型进行动态绑定。
* 静态多态由于编译器已经在编译时决定了函数的调用，静态多态通常不涉及运行时的开销。编译器在生成代码时直接选择函数，因此效率较高。动态多态由于需要在运行时进行虚函数表的查找，动态多态通常会有轻微的性能开销，尤其是在多次调用虚函数时。


```C++
//将自己作为基类模板参数的子类 - 对应表达式编译树中的叶节点
class Vec : public VecExpression<Vec> {
    std::vector<double> elems;
 
  public:
    double operator[](size_t i) const { return elems[i]; }
    double &operator[](size_t i)      { return elems[i]; }
    size_t size() const               { return elems.size(); }
 
    Vec(size_t n) : elems(n) {}
 
    Vec(std::initializer_list<double>init){
        for(auto i:init)
            elems.push_back(i);
    }
 
    //赋值构造函数可以接受任意父类VecExpression的实例，并且进行表达式的展开（对应表达式编译树中的赋值运算符节点）
    template <typename E>
    Vec(VecExpression<E> const& vec) : elems(vec.size()) {
        for (size_t i = 0; i != vec.size(); ++i) {
            elems[i] = vec[i];
        }
    }
};
```

![编译树](https://i-blog.csdnimg.cn/blog_migrate/885b8531b7c266da2093a935283ded46.png)

上述代码构造了一个Vec类，但是该类继承了CRTP中的基类模板，并且基类模板使用Vec类本身进行初始化。

```C++
//将自己作为基类模板参数的子类 - 对应表达式编译树中的二元运算符输出的内部节点
//该结构的巧妙之处在于模板参数E1 E2可以是VecSum，从而形成VecSum<VecSum<VecSum ... > > >的嵌套结构，体现了表达式模板的精髓：将表达式计算改造成为了构造嵌套结构
template <typename E1, typename E2>
class VecSum : public VecExpression<VecSum<E1, E2> > {
    E1 const& _u;
    E2 const& _v;
public:
    VecSum(E1 const& u, E2 const& v) : _u(u), _v(v) {
        assert(u.size() == v.size());
    }
 
    double operator[](size_t i) const { return _u[i] + _v[i]; }
    size_t size()               const { return _v.size(); }
};
```


```C++
//对应编译树上的二元运算符，将加法表达式构造为VecSum<VecSum... > >的嵌套结构
template <typename E1, typename E2>
VecSum<E1,E2> const operator+(E1 const& u, E2 const& v) {
   return VecSum<E1, E2>(u, v);
}
```
在重载+号运算符中，我们并没有直接进行计算，而是将构造了一个嵌套结构对象，我们在[]号运算符的重载中才实现了+法操作，因此可以延迟计算。


main函数实例
```C++
//创建3个叶子节点
    Vec v0 = {1.0, 1.0, 1.0};
    Vec v1 = {2.0, 2.0, 2.0};
    Vec v2 = {3.0, 3.0, 3.0};
    
    //构建表达式 v0 + v1 + v2，赋值给v3，编译阶段形成表达式树
    auto v4 = v0 + v1;
    auto v3 = v0 + v1 + v2;
    
    //输出结算结果
    for (int i = 0; i < v3.size(); i ++) {
        std::cout <<" "<< v3[i];
    }
    std::cout << std::endl;
```

![CRTP继承关系](https://i-blog.csdnimg.cn/blog_migrate/36d06f13f1a5013bd4a28f605ea0e32e.png)

![加法树形关系](https://i-blog.csdnimg.cn/blog_migrate/1d044d6961b26f9b48e186fb61b614fc.png)


### item6

