#ifndef PBUTILS_H
#define PBUTILS_H

#include <vector>
#include <string>
#include <stdexcept>
#include <type_traits>
#include <iostream>
#include <memory>

#include "pb_encode.h"
#include "pb_decode.h"
#include "mercury.pb.h"
#include "keyexchange.pb.h"
#include "spirc.pb.h"
#include "authentication.pb.h"
#include "metadata.pb.h"

std::vector<uint8_t> encodePB(const pb_msgdesc_t *fields, const void *src_struct);

pb_bytes_array_t *vectorToPbArray(const std::vector<uint8_t> &vectorToPack);

void packString(char *&dst, std::string stringToPack);

template <typename T>
T decodePB(const pb_msgdesc_t *fields, std::vector<uint8_t> &data)
{

    T result = {};
    // Create stream
    pb_istream_t stream = pb_istream_from_buffer(&data[0], data.size());

    // Decode the message
    if (pb_decode(&stream, fields, &result) == false)
    {
        printf("Decode failed: %s\n", PB_GET_ERROR(&stream));
    }
    return result;
}

/**
 * PBWrapper is like a smart pointer but for nanoPB structs. 
 * It manages decoding and freeing the memory allocated by nanopb.
 * This is a replacement for decodePB.
 * */
template <typename T>
class PBWrapper
{
public:
    std::shared_ptr<T> innerData;
    T result = {};

    PBWrapper()
    {
    }

    PBWrapper(std::vector<uint8_t> &data)
    {
        // Create stream
        pb_istream_t stream = pb_istream_from_buffer(&data[0], data.size());
        std::cout << typeid(T).name() << std::endl;

        auto fields = this->getFieldsForType();
        std::cout << fields << std::endl;
        std::cout << APResponseMessage_fields << std::endl;

        // Decode the message
        if (pb_decode(&stream, fields, &result) == false)
        {
            auto dat = std::string("failed to decode nanoPB: ") + PB_GET_ERROR(&stream);
            std::cout << dat << std::endl;
            throw std::runtime_error(std::string("failed to decode nanoPB: ") + PB_GET_ERROR(&stream));
        }

        innerData = std::make_shared<T>(result);
    }

    ~PBWrapper()
    {
        if (this->innerData != nullptr)
        {
            pb_release(getFieldsForType(), &result);
            // delete this->innerData;
        }
    }
    PBWrapper(const PBWrapper &) = delete; // delete copy constructor since I don't even know how to copy PB structs

    // move constructor
    PBWrapper(PBWrapper &&other)
    {

        this->innerData = other.innerData;
        other.innerData = nullptr; // empty the pointer not to free it twice
    }

    // move copy constructor
    PBWrapper &operator=(PBWrapper &&other)
    {

        this->innerData = other.innerData;
        other.innerData = nullptr;

        return *this;
    }

    std::shared_ptr<T> operator->() const
    {
        return innerData;
    }

private:
    // dummy method, it's name serves as an error message when the programmer forgets to register thr nanopb type
    static void YOU_MORON_YOU_NEED_TO_REGISTER_THE_NANOPB_TYPE_IN_PBUTILS_PBWRAPPER_______LOOK_MORTY_I_TURNED_MYSELF_INTO_A_PICKLE(){};
    static constexpr const pb_msgdesc_t *getFieldsForType()
    {

#define PB_WRAPPER_REGISTER_TYPE(NAME) \
    do                                 \
    {                                  \
        if (std::is_same_v<T, NAME>)   \
        {                              \
            return (NAME##_fields);    \
        }                              \
    } while (0)

        // Type registrations
        PB_WRAPPER_REGISTER_TYPE(Header);
        PB_WRAPPER_REGISTER_TYPE(APResponseMessage);
        PB_WRAPPER_REGISTER_TYPE(APWelcome);
        PB_WRAPPER_REGISTER_TYPE(Frame);
        PB_WRAPPER_REGISTER_TYPE(Track);
        // You have constructed PBWrapper with an unknown type. You need to register it in PBUtils
        YOU_MORON_YOU_NEED_TO_REGISTER_THE_NANOPB_TYPE_IN_PBUTILS_PBWRAPPER_______LOOK_MORTY_I_TURNED_MYSELF_INTO_A_PICKLE();

        return nullptr;
    }
};

#endif
