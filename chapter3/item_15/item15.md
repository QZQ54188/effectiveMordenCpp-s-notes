## item15:尽可能的使用‘constexpr’

[item15原文](https://cntransgroup.github.io/EffectiveModernCppChinese/3.MovingToModernCpp/item15.html)

```C++
    const int a = 10;       //编译器常量
    const int b = 1 + 1;    //编译器常量
    int e = 10;
    const int c = e;        //运行时常量
    const int d = get_value();//运行时常量
```

在vscode中，我们只需要将光标放到变量名字上，观察是否存在类似于const int a = 10形式的等号，如果存在的话就是可以在编译器确定的常量，否则就是运行时常量。

在某些特定的场景中必须使用编译期常量，例如在初始化模板的时候
```C++
std::array<int, a> array_a; //正常运行
std::array<int, c> array_c; //报错
```


### constexpr
**constexpr** 可以保证修饰的变量是编译器常量，注意，不可以将一个非编译期的常量赋值给编译期常量。

所有的constexpr都是const，但是并非所有的const都是constexpr。

### constexpr修饰函数

C++11中的constexpr

普通函数：
* 函数的返回值类型不可以是void，必须返回一个值。听起来很受限，但实际上有两个技巧可以扩展constexpr函数的表达能力。第一，使用三元运算符“?:”来代替if-else语句，第二，使用递归代替循环。
* 函数体只可以是一句单纯的return expr;其中expr必须是一个常量表达式，如果函数有形参，那么将形参替换到expr后，expr仍要为常量表达式。
* 如果给constexpr函数传一个运行时的值，那么函数会退化为普通函数。

构造函数：
* 构造函数列表中必须是常量表达式。  
* 构造函数的函数体必须为空。
* 所有和这个类相关的成员，析构函数都必须是默认的。

类成员函数：
* constexpr声明的成员函数具有const属性。

C++14中的constexpr
* 函数的返回值可以是void，不是必须要返回一个常量表达式。
* 函数更加灵活，不止可以只有一句单纯的return expr;
* 构造函数的函数体不必要为空。
* 函数可以修改与函数生命周期相同的对象。

### item15
constexpr修饰编译期可知的变量
```C++
int sz;                             //non-constexpr变量
…
constexpr auto arraySize1 = sz;     //错误！sz的值在
                                    //编译期不可知
std::array<int, sz> data1;          //错误！一样的问题
constexpr auto arraySize2 = 10;     //没问题，10是
                                    //编译期可知常量
std::array<int, arraySize2> data2;  //没问题, arraySize2是constexpr
```

注意const不提供constexpr所能保证之事，因为const对象不需要在编译期初始化它的值。
```C++
int sz;                            //和之前一样
…
const auto arraySize = sz;         //没问题，arraySize是sz的const复制
std::array<int, arraySize> data;   //错误，arraySize值在编译期不可知
```

**简而言之，所有constexpr对象都是const，但不是所有const对象都是constexpr。**

constexpr函数需要注意的点：
* constexpr函数可以用于需求编译期常量的上下文。如果你传给constexpr函数的实参在编译期可知，那么结果将在编译期计算。如果实参的值在编译期不知道，你的代码就会被拒绝。
* 当一个constexpr函数被一个或者多个编译期不可知值调用时，它就像普通函数一样，运行时计算它的结果。这意味着你不需要两个函数，一个用于编译期计算，一个用于运行时计算。constexpr全做了。


示例：
```C++
class Point {
public:
    constexpr Point(double xVal = 0, double yVal = 0) noexcept
    : x(xVal), y(yVal)
    {}

    constexpr double xValue() const noexcept { return x; } 
    constexpr double yValue() const noexcept { return y; }

    void setX(double newX) noexcept { x = newX; }
    void setY(double newY) noexcept { y = newY; }

private:
    double x, y;
};
```

Point的构造函数可被声明为constexpr，因为如果传入的参数在编译期可知，Point的数据成员也能在编译器可知。因此这样初始化的Point就能为constexpr

```C++
constexpr Point p1(9.4, 27.7);  //没问题，constexpr构造函数
                                //会在编译期“运行”
constexpr Point p2(28.8, 5.3);  //也没问题
```

类似的，xValue和yValue的getter（取值器）函数也能是constexpr，因为如果对一个编译期已知的Point对象（如一个constexpr Point对象）调用getter，数据成员x和y的值也能在编译期知道。这使得我们可以写一个constexpr函数，里面调用Point的getter并初始化constexpr的对象：
```C++
constexpr
Point midpoint(const Point& p1, const Point& p2) noexcept
{
    return { (p1.xValue() + p2.xValue()) / 2,   //调用constexpr
             (p1.yValue() + p2.yValue()) / 2 }; //成员函数
}
constexpr auto mid = midpoint(p1, p2);      //使用constexpr函数的结果
                                            //初始化constexpr对象
```
这段代码意味着mid对象通过调用构造函数，getter和非成员函数来进行初始化过程就能在只读内存中被创建出来！也意味着以前相对严格的编译期完成的工作和运行时完成的工作的界限变得模糊，一些传统上在运行时的计算过程能并入编译时。越多这样的代码并入，你的程序就越快。（然而，编译会花费更长时间）

还有个重要的需要注意的是constexpr是对象和函数接口的一部分。加上constexpr相当于宣称“我能被用在C++要求常量表达式的地方”。如果你声明一个对象或者函数是constexpr，客户端程序员就可能会在那些场景中使用它。如果你后面认为使用constexpr是一个错误并想移除它，你可能造成大量客户端代码不能编译。

**请记住：**
* constexpr对象是const，它被在编译期可知的值初始化
* 当传递编译期可知的值时，constexpr函数可以产出编译期可知的结果
* constexpr对象和函数可以使用的范围比non-constexpr对象和函数要大
* constexpr是对象和函数接口的一部分