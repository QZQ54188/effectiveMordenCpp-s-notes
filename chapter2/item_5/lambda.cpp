#include <iostream>
#include <string>
#include <memory>

int a = 0;
class SizeComp{
public:
    SizeComp(size_t n) : sz(n) {};
    bool operator() (const std::string &s) const 
    {
        return s.size() > sz;
    }

private:
    size_t &sz;
};

int main(){
    return 0;
}