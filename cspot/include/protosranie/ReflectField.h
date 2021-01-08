// THIS CORNFILE IS GENERATED. DO NOT EDIT! 🌽
#ifndef __REFLECT_FIELDH
#define __REFLECT_FIELDH
#include <string>
#include "ReflectTypeID.h"
class ReflectField {
public:
ReflectTypeID typeID;
std::string name;
size_t offset;
uint32_t protobufTag;
static constexpr ReflectTypeID _TYPE_ID = ReflectTypeID::ClassReflectField;

			ReflectField() {};
			ReflectField(ReflectTypeID typeID, std::string name, size_t offset, uint32_t protobufTag) {
				this->typeID = typeID;
				this->name = name;
				this->offset = offset;
				this->protobufTag = protobufTag;
			}
		};

#endif
