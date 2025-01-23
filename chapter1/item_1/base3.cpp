#include <iostream>

int main(){
    
    int array[5] = {1,2,3,4,5};
    int *ptr = array;
    int (*ptr1)[5] = &array;
    int (&ref)[5] = array;

    return 0;
}