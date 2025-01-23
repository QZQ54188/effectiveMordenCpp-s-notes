## 理解模板类型推导
[条款一原文](https://cntransgroup.github.io/EffectiveModernCppChinese/1.DeducingTypes/item1.html)
### base1(理解底层const和顶层const)
个人理解:_只要新定义的量保证不能修改之前定义过的常量就合法。_

``` c++
const(底层const) int *const(顶层const) p
```

>**_《C++ Primer》_ p58**:在执行对象拷贝操作时，常量的顶层const不受什么影响，但是底层const必须具有相同的资格。

```C++
    const int *const p = new int(10);
    const int a = 10;
    int b = a;

    int *p1 = p;        //非法
    int *const p2 = p;  //非法
    const int *p3 = p;  //合法

    const int &r1 = 20; //合法
    const int &r2 = b;  //合法
    int &r3 = a;        //非法
    int c = r1;         //合法
```

#### 不涉及引用的const
* 在C语言与C++中，const只是编译时的约束，编译器会检查是否违反了这个约束（比如试图通过const指针修改数据）。但是，const本身不会在运行时强制保护内存，也就是说，从内存的角度来说，const的数据是可以被程序通过某些手段修改的。**(修改const修饰的变量可能会导致未定义行为)**
```C
    const int x = 10;
    int *ptr = (int *)&x;
    *ptr = 20; // 编译时不会报错，但是修改了常量
```
编译运行之后*ptr的值为20，但是x的值为10，这是为什么，ptr不是指向x吗，为什么\*ptr的值和x的值不一致？
>*ptr 显示为 20 是因为通过指针修改了内存中的值。
> x 显示为 10 是因为 const 变量的值通常在编译时被固定，即使内存中的值被修改，编译器仍然认为它保持原值。

#### 涉及引用的const

##### 为什么使用引用
* 引用不会创建数据的副本，而是直接指向原始数据。对于大对象（如大型数组、容器、结构体等），如果使用拷贝传递或返回，系统会创建一个新的副本，这会增加内存使用和时间开销。
* 当传递大对象时，使用引用可以避免额外的内存分配。拷贝会导致额外的内存开销，因为需要为数据创建新的副本，而引用只是传递一个指向原数据的指针，节省了内存空间。
* 如果引用是非const的，允许通过引用直接修改原数据。在某些情况下，修改原数据是必要的，而不需要返回一个副本。
  
##### 引用与const
* 表达式左边是常量引用的话可以接任何值(立即数或者变量)
```C++
int a = 10;
const int &r1 = 10;     //合法,主要用于传参数
const int &r2 = a;     //合法
```
* 非常量引用 = 常量，报错(原因在于可以改变const修饰的内存块)
```C++
const int a = 10;
int &r3 = a;        //报错
```
* 非常量 = 常量引用，不报错
```C++
const int &a = 10;
int b = a;
```
  


### base2(值类型与右值引用)

#### 函数返回中拷贝的低效
```C++
int geta(){
    int a = 10;
    return a;
}
int x = a;
```
这个函数会发生**两次拷贝**
(使用**fno-elide-constructors**编译没有返回值优化的情况)
```C++
int temp = a;
int x = temp;
```
* 假如有一块大内存a，我们在运行上述函数时会将这块大内存a拷贝到temp中，然后再将temp拷贝给x，之后temp和a这两个对象被立刻析构，效率十分低。

```C++
class BigMemoryPool
{
private:
    char *pool_;
public:
    static const int PoolSize = 4096;
    BigMemoryPool() : pool_(new char[PoolSize]){}

    ~BigMemoryPool(){
        if(pool_ != nullptr){
            delete[] pool_;
        }
    }
    BigMemoryPool(const BigMemoryPool &other) : pool_(new char[PoolSize]){
        std::cout<<"copy"<<std::endl;
        memcpy(pool_, other.pool_, PoolSize);
    }
};

BigMemoryPool getPool(){
    BigMemoryPool memorypool;
    return memorypool;
}

int main(){
    int x = 10;
    BigMemoryPool bbb = getPool();
    return 0;
}
```
在编译时使用-fno-elide-constructors关闭返回值优化，那么上述代码的运行结果会发生两次copy，与上述分析对应。

#### C++中的表达式类型
* **左值**:指向特定内存的具名对象，可以取地址
* **右值**:临时对象，字符串除外的字面量，不可取地址


```C++
int x = 10;
int *p1 = &x++; //非法
int *p2 = &++x; //合法
```
原因如下，将x++和++x的逻辑分别改写为funca和funcb
```C++
int funca(int &a){
    int b = a;
    a += 1;
    return b;
}
int &funcb(int &a){
    a += 1;
    return a;
}
```
* 由上述分析可知，funca返回的是临时变量，是一个右值，无法取地址。但是funcb返回的是引用，是一个左值，可以取地址。

#### 移动构造函数应运而生
* 拷贝构造函数具有低效的缺点
* 引用虽然不需要拷贝，但是终究只是别名，对引用对象进行修改，最终会体现在引用和被引用的变量身上，不太方便
* 移动构造通过转移资源的所有权，避免了重复的内存分配和释放。在移动构造函数中，源对象的资源被“清空”或标记为空，而不是销毁，这样新对象就能直接接管资源，避免了不必要的资源管理。


**移动语义**:std::move(value)可以将左值转化为右值。

```C++
//上述类的移动构造函数
BigMemoryPool(BigMemoryPool &&other){
        std::cout << "move" << std::endl;
        pool_ = other.pool_;
        other.pool_ = nullptr;
    }

BigMemoryPool aaa;
std::cout<<"a has been constructed"<<std::endl;
BigMemoryPool ccc = std::move(aaa);
//输出结果为一次move

BigMemoryPool getPool(const BigMemoryPool &pool){
    std::cout<<"return pool"<<std::endl;
    return pool;
}

BigMemoryPool make_pool(){
    BigMemoryPool pool;
    std::cout<<"pool con"<<std::endl;
    return getPool(pool);
}
BigMemoryPool ccc = make_pool();
/*输出结果为一次copy和两次move，第一次copy发生在getpool函数返回时将引用拷贝到返回的右值中，第一次move发生在make_pool函数返回时，将getpool函数返回的右值移动到当前函数准备返回的右值中，第二次move发生在make_pool函数返回的右值移动给ccc变量*/
```

<mark>类中未实现移动构造，std::move()之后仍是拷贝。</mark>

<mark>右值绑定到右值引用上什么事情都不会发生，就相当于取别名。</mark>


### base3(函数指针与数组指针)

#### 数组指针
```C++
int array[5] = {1,2,3,4,5};
int *ptr = array;   //数组名退化为指针
int (*ptr1)[5] = &array;//ptr1的类型为int(*)[5]
int (&ref)[5] = array;  //ref的类型为int(&)[5]
```

* 数组作为函数参数会退化为指针 
```C++
void fun(int a[100]);
void fun1 (int a[]);
void fun2 (int *a);
void fun3 (int (*a)[100]);
void fun4 (int (*a)[5]);
```

fun,fun1,fun2三个函数的参数均为int \*a,故这三个函数等价。但是fun3和fun4的函数参数分别为int(*)[100]和int(\*)[5]。

#### 函数指针
```C++
bool fun (int a, int b);//类型bool (int a, int b)
bool (*funptr)(int a, int b);//类型bool (*) (int a, int b),相当于声明指针变量
//函数指针作为形参
bool func(int a, bool (*funptr)(int, int));
```

* 由于C++中函数指针的使用方法很难记忆，因此推荐使用类型别名的方式将函数指针定义为一个指针类型使用，便于理解
    ```C++
    using funptr = bool (*) (int, int);
    using fun = bool (int, int);
    ```

### item1
#### _一些补充知识_
* 函数参数类型推导
```C++
void fun(int a){}
void fun(const int a){}
```
在上面这两个函数中，若把光标放在下面这个函数的函数名上面，编译器会显示这个函数的参数是**int a**，而不是**const int a**。
>ChatGpt:C++ 中，const 修饰符应用于函数参数时，它并不会改变该参数的类型。具体来说，当你声明一个函数 void fun(const int a) 时，const int 的作用仅仅是告诉编译器 a 是一个常量，意味着你在函数内部不能修改 a 的值。然而，const 并不改变 a 的基本类型，因此它仍然是一个 int 类型。因此，编译器视 const int 为 int，并不会因为 const 的存在而产生不同的行为，除非在函数体内尝试修改 a 的值。

* 函数指针与函数引用

正常用法如下
```C++
int func1(int a){return a;}
int (*func1ptr)(int) = &func1;
```
既然函数指针是一个指针，那么他一定也有底层const和顶层const
```C++
int func1(int a){return a;}
//顶层const
int (*const func1ptr)(int) = &func1;
//底层const报错:int(*)(int)类型无法用于初始化const int(*)(int)类型
const int (*func1ptr)(int) = &func1;
```
但是如果使用类型别名的方式就可以同时使用底层和顶层const
```C++
using F = int(int);
const F *aaa = &func1;
F *const bbb = &func1;
```
>个人理解：因为在编译链接的过程中，函数中的代码必定会被加载到.text这段segment，而且这段segment在内存中的权限是只读，那么当然函数指针指向的函数的内容肯定不会改变。

#### 模板类型推导

>《effective morden cpp》：类型推导的广泛应用，让你从拼写那些或明显或冗杂的类型名的暴行中脱离出来。它让C++程序更具适应性，因为在源代码某处修改类型会通过类型推导自动传播到其它地方。但是类型推导也会让代码更复杂，因为由编译器进行的类型推导并不总是如我们期望的那样进行。

考虑函数模板
```C++
template <typename T>
void f(ParamType param);

f(expr);                        //使用表达式调用f
```

在编译期间，编译器使用expr进行两个类型推导：一个是针对T的，另一个是针对ParamType的。这两个类型通常是不同的，因为ParamType包含一些修饰，比如const和引用修饰符。
```C++
template<typename T>
void f(const T& param);         //ParamType是const T&
```

##### case1:ParamType是一个指针或引用，但不是通用引用

在这种情况下，类型推导会这样进行：
1. 如果expr的类型是一个引用，忽略引用部分
2. 然后expr的类型与ParamType进行模式匹配来决定T
   
```C++
template<typename T>
void f(T& param);               //param是一个引用

int x=27;                       //x是int
const int cx=x;                 //cx是const int
const int& rx=x;                //rx是指向作为const int的x的引用

f(x);                           //T是int，param的类型是int&
f(cx);                          //T是const int，param的类型是const int&
f(rx);                          //T是const int，param的类型是const int&
```

>《effective morden cpp》:在第二个和第三个调用中，注意因为cx和rx被指定为const值，所以T被推导为const int，从而产生了const int&的形参类型。这对于调用者来说很重要。当他们传递一个const对象给一个引用类型的形参时，他们期望对象保持不可改变性，也就是说，形参是**reference-to-const**的。这也是为什么将一个const对象传递给以T&类型为形参的模板安全的：对象的常量性constness会被保留为T的一部分。
>在第三个例子中，注意即使rx的类型是一个引用，T也会被推导为一个非引用 ，这是因为**rx的引用性（reference-ness）在类型推导中会被忽略**。
>这些例子只展示了左值引用，但是类型推导会如左值引用一样对待右值引用。当然，右值只能传递给右值引用，但是在类型推导中这种限制将不复存在。

##### base2:情景二：ParamType是一个通用引用

* 如果expr是左值，T和ParamType都会被推导为左值引用。这非常不寻常，第一，这是模板类型推导中唯一一种T被推导为引用的情况。第二，虽然ParamType被声明为右值引用类型，但是最后推导的结果是左值引用。
* 如果expr是右值，就使用正常的（也就是情景一）推导规则

```C++
template<typename T>
void f(T&& param);              //param现在是一个通用引用类型
        
int x=27;                       //如之前一样
const int cx=x;                 //如之前一样
const int & rx=cx;              //如之前一样

f(x);                           //x是左值，所以T是int&，
                                //param类型也是int&

f(cx);                          //cx是左值，所以T是const int&，
                                //param类型也是const int&

f(rx);                          //rx是左值，所以T是const int&，
                                //param类型也是const int&

f(27);                          //27是右值，所以T是int，
                                //param类型就是int&&
```

##### base3:ParamType既不是指针也不是引用

当ParamType既不是指针也不是引用时，我们通过传值（pass-by-value）的方式处理：
1. 和之前一样，如果expr的类型是一个引用，忽略这个引用部分。
2. 如果忽略expr的引用性（reference-ness）之后，expr是一个const，那就再忽略const。

```C++
int x=27;                       //如之前一样
const int cx=x;                 //如之前一样
const int & rx=cx;              //如之前一样

f(x);                           //T和param的类型都是int
f(cx);                          //T和param的类型都是int
f(rx);                          //T和param的类型都是int
```

**注意即使cx和rx表示const值，param也不是const。这是有意义的。param是一个完全独立于cx和rx的对象——是cx或rx的一个拷贝(_passed-by-value_)。这就是为什么expr的常量性constness（或易变性volatileness）在推导param类型时会被忽略：因为expr不可修改并不意味着它的拷贝也不能被修改。**


### 总结
* <mark>在模板类型推导时，有引用的实参会被视为无引用，他们的引用会被忽略</mark>
* <mark>对于通用引用的推导，左值实参会被特殊对待</mark>
* <mark>对于传值类型推导，const和/或volatile实参会被认为是non-const的和non-volatile的
在模板类型推导时，数组名或者函数名实参会退化为指针，除非它们被用于初始化引用</mark>
