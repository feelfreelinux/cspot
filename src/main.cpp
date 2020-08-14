#include <stdio.h>
#include <iostream>
#include <string.h>
#include <PlainConnection.h>
#include <Utils.h>
#include <SHA1.h>

int main()
{
     std::string str = "dd";

     std::vector<uint8_t> vec(str.begin(), str.end());

     auto key = std::string("dd");
     std::cout << SHA1HMAC(vec, key) << std::endl;

    return 0;
}