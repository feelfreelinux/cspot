#ifndef BELL_NANOPB_HELPER_H
#define BELL_NANOPB_HELPER_H

#include <vector>
#include "pb_encode.h"
#include "pb_decode.h"
#include <string>

std::vector<uint8_t> pbEncode(const pb_msgdesc_t *fields, const void *src_struct);

pb_bytes_array_t* vectorToPbArray(const std::vector<uint8_t>& vectorToPack);

void packString(char* &dst, std::string stringToPack);

std::vector<uint8_t> pbArrayToVector(pb_bytes_array_t* pbArray);

template <typename T>
T pbDecode(const pb_msgdesc_t *fields, std::vector<uint8_t> &data)
{

    T result = {};
    // Create stream
    pb_istream_t stream = pb_istream_from_buffer(&data[0], data.size());
    
    // Decode the message
    if (pb_decode(&stream, fields, &result) == false) {
        printf("Decode failed: %s\n", PB_GET_ERROR(&stream));
    }
    return result;
}

template <typename T>
void pbDecode(T &result, const pb_msgdesc_t *fields, std::vector<uint8_t> &data)
{
    // Create stream
    pb_istream_t stream = pb_istream_from_buffer(&data[0], data.size());
    
    // Decode the message
    if (pb_decode(&stream, fields, &result) == false) {
        printf("Decode failed: %s\n", PB_GET_ERROR(&stream));
    }
}

void pbFree(const pb_msgdesc_t *fields, void *src_struct);

#endif