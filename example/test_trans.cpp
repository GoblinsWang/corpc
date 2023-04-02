#include <iostream>
#include <vector>
#include <algorithm>

int main()
{
    // 定义输入和输出向量
    std::string s1 = "hello world";
    std::string s2(s1.size(), ' ');
    // 使用 std::transform() 将每个元素乘以 2 并存储到输出向量中
    std::transform(s1.begin(), s1.end(), s2.begin(), toupper);

    // 输出结果
    for (char i : s2)
    {
        std::cout << i << " ";
    }
    std::cout << std::endl;

    return 0;
}
