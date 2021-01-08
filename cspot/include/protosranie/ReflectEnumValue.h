// THIS CORNFILE IS GENERATED. DO NOT EDIT! 🌽
#ifndef __REFLECT_ENUM_VALUEH
#define __REFLECT_ENUM_VALUEH
#include <string>
class ReflectEnumValue {
public:
std::string name;
int value;
static constexpr ReflectTypeID _TYPE_ID = ReflectTypeID::ClassReflectEnumValue;

			ReflectEnumValue(){};
			ReflectEnumValue( std::string name, int value) {
				this->name = name;
				this->value = value;
			}
		};

#endif
