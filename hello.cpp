#include <iostream>
int main(int argc, const char **argv)
{
    std::cout << "Hello World!" << std::endl;
    for (int i = 0; i < argc; ++i)
    {
        std::cout << "argv[" << i << "]: " << argv[i] << std::endl;
    }
}
