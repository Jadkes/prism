#include <vector>
#include <iostream>

int main()
{
    std::vector<int> v = {1, 2, 3, 4, 5};
    int val = v.at(10);
    std::cout << val << std::endl;
    return 0;
}
