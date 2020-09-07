#ifndef PBUTILS_H
#define PBUTILS_H

#include <vector>
#include "pb_encode.h"
#include "pb_decode.h"
#include <string>

std::vector<uint8_t> encodePB(const pb_msgdesc_t *fields, const void *src_struct);

pb_bytes_array_t* stringToPBBytes(std::string stringToPack);

void packString(char* &dst, std::string stringToPack);

template <typename T>
T decodePB(const pb_msgdesc_t *fields, std::vector<uint8_t> &data)
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

#endif