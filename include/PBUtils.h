#ifndef PBUTILS_H
#define PBUTILS_H

#include <vector>
#include "pb_encode.h"
#include "pb_decode.h"

std::vector<uint8_t> encodePB(const pb_msgdesc_t *fields, const void *src_struct);

template <typename T>
T decodePB(const pb_msgdesc_t *fields, std::vector<uint8_t> &data)
{

    T result = {};
    // Create stream
    pb_istream_t stream = pb_istream_from_buffer(&data[0], data.size());

    // Decode the message
    pb_decode(&stream, fields, &result);
    return result;
}

#endif