// THIS CORNFILE IS GENERATED. DO NOT EDIT! 🌽
#ifndef __REFLECT_TYPEH
#define __REFLECT_TYPEH
#include <vector>
#include <string>
#include "ReflectTypeID.h"
#include "ReflectTypeKind.h"
#include "ReflectField.h"
#include "ReflectEnumValue.h"
class ReflectType {
public:
ReflectTypeID typeID;
std::string name;
ReflectTypeKind kind;
size_t size;
ReflectTypeID innerType;
std::vector<ReflectField> fields;
std::vector<ReflectEnumValue> enumValues;
static constexpr ReflectTypeID _TYPE_ID = ReflectTypeID::ClassReflectType;

		void (*_Construct)(void *mem);
		void (*_Destruct)(void *obj);
		VectorOperations vectorOps;
		OptionalOperations optionalOps;
		static ReflectType ofPrimitive(ReflectTypeID id, std::string name, size_t size) {
			ReflectType t;
			t.kind = ReflectTypeKind::Primitive;
			t.typeID = id;
			t.name = name;
			t.size = size;
			return t;
		}
		static ReflectType ofEnum(ReflectTypeID id, std::string name, std::vector<ReflectEnumValue> enumValues, size_t size) {
			ReflectType t;
			t.kind = ReflectTypeKind::Enum;
			t.typeID = id;
			t.name = name;
			t.size = size;
			t.enumValues = enumValues;
			return t;
		}
		static ReflectType ofVector(ReflectTypeID id, ReflectTypeID innerType, size_t size, 
			VectorOperations vectorOps,
			void (*_Construct)(void *mem), void (*_Destruct)(void *obj)) {
			ReflectType t;
			t.kind = ReflectTypeKind::Vector;
			t.typeID = id;
			t.innerType = innerType;
			t.size = size;
			t._Construct = _Construct;
			t._Destruct = _Destruct;
			t.vectorOps = vectorOps;
			return t;
		}
		static ReflectType ofOptional(ReflectTypeID id, ReflectTypeID innerType, size_t size, 
			OptionalOperations optionalOps,
			void (*_Construct)(void *mem), void (*_Destruct)(void *obj)) {
			ReflectType t;
			t.kind = ReflectTypeKind::Optional;
			t.typeID = id;
			t.innerType = innerType;
			t.size = size;
			t._Construct = _Construct;
			t._Destruct = _Destruct;
			t.optionalOps = optionalOps;
			return t;
		}
		static ReflectType ofClass(ReflectTypeID id, std::string name, std::vector<ReflectField> fields, size_t size, void (*_Construct)(void *mem), void (*_Destruct)(void *obj)) {
			ReflectType t;
			t.kind = ReflectTypeKind::Class;
			t.name = name;
			t.typeID = id;
			t.size = size;
			t.fields = std::move(fields);
			t._Construct = _Construct;
			t._Destruct = _Destruct;
			return t;
		}
		
		};

#endif
