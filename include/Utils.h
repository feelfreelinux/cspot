#ifndef UTILS_H
#define UTILS_H

#include <unistd.h>
#include <vector>
#include <sys/socket.h>
#include <sys/types.h>
#include <cstdint>
#include <netdb.h>
#include <cstring>
#include <memory>

#ifdef __APPLE__
#include <libkern/OSByteOrder.h>
#define htobe64(x) OSSwapHostToBigInt64(x)
#define htole64(x) OSSwapHostToLittleInt64(x)
#endif

#define HMAC_SHA1_BLOCKSIZE 64

std::vector<uint8_t> blockRead(int fd, size_t readSize);

ssize_t blockWrite (int fd, std::vector<uint8_t> data);

std::vector<uint8_t> SHA1HMAC(std::vector<uint8_t> &inputKey, std::vector<uint8_t> &message);

// Reads a type from vector of binary data
template <typename T>
T extract(const std::vector<unsigned char> &v, int pos)
{
  T value;
  memcpy(&value, &v[pos], sizeof(T));
  return value;
}

// Writes a type to vector of binary data
template <typename T>
std::vector<uint8_t> pack(T data)
{
     std::vector<std::uint8_t> rawData( (std::uint8_t*)&data, (std::uint8_t*)&(data) + sizeof(T));

     return rawData;
}


#endif