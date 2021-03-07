#include <iostream>

#include "tools/json.hpp"

using json = nlohmann::json;

int main()
{
    json j;

    j["pi"] = 3.141;
    j["happy"] = true;

    std::cout << j.dump(4) << std::endl;


    return 0;
}