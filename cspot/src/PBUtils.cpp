#include "PBUtils.h"

static bool vectorWrite(pb_ostream_t *stream, const pb_byte_t *buf, size_t count)
{
    size_t i;
    auto *dest = reinterpret_cast<std::vector<uint8_t> *>(stream->state);

    dest->insert(dest->end(), buf, buf + count);

    return true;
}

pb_ostream_t pb_ostream_from_vector(std::vector<uint8_t> &vec)
{
    pb_ostream_t stream;

    stream.callback = &vectorWrite;
    stream.state = &vec;
    stream.max_size = 100000;
    stream.bytes_written = 0;

    return stream;
}

std::vector<uint8_t> encodePB(const pb_msgdesc_t *fields, const void *src_struct)
{
    std::vector<uint8_t> vecData(0);
    pb_ostream_t stream = pb_ostream_from_vector(vecData);
    pb_encode(&stream, fields, src_struct);

    return vecData;
}

void packString(char *&dst, std::string stringToPack)
{
    dst = (char *)malloc((strlen(stringToPack.c_str()) + 1) * sizeof(char));
    strcpy(dst, stringToPack.c_str());
}

pb_bytes_array_t* vectorToPbArray(const std::vector<uint8_t>& vectorToPack)
{
    auto size = static_cast<pb_size_t>(vectorToPack.size());
    auto result = static_cast<pb_bytes_array_t *>(
        malloc(PB_BYTES_ARRAY_T_ALLOCSIZE(size)));
    result->size = size;
    memcpy(result->bytes, vectorToPack.data(), size);
    return result;
}