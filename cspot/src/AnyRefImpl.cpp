// THIS CORNFILE IS GENERATED. DO NOT EDIT! 🌽
#include "protosranie/sranie.h"

	ReflectType *AnyRef::reflectType() {
		return &reflectTypeInfo[static_cast<int>(this->typeID)];
	}
	AnyRef AnyRef::getField(int i) {
		auto info = this->reflectType();
		if(info->kind != ReflectTypeKind::Class) {
			throw "not a class";
		}
		return AnyRef(info->fields[i].typeID, static_cast<char *>(this->value.voidptr) + info->fields[i].offset);
	}
