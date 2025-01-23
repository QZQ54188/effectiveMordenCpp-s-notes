#include <iostream>
#include <vector>

template<typename Container, typename Index>    //可以工作，
auto testFun(Container& c, Index i)       //但是需要改良
    ->decltype(c[i])
{
    return c[i];
}

template<typename Container, typename Index>    //C++14版本，
auto                               //可以工作，
test_error(Container& c, Index i)            //但是还需要
{                                               //改良
    return c[i];
}

template<typename Container, typename Index>    //C++14版本，
decltype(auto)                                  //可以工作，
test_right(Container& c, Index i)            //但是还需要
{                                               //改良
    return c[i];
}

template <typename T>
void printVector(const std::vector<T>& vec) {
    std::cout << "[ ";
    for (const auto& element : vec) {
        std::cout << element << " ";
    }
    std::cout << "]" << std::endl;
}

int main(){
    int a = 10;
    // decltype(a) b;      //b被推导为int
    // int &aref = a;
    // decltype(aref) bref = a;//bref被推导为int &
    // const int &arefc = a;
    // decltype(arefc) brefc = a;//brefc被推导为const int &
    // const int *const aptr = &a;
    // decltype(aptr) bptr = &a;//bptr被推导为const int const *
    // const int &&carref = std::move(a);
    // decltype(carref) cbrref;//cbrref被推导为const int &&
    // int array[2] = {1, 2};
    // decltype(array)arr;
    // int *aptr = &a;
    // decltype(*aptr) b = a;//int&
    // decltype(a) b1; //int
    // decltype((a)) b2;   //int&
    // int *const acptr = &a;
    // decltype(*acptr) aaa;
    // const int *captr = &a;
    // decltype(*captr) bbb;
    // const int *const cacptr = &a;
    // decltype(*cacptr) ccc;

    std::vector<bool> vec = {true, false, true};
    decltype(vec[1]) value1;
    std::vector<int> vec1 = {1,2,3};
    decltype(vec1[1]) value2 = a;

    // testFun(vec, 1) = true;
    // testFun(vec1, 1) = 5;

    // test_error(vec, 1) = true;
    // test_error(vec1, 1) = 5;

    test_right(vec, 1) = true;
    test_right(vec1, 1) = 5;

    printVector(vec);
    printVector(vec1);

    return 0;
}