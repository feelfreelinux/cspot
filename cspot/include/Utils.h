#ifndef UTILS_H
#define UTILS_H

#include <unistd.h>
#include <vector>
#include "sys/socket.h"
#include <cstdint>
#include <netdb.h>
#include <cstring>
#include <memory>
#include <chrono>
#include <string>
#include <sstream>
#include <iostream>
#include <iomanip>

#define HMAC_SHA1_BLOCKSIZE 64

/**
 * @brief Returns current timestamp
 * 
 * @return unsigned long long resulting timestamp in milliseconds from unix time zero
 */
unsigned long long getCurrentTimestamp();

/**
 * @brief portable 64bit equivalent of htons / htonl. aka endianess swap
 * 
 * @param value input value to swap
 * @return uint64_t swapped result
 */
uint64_t hton64(uint64_t value);

/**
 * @brief Performs big number multiplication on two numbers
 * 
 * @param num big num in vector format
 * @param n secondary number
 * @return std::vector<uint8_t> resulting number
 */
std::vector<uint8_t> bigNumMultiply(std::vector<uint8_t> num, int n);

/**
 * @brief Performs big number addition on two numbers
 * 
 * @param num big num in vector format
 * @param n secondary number
 * @return std::vector<uint8_t> resulting number
 */
std::vector<uint8_t> bigNumAdd(std::vector<uint8_t> num, int n);

unsigned char h2int(char c);


std::string urlDecode(std::string str);


/**
 * @brief Converts provided bytes into a human readable hex string
 * 
 * @param bytes vector containing binary data
 * @return std::string string containing hex representation of inputted data
 */
std::string bytesToHexString(std::vector<uint8_t> &bytes);

/**
 * @brief Extracts given type from binary data
 * 
 * @tparam T type to extract
 * @param v vector containing binary data to extract from
 * @param pos position offset
 * @return T extracted type
 */
template <typename T>
T extract(const std::vector<unsigned char> &v, int pos)
{
  T value;
  memcpy(&value, &v[pos], sizeof(T));
  return value;
}

/**
 * @brief Packs given type into binary data
 * 
 * @tparam T type of data to pack
 * @param data data to pack
 * @return std::vector<uint8_t> resulting vector containing binary data
 */
template <typename T>
std::vector<uint8_t> pack(T data)
{
     std::vector<std::uint8_t> rawData( (std::uint8_t*)&data, (std::uint8_t*)&(data) + sizeof(T));

     return rawData;
}


#endif