#include <iostream>
#include <vector>
#include <string>
using namespace std;

size_t split(const std::string& filename, const std::string& sep, std::vector<std::string>* ret)
{
    std::size_t previous = 0;
    std::size_t current = filename.find(sep);
    while (current != std::string::npos)
    {
        if (current > previous)
        {
            ret->push_back(filename.substr(previous, current - previous));
        }
        previous = current + 1;
        current = filename.find(sep, previous);
    }
    if (previous != filename.size())
    {
        ret->push_back(filename.substr(previous));
    }
    return ret->size();
}

int main()
{
    string str = "abc,,,,def,deg,";
    vector<string> ret;
    split(str, ",", &ret);
    for (auto &i : ret)
    {
        cout << i << endl;
    }
    cout << ret.size() << endl;
    return 0;
}