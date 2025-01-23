#include <iostream>
#include <vector>
#include <cassert>

template <typename Derived>
struct Base{
    void name(){
        (static_cast<Derived *>(this))->impl();
    }
};

struct D1 : public Base<D1>{
    void impl(){
        std::cout<<"D1::impl"<<std::endl;
    }
};


struct D2 : public Base<D2>{
    void impl(){
        std::cout<<"D2::impl"<<std::endl;
    }
};

template <typename Derived>
void func(Base<Derived> derived){
    derived.name();
}

//CRTP中的基类模板
template <typename E>
class VecExpression {
public:
    //通过将自己static_cast成为子类，调用子类的对应函数实现实现静态多态
    double operator[](size_t i) const { return static_cast<E const&>(*this)[i];     }
    size_t size()               const { return static_cast<E const&>(*this).size(); }
 
};

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

//对应编译树上的二元运算符，将加法表达式构造为VecSum<VecSum... > >的嵌套结构
template <typename E1, typename E2>
VecSum<E1,E2> const operator+(E1 const& u, E2 const& v) {
   return VecSum<E1, E2>(u, v);
}



int main(){
    // Base<D1> derived1;
    // Base<D2> derived2;
    // D1 derived3;
    // D2 derived4;
    // func(derived1);
    // func(derived2);
    // func(derived3);
    // func(derived4);

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


    return 0;
}