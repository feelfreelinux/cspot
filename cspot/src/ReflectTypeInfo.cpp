// THIS CORNFILE IS GENERATED. DO NOT EDIT! 🌽
#include <vector>
#include "protosranie/sranie.h"
ReflectType reflectTypeInfo[82] = {
ReflectType::ofEnum(/* mine id */ ReflectTypeID::EnumReflectTypeID, /* name */ "ReflectTypeID", /* enum values */ std::move(std::vector<ReflectEnumValue>{   ReflectEnumValue("EnumReflectTypeID", 0),
   ReflectEnumValue("ClassReflectType", 1),
   ReflectEnumValue("ClassReflectField", 2),
   ReflectEnumValue("ClassReflectEnumValue", 3),
   ReflectEnumValue("EnumReflectTypeKind", 4),
   ReflectEnumValue("VectorOfClassReflectField", 5),
   ReflectEnumValue("VectorOfClassReflectEnumValue", 6),
   ReflectEnumValue("Bool", 7),
   ReflectEnumValue("Int32", 8),
   ReflectEnumValue("Uint64", 9),
   ReflectEnumValue("Double", 10),
   ReflectEnumValue("Int64", 11),
   ReflectEnumValue("Uint32", 12),
   ReflectEnumValue("Float", 13),
   ReflectEnumValue("String", 14),
   ReflectEnumValue("SizeT", 15),
   ReflectEnumValue("UnsignedInt", 16),
   ReflectEnumValue("Char", 17),
   ReflectEnumValue("UnsignedChar", 18),
   ReflectEnumValue("Int", 19),
   ReflectEnumValue("Uint8", 20),
   ReflectEnumValue("VectorOfUint8", 21),
   ReflectEnumValue("EnumCpuFamily", 22),
   ReflectEnumValue("EnumOs", 23),
   ReflectEnumValue("EnumAuthenticationType", 24),
   ReflectEnumValue("OptionalOfString", 25),
   ReflectEnumValue("ClassSystemInfo", 26),
   ReflectEnumValue("OptionalOfVectorOfUint8", 27),
   ReflectEnumValue("ClassLoginCredentials", 28),
   ReflectEnumValue("ClassClientResponseEncrypted", 29),
   ReflectEnumValue("EnumProduct", 30),
   ReflectEnumValue("EnumPlatform", 31),
   ReflectEnumValue("EnumCryptosuite", 32),
   ReflectEnumValue("ClassLoginCryptoDiffieHellmanChallenge", 33),
   ReflectEnumValue("OptionalOfClassLoginCryptoDiffieHellmanChallenge", 34),
   ReflectEnumValue("ClassLoginCryptoChallengeUnion", 35),
   ReflectEnumValue("ClassLoginCryptoDiffieHellmanHello", 36),
   ReflectEnumValue("OptionalOfClassLoginCryptoDiffieHellmanHello", 37),
   ReflectEnumValue("ClassLoginCryptoHelloUnion", 38),
   ReflectEnumValue("ClassBuildInfo", 39),
   ReflectEnumValue("OptionalOfBool", 40),
   ReflectEnumValue("ClassFeatureSet", 41),
   ReflectEnumValue("ClassAPChallenge", 42),
   ReflectEnumValue("OptionalOfClassAPChallenge", 43),
   ReflectEnumValue("ClassAPResponseMessage", 44),
   ReflectEnumValue("ClassLoginCryptoDiffieHellmanResponse", 45),
   ReflectEnumValue("OptionalOfClassLoginCryptoDiffieHellmanResponse", 46),
   ReflectEnumValue("ClassLoginCryptoResponseUnion", 47),
   ReflectEnumValue("ClassCryptoResponseUnion", 48),
   ReflectEnumValue("ClassPoWResponseUnion", 49),
   ReflectEnumValue("ClassClientResponsePlaintext", 50),
   ReflectEnumValue("VectorOfEnumCryptosuite", 51),
   ReflectEnumValue("OptionalOfClassFeatureSet", 52),
   ReflectEnumValue("ClassClientHello", 53),
   ReflectEnumValue("ClassHeader", 54),
   ReflectEnumValue("EnumAudioFormat", 55),
   ReflectEnumValue("OptionalOfEnumAudioFormat", 56),
   ReflectEnumValue("ClassAudioFile", 57),
   ReflectEnumValue("OptionalOfInt32", 58),
   ReflectEnumValue("VectorOfClassAudioFile", 59),
   ReflectEnumValue("ClassTrack", 60),
   ReflectEnumValue("ClassEpisode", 61),
   ReflectEnumValue("EnumMessageType", 62),
   ReflectEnumValue("EnumPlayStatus", 63),
   ReflectEnumValue("EnumCapabilityType", 64),
   ReflectEnumValue("ClassTrackRef", 65),
   ReflectEnumValue("OptionalOfUint32", 66),
   ReflectEnumValue("OptionalOfEnumPlayStatus", 67),
   ReflectEnumValue("OptionalOfUint64", 68),
   ReflectEnumValue("VectorOfClassTrackRef", 69),
   ReflectEnumValue("ClassState", 70),
   ReflectEnumValue("OptionalOfEnumCapabilityType", 71),
   ReflectEnumValue("VectorOfInt64", 72),
   ReflectEnumValue("VectorOfString", 73),
   ReflectEnumValue("ClassCapability", 74),
   ReflectEnumValue("OptionalOfInt64", 75),
   ReflectEnumValue("VectorOfClassCapability", 76),
   ReflectEnumValue("ClassDeviceState", 77),
   ReflectEnumValue("OptionalOfEnumMessageType", 78),
   ReflectEnumValue("OptionalOfClassDeviceState", 79),
   ReflectEnumValue("OptionalOfClassState", 80),
   ReflectEnumValue("ClassFrame", 81),
}), /* size */ sizeof(ReflectTypeID)),
ReflectType::ofClass(
	/* mine type id */ ReflectTypeID::ClassReflectType, 
	/* name */ "ReflectType", 
	/* fields */ std::move(std::vector<ReflectField>{ReflectField( /* typeID */ ReflectTypeID::EnumReflectTypeID, /* name */ "typeID", /* offset */ offsetof(ReflectType, typeID), /* protobuf tag */ 0),
ReflectField( /* typeID */ ReflectTypeID::String, /* name */ "name", /* offset */ offsetof(ReflectType, name), /* protobuf tag */ 0),
ReflectField( /* typeID */ ReflectTypeID::EnumReflectTypeKind, /* name */ "kind", /* offset */ offsetof(ReflectType, kind), /* protobuf tag */ 0),
ReflectField( /* typeID */ ReflectTypeID::SizeT, /* name */ "size", /* offset */ offsetof(ReflectType, size), /* protobuf tag */ 0),
ReflectField( /* typeID */ ReflectTypeID::EnumReflectTypeID, /* name */ "innerType", /* offset */ offsetof(ReflectType, innerType), /* protobuf tag */ 0),
ReflectField( /* typeID */ ReflectTypeID::VectorOfClassReflectField, /* name */ "fields", /* offset */ offsetof(ReflectType, fields), /* protobuf tag */ 0),
ReflectField( /* typeID */ ReflectTypeID::VectorOfClassReflectEnumValue, /* name */ "enumValues", /* offset */ offsetof(ReflectType, enumValues), /* protobuf tag */ 0),
}), 
	/* size */ sizeof(ReflectType), 
	__reflectConstruct<ReflectType>,
	__reflectDestruct<ReflectType>),
ReflectType::ofClass(
	/* mine type id */ ReflectTypeID::ClassReflectField, 
	/* name */ "ReflectField", 
	/* fields */ std::move(std::vector<ReflectField>{ReflectField( /* typeID */ ReflectTypeID::EnumReflectTypeID, /* name */ "typeID", /* offset */ offsetof(ReflectField, typeID), /* protobuf tag */ 0),
ReflectField( /* typeID */ ReflectTypeID::String, /* name */ "name", /* offset */ offsetof(ReflectField, name), /* protobuf tag */ 0),
ReflectField( /* typeID */ ReflectTypeID::SizeT, /* name */ "offset", /* offset */ offsetof(ReflectField, offset), /* protobuf tag */ 0),
ReflectField( /* typeID */ ReflectTypeID::Uint32, /* name */ "protobufTag", /* offset */ offsetof(ReflectField, protobufTag), /* protobuf tag */ 0),
}), 
	/* size */ sizeof(ReflectField), 
	__reflectConstruct<ReflectField>,
	__reflectDestruct<ReflectField>),
ReflectType::ofClass(
	/* mine type id */ ReflectTypeID::ClassReflectEnumValue, 
	/* name */ "ReflectEnumValue", 
	/* fields */ std::move(std::vector<ReflectField>{ReflectField( /* typeID */ ReflectTypeID::String, /* name */ "name", /* offset */ offsetof(ReflectEnumValue, name), /* protobuf tag */ 0),
ReflectField( /* typeID */ ReflectTypeID::Int, /* name */ "value", /* offset */ offsetof(ReflectEnumValue, value), /* protobuf tag */ 0),
}), 
	/* size */ sizeof(ReflectEnumValue), 
	__reflectConstruct<ReflectEnumValue>,
	__reflectDestruct<ReflectEnumValue>),
ReflectType::ofEnum(/* mine id */ ReflectTypeID::EnumReflectTypeKind, /* name */ "ReflectTypeKind", /* enum values */ std::move(std::vector<ReflectEnumValue>{   ReflectEnumValue("Primitive", 0),
   ReflectEnumValue("Enum", 1),
   ReflectEnumValue("Class", 2),
   ReflectEnumValue("Vector", 3),
   ReflectEnumValue("Optional", 4),
}), /* size */ sizeof(ReflectTypeKind)),

	ReflectType::ofVector(
		/* mine typeId */ ReflectTypeID::VectorOfClassReflectField,
		/* inner type id */  ReflectTypeID::ClassReflectField,
		/* size */ sizeof(std::vector<ReflectField>),
		VectorOperations{
			.push_back = __VectorManipulator<ReflectField>::push_back,
			.at = __VectorManipulator<ReflectField>::at,
			.size = __VectorManipulator<ReflectField>::size,
			.emplace_back =  __VectorManipulator<ReflectField>::emplace_back,
			.clear = __VectorManipulator<ReflectField>::clear,
			.reserve = __VectorManipulator<ReflectField>::reserve,
		},
		__reflectConstruct<std::vector<ReflectField>>,
		__reflectDestruct<std::vector<ReflectField>>
	)
	
	
	
	,

	ReflectType::ofVector(
		/* mine typeId */ ReflectTypeID::VectorOfClassReflectEnumValue,
		/* inner type id */  ReflectTypeID::ClassReflectEnumValue,
		/* size */ sizeof(std::vector<ReflectEnumValue>),
		VectorOperations{
			.push_back = __VectorManipulator<ReflectEnumValue>::push_back,
			.at = __VectorManipulator<ReflectEnumValue>::at,
			.size = __VectorManipulator<ReflectEnumValue>::size,
			.emplace_back =  __VectorManipulator<ReflectEnumValue>::emplace_back,
			.clear = __VectorManipulator<ReflectEnumValue>::clear,
			.reserve = __VectorManipulator<ReflectEnumValue>::reserve,
		},
		__reflectConstruct<std::vector<ReflectEnumValue>>,
		__reflectDestruct<std::vector<ReflectEnumValue>>
	)
	
	
	
	,
ReflectType::ofPrimitive(/* type id */ ReflectTypeID::Bool, /* name */ "bool", /* size */ sizeof(bool)),
ReflectType::ofPrimitive(/* type id */ ReflectTypeID::Int32, /* name */ "int32_t", /* size */ sizeof(int32_t)),
ReflectType::ofPrimitive(/* type id */ ReflectTypeID::Uint64, /* name */ "uint64_t", /* size */ sizeof(uint64_t)),
ReflectType::ofPrimitive(/* type id */ ReflectTypeID::Double, /* name */ "double", /* size */ sizeof(double)),
ReflectType::ofPrimitive(/* type id */ ReflectTypeID::Int64, /* name */ "int64_t", /* size */ sizeof(int64_t)),
ReflectType::ofPrimitive(/* type id */ ReflectTypeID::Uint32, /* name */ "uint32_t", /* size */ sizeof(uint32_t)),
ReflectType::ofPrimitive(/* type id */ ReflectTypeID::Float, /* name */ "float", /* size */ sizeof(float)),
ReflectType::ofPrimitive(/* type id */ ReflectTypeID::String, /* name */ "std::string", /* size */ sizeof(std::string)),
ReflectType::ofPrimitive(/* type id */ ReflectTypeID::SizeT, /* name */ "size_t", /* size */ sizeof(size_t)),
ReflectType::ofPrimitive(/* type id */ ReflectTypeID::UnsignedInt, /* name */ "unsigned int", /* size */ sizeof(unsigned int)),
ReflectType::ofPrimitive(/* type id */ ReflectTypeID::Char, /* name */ "char", /* size */ sizeof(char)),
ReflectType::ofPrimitive(/* type id */ ReflectTypeID::UnsignedChar, /* name */ "unsigned char", /* size */ sizeof(unsigned char)),
ReflectType::ofPrimitive(/* type id */ ReflectTypeID::Int, /* name */ "int", /* size */ sizeof(int)),
ReflectType::ofPrimitive(/* type id */ ReflectTypeID::Uint8, /* name */ "uint8_t", /* size */ sizeof(uint8_t)),

	ReflectType::ofVector(
		/* mine typeId */ ReflectTypeID::VectorOfUint8,
		/* inner type id */  ReflectTypeID::Uint8,
		/* size */ sizeof(std::vector<uint8_t>),
		VectorOperations{
			.push_back = __VectorManipulator<uint8_t>::push_back,
			.at = __VectorManipulator<uint8_t>::at,
			.size = __VectorManipulator<uint8_t>::size,
			.emplace_back =  __VectorManipulator<uint8_t>::emplace_back,
			.clear = __VectorManipulator<uint8_t>::clear,
			.reserve = __VectorManipulator<uint8_t>::reserve,
		},
		__reflectConstruct<std::vector<uint8_t>>,
		__reflectDestruct<std::vector<uint8_t>>
	)
	
	
	
	,
ReflectType::ofEnum(/* mine id */ ReflectTypeID::EnumCpuFamily, /* name */ "CpuFamily", /* enum values */ std::move(std::vector<ReflectEnumValue>{   ReflectEnumValue("CPU_UNKNOWN", 0),
   ReflectEnumValue("CPU_X86", 1),
   ReflectEnumValue("CPU_X86_64", 2),
   ReflectEnumValue("CPU_PPC", 3),
   ReflectEnumValue("CPU_PPC_64", 4),
   ReflectEnumValue("CPU_ARM", 5),
   ReflectEnumValue("CPU_IA64", 6),
   ReflectEnumValue("CPU_SH", 7),
   ReflectEnumValue("CPU_MIPS", 8),
   ReflectEnumValue("CPU_BLACKFIN", 9),
}), /* size */ sizeof(CpuFamily)),
ReflectType::ofEnum(/* mine id */ ReflectTypeID::EnumOs, /* name */ "Os", /* enum values */ std::move(std::vector<ReflectEnumValue>{   ReflectEnumValue("OS_UNKNOWN", 0),
   ReflectEnumValue("OS_WINDOWS", 1),
   ReflectEnumValue("OS_OSX", 2),
   ReflectEnumValue("OS_IPHONE", 3),
   ReflectEnumValue("OS_S60", 4),
   ReflectEnumValue("OS_LINUX", 5),
   ReflectEnumValue("OS_WINDOWS_CE", 6),
   ReflectEnumValue("OS_ANDROID", 7),
   ReflectEnumValue("OS_PALM", 8),
   ReflectEnumValue("OS_FREEBSD", 9),
   ReflectEnumValue("OS_BLACKBERRY", 10),
   ReflectEnumValue("OS_SONOS", 11),
   ReflectEnumValue("OS_LOGITECH", 12),
   ReflectEnumValue("OS_WP7", 13),
   ReflectEnumValue("OS_ONKYO", 14),
   ReflectEnumValue("OS_PHILIPS", 15),
   ReflectEnumValue("OS_WD", 16),
   ReflectEnumValue("OS_VOLVO", 17),
   ReflectEnumValue("OS_TIVO", 18),
   ReflectEnumValue("OS_AWOX", 19),
   ReflectEnumValue("OS_MEEGO", 20),
   ReflectEnumValue("OS_QNXNTO", 21),
   ReflectEnumValue("OS_BCO", 22),
}), /* size */ sizeof(Os)),
ReflectType::ofEnum(/* mine id */ ReflectTypeID::EnumAuthenticationType, /* name */ "AuthenticationType", /* enum values */ std::move(std::vector<ReflectEnumValue>{   ReflectEnumValue("AUTHENTICATION_USER_PASS", 0),
   ReflectEnumValue("AUTHENTICATION_STORED_SPOTIFY_CREDENTIALS", 1),
   ReflectEnumValue("AUTHENTICATION_STORED_FACEBOOK_CREDENTIALS", 2),
   ReflectEnumValue("AUTHENTICATION_SPOTIFY_TOKEN", 3),
   ReflectEnumValue("AUTHENTICATION_FACEBOOK_TOKEN", 4),
}), /* size */ sizeof(AuthenticationType)),

	ReflectType::ofOptional(
		/* mine typeId */ ReflectTypeID::OptionalOfString,
		/* inner type id */  ReflectTypeID::String,
		/* size */ sizeof(std::optional<std::string>),
		/* option */ OptionalOperations{
			.get = __OptionalManipulator<std::string>::get,
			.has_value = __OptionalManipulator<std::string>::has_value,
			.set = __OptionalManipulator<std::string>::set,
			.reset = __OptionalManipulator<std::string>::reset,
			.emplaceEmpty =  __OptionalManipulator<std::string>::emplaceEmpty,
		},
		__reflectConstruct<std::optional<std::string>>,
		__reflectDestruct<std::optional<std::string>>
	)
	
	
	
	,
ReflectType::ofClass(
	/* mine type id */ ReflectTypeID::ClassSystemInfo, 
	/* name */ "SystemInfo", 
	/* fields */ std::move(std::vector<ReflectField>{ReflectField( /* typeID */ ReflectTypeID::EnumCpuFamily, /* name */ "cpu_family", /* offset */ offsetof(SystemInfo, cpu_family), /* protobuf tag */ 10),
ReflectField( /* typeID */ ReflectTypeID::EnumOs, /* name */ "os", /* offset */ offsetof(SystemInfo, os), /* protobuf tag */ 60),
ReflectField( /* typeID */ ReflectTypeID::OptionalOfString, /* name */ "system_information_string", /* offset */ offsetof(SystemInfo, system_information_string), /* protobuf tag */ 90),
ReflectField( /* typeID */ ReflectTypeID::OptionalOfString, /* name */ "device_id", /* offset */ offsetof(SystemInfo, device_id), /* protobuf tag */ 100),
}), 
	/* size */ sizeof(SystemInfo), 
	__reflectConstruct<SystemInfo>,
	__reflectDestruct<SystemInfo>),

	ReflectType::ofOptional(
		/* mine typeId */ ReflectTypeID::OptionalOfVectorOfUint8,
		/* inner type id */  ReflectTypeID::VectorOfUint8,
		/* size */ sizeof(std::optional<std::vector<uint8_t>>),
		/* option */ OptionalOperations{
			.get = __OptionalManipulator<std::vector<uint8_t>>::get,
			.has_value = __OptionalManipulator<std::vector<uint8_t>>::has_value,
			.set = __OptionalManipulator<std::vector<uint8_t>>::set,
			.reset = __OptionalManipulator<std::vector<uint8_t>>::reset,
			.emplaceEmpty =  __OptionalManipulator<std::vector<uint8_t>>::emplaceEmpty,
		},
		__reflectConstruct<std::optional<std::vector<uint8_t>>>,
		__reflectDestruct<std::optional<std::vector<uint8_t>>>
	)
	
	
	
	,
ReflectType::ofClass(
	/* mine type id */ ReflectTypeID::ClassLoginCredentials, 
	/* name */ "LoginCredentials", 
	/* fields */ std::move(std::vector<ReflectField>{ReflectField( /* typeID */ ReflectTypeID::OptionalOfString, /* name */ "username", /* offset */ offsetof(LoginCredentials, username), /* protobuf tag */ 10),
ReflectField( /* typeID */ ReflectTypeID::EnumAuthenticationType, /* name */ "typ", /* offset */ offsetof(LoginCredentials, typ), /* protobuf tag */ 20),
ReflectField( /* typeID */ ReflectTypeID::OptionalOfVectorOfUint8, /* name */ "auth_data", /* offset */ offsetof(LoginCredentials, auth_data), /* protobuf tag */ 30),
}), 
	/* size */ sizeof(LoginCredentials), 
	__reflectConstruct<LoginCredentials>,
	__reflectDestruct<LoginCredentials>),
ReflectType::ofClass(
	/* mine type id */ ReflectTypeID::ClassClientResponseEncrypted, 
	/* name */ "ClientResponseEncrypted", 
	/* fields */ std::move(std::vector<ReflectField>{ReflectField( /* typeID */ ReflectTypeID::ClassLoginCredentials, /* name */ "login_credentials", /* offset */ offsetof(ClientResponseEncrypted, login_credentials), /* protobuf tag */ 10),
ReflectField( /* typeID */ ReflectTypeID::ClassSystemInfo, /* name */ "system_info", /* offset */ offsetof(ClientResponseEncrypted, system_info), /* protobuf tag */ 50),
ReflectField( /* typeID */ ReflectTypeID::OptionalOfString, /* name */ "version_string", /* offset */ offsetof(ClientResponseEncrypted, version_string), /* protobuf tag */ 70),
}), 
	/* size */ sizeof(ClientResponseEncrypted), 
	__reflectConstruct<ClientResponseEncrypted>,
	__reflectDestruct<ClientResponseEncrypted>),
ReflectType::ofEnum(/* mine id */ ReflectTypeID::EnumProduct, /* name */ "Product", /* enum values */ std::move(std::vector<ReflectEnumValue>{   ReflectEnumValue("PRODUCT_CLIENT", 0),
   ReflectEnumValue("PRODUCT_LIBSPOTIFY", 1),
   ReflectEnumValue("PRODUCT_MOBILE", 2),
   ReflectEnumValue("PRODUCT_PARTNER", 3),
   ReflectEnumValue("PRODUCT_LIBSPOTIFY_EMBEDDED", 5),
}), /* size */ sizeof(Product)),
ReflectType::ofEnum(/* mine id */ ReflectTypeID::EnumPlatform, /* name */ "Platform", /* enum values */ std::move(std::vector<ReflectEnumValue>{   ReflectEnumValue("PLATFORM_WIN32_X86", 0),
   ReflectEnumValue("PLATFORM_OSX_X86", 1),
   ReflectEnumValue("PLATFORM_LINUX_X86", 2),
   ReflectEnumValue("PLATFORM_IPHONE_ARM", 3),
   ReflectEnumValue("PLATFORM_S60_ARM", 4),
   ReflectEnumValue("PLATFORM_OSX_PPC", 5),
   ReflectEnumValue("PLATFORM_ANDROID_ARM", 6),
   ReflectEnumValue("PLATFORM_WINDOWS_CE_ARM", 7),
   ReflectEnumValue("PLATFORM_LINUX_X86_64", 8),
   ReflectEnumValue("PLATFORM_OSX_X86_64", 9),
   ReflectEnumValue("PLATFORM_PALM_ARM", 10),
   ReflectEnumValue("PLATFORM_LINUX_SH", 11),
   ReflectEnumValue("PLATFORM_FREEBSD_X86", 12),
   ReflectEnumValue("PLATFORM_FREEBSD_X86_64", 13),
   ReflectEnumValue("PLATFORM_BLACKBERRY_ARM", 14),
   ReflectEnumValue("PLATFORM_SONOS", 15),
   ReflectEnumValue("PLATFORM_LINUX_MIPS", 16),
   ReflectEnumValue("PLATFORM_LINUX_ARM", 17),
   ReflectEnumValue("PLATFORM_LOGITECH_ARM", 18),
   ReflectEnumValue("PLATFORM_LINUX_BLACKFIN", 19),
   ReflectEnumValue("PLATFORM_WP7_ARM", 20),
   ReflectEnumValue("PLATFORM_ONKYO_ARM", 21),
   ReflectEnumValue("PLATFORM_QNXNTO_ARM", 22),
   ReflectEnumValue("PLATFORM_BCO_ARM", 23),
}), /* size */ sizeof(Platform)),
ReflectType::ofEnum(/* mine id */ ReflectTypeID::EnumCryptosuite, /* name */ "Cryptosuite", /* enum values */ std::move(std::vector<ReflectEnumValue>{   ReflectEnumValue("CRYPTO_SUITE_SHANNON", 0),
   ReflectEnumValue("CRYPTO_SUITE_RC4_SHA1_HMAC", 1),
}), /* size */ sizeof(Cryptosuite)),
ReflectType::ofClass(
	/* mine type id */ ReflectTypeID::ClassLoginCryptoDiffieHellmanChallenge, 
	/* name */ "LoginCryptoDiffieHellmanChallenge", 
	/* fields */ std::move(std::vector<ReflectField>{ReflectField( /* typeID */ ReflectTypeID::VectorOfUint8, /* name */ "gs", /* offset */ offsetof(LoginCryptoDiffieHellmanChallenge, gs), /* protobuf tag */ 10),
}), 
	/* size */ sizeof(LoginCryptoDiffieHellmanChallenge), 
	__reflectConstruct<LoginCryptoDiffieHellmanChallenge>,
	__reflectDestruct<LoginCryptoDiffieHellmanChallenge>),

	ReflectType::ofOptional(
		/* mine typeId */ ReflectTypeID::OptionalOfClassLoginCryptoDiffieHellmanChallenge,
		/* inner type id */  ReflectTypeID::ClassLoginCryptoDiffieHellmanChallenge,
		/* size */ sizeof(std::optional<LoginCryptoDiffieHellmanChallenge>),
		/* option */ OptionalOperations{
			.get = __OptionalManipulator<LoginCryptoDiffieHellmanChallenge>::get,
			.has_value = __OptionalManipulator<LoginCryptoDiffieHellmanChallenge>::has_value,
			.set = __OptionalManipulator<LoginCryptoDiffieHellmanChallenge>::set,
			.reset = __OptionalManipulator<LoginCryptoDiffieHellmanChallenge>::reset,
			.emplaceEmpty =  __OptionalManipulator<LoginCryptoDiffieHellmanChallenge>::emplaceEmpty,
		},
		__reflectConstruct<std::optional<LoginCryptoDiffieHellmanChallenge>>,
		__reflectDestruct<std::optional<LoginCryptoDiffieHellmanChallenge>>
	)
	
	
	
	,
ReflectType::ofClass(
	/* mine type id */ ReflectTypeID::ClassLoginCryptoChallengeUnion, 
	/* name */ "LoginCryptoChallengeUnion", 
	/* fields */ std::move(std::vector<ReflectField>{ReflectField( /* typeID */ ReflectTypeID::OptionalOfClassLoginCryptoDiffieHellmanChallenge, /* name */ "diffie_hellman", /* offset */ offsetof(LoginCryptoChallengeUnion, diffie_hellman), /* protobuf tag */ 10),
}), 
	/* size */ sizeof(LoginCryptoChallengeUnion), 
	__reflectConstruct<LoginCryptoChallengeUnion>,
	__reflectDestruct<LoginCryptoChallengeUnion>),
ReflectType::ofClass(
	/* mine type id */ ReflectTypeID::ClassLoginCryptoDiffieHellmanHello, 
	/* name */ "LoginCryptoDiffieHellmanHello", 
	/* fields */ std::move(std::vector<ReflectField>{ReflectField( /* typeID */ ReflectTypeID::VectorOfUint8, /* name */ "gc", /* offset */ offsetof(LoginCryptoDiffieHellmanHello, gc), /* protobuf tag */ 10),
ReflectField( /* typeID */ ReflectTypeID::Uint32, /* name */ "server_keys_known", /* offset */ offsetof(LoginCryptoDiffieHellmanHello, server_keys_known), /* protobuf tag */ 20),
}), 
	/* size */ sizeof(LoginCryptoDiffieHellmanHello), 
	__reflectConstruct<LoginCryptoDiffieHellmanHello>,
	__reflectDestruct<LoginCryptoDiffieHellmanHello>),

	ReflectType::ofOptional(
		/* mine typeId */ ReflectTypeID::OptionalOfClassLoginCryptoDiffieHellmanHello,
		/* inner type id */  ReflectTypeID::ClassLoginCryptoDiffieHellmanHello,
		/* size */ sizeof(std::optional<LoginCryptoDiffieHellmanHello>),
		/* option */ OptionalOperations{
			.get = __OptionalManipulator<LoginCryptoDiffieHellmanHello>::get,
			.has_value = __OptionalManipulator<LoginCryptoDiffieHellmanHello>::has_value,
			.set = __OptionalManipulator<LoginCryptoDiffieHellmanHello>::set,
			.reset = __OptionalManipulator<LoginCryptoDiffieHellmanHello>::reset,
			.emplaceEmpty =  __OptionalManipulator<LoginCryptoDiffieHellmanHello>::emplaceEmpty,
		},
		__reflectConstruct<std::optional<LoginCryptoDiffieHellmanHello>>,
		__reflectDestruct<std::optional<LoginCryptoDiffieHellmanHello>>
	)
	
	
	
	,
ReflectType::ofClass(
	/* mine type id */ ReflectTypeID::ClassLoginCryptoHelloUnion, 
	/* name */ "LoginCryptoHelloUnion", 
	/* fields */ std::move(std::vector<ReflectField>{ReflectField( /* typeID */ ReflectTypeID::OptionalOfClassLoginCryptoDiffieHellmanHello, /* name */ "diffie_hellman", /* offset */ offsetof(LoginCryptoHelloUnion, diffie_hellman), /* protobuf tag */ 10),
}), 
	/* size */ sizeof(LoginCryptoHelloUnion), 
	__reflectConstruct<LoginCryptoHelloUnion>,
	__reflectDestruct<LoginCryptoHelloUnion>),
ReflectType::ofClass(
	/* mine type id */ ReflectTypeID::ClassBuildInfo, 
	/* name */ "BuildInfo", 
	/* fields */ std::move(std::vector<ReflectField>{ReflectField( /* typeID */ ReflectTypeID::EnumProduct, /* name */ "product", /* offset */ offsetof(BuildInfo, product), /* protobuf tag */ 10),
ReflectField( /* typeID */ ReflectTypeID::EnumPlatform, /* name */ "platform", /* offset */ offsetof(BuildInfo, platform), /* protobuf tag */ 30),
ReflectField( /* typeID */ ReflectTypeID::Uint64, /* name */ "version", /* offset */ offsetof(BuildInfo, version), /* protobuf tag */ 40),
}), 
	/* size */ sizeof(BuildInfo), 
	__reflectConstruct<BuildInfo>,
	__reflectDestruct<BuildInfo>),

	ReflectType::ofOptional(
		/* mine typeId */ ReflectTypeID::OptionalOfBool,
		/* inner type id */  ReflectTypeID::Bool,
		/* size */ sizeof(std::optional<bool>),
		/* option */ OptionalOperations{
			.get = __OptionalManipulator<bool>::get,
			.has_value = __OptionalManipulator<bool>::has_value,
			.set = __OptionalManipulator<bool>::set,
			.reset = __OptionalManipulator<bool>::reset,
			.emplaceEmpty =  __OptionalManipulator<bool>::emplaceEmpty,
		},
		__reflectConstruct<std::optional<bool>>,
		__reflectDestruct<std::optional<bool>>
	)
	
	
	
	,
ReflectType::ofClass(
	/* mine type id */ ReflectTypeID::ClassFeatureSet, 
	/* name */ "FeatureSet", 
	/* fields */ std::move(std::vector<ReflectField>{ReflectField( /* typeID */ ReflectTypeID::OptionalOfBool, /* name */ "autoupdate2", /* offset */ offsetof(FeatureSet, autoupdate2), /* protobuf tag */ 1),
}), 
	/* size */ sizeof(FeatureSet), 
	__reflectConstruct<FeatureSet>,
	__reflectDestruct<FeatureSet>),
ReflectType::ofClass(
	/* mine type id */ ReflectTypeID::ClassAPChallenge, 
	/* name */ "APChallenge", 
	/* fields */ std::move(std::vector<ReflectField>{ReflectField( /* typeID */ ReflectTypeID::ClassLoginCryptoChallengeUnion, /* name */ "login_crypto_challenge", /* offset */ offsetof(APChallenge, login_crypto_challenge), /* protobuf tag */ 10),
}), 
	/* size */ sizeof(APChallenge), 
	__reflectConstruct<APChallenge>,
	__reflectDestruct<APChallenge>),

	ReflectType::ofOptional(
		/* mine typeId */ ReflectTypeID::OptionalOfClassAPChallenge,
		/* inner type id */  ReflectTypeID::ClassAPChallenge,
		/* size */ sizeof(std::optional<APChallenge>),
		/* option */ OptionalOperations{
			.get = __OptionalManipulator<APChallenge>::get,
			.has_value = __OptionalManipulator<APChallenge>::has_value,
			.set = __OptionalManipulator<APChallenge>::set,
			.reset = __OptionalManipulator<APChallenge>::reset,
			.emplaceEmpty =  __OptionalManipulator<APChallenge>::emplaceEmpty,
		},
		__reflectConstruct<std::optional<APChallenge>>,
		__reflectDestruct<std::optional<APChallenge>>
	)
	
	
	
	,
ReflectType::ofClass(
	/* mine type id */ ReflectTypeID::ClassAPResponseMessage, 
	/* name */ "APResponseMessage", 
	/* fields */ std::move(std::vector<ReflectField>{ReflectField( /* typeID */ ReflectTypeID::OptionalOfClassAPChallenge, /* name */ "challenge", /* offset */ offsetof(APResponseMessage, challenge), /* protobuf tag */ 10),
}), 
	/* size */ sizeof(APResponseMessage), 
	__reflectConstruct<APResponseMessage>,
	__reflectDestruct<APResponseMessage>),
ReflectType::ofClass(
	/* mine type id */ ReflectTypeID::ClassLoginCryptoDiffieHellmanResponse, 
	/* name */ "LoginCryptoDiffieHellmanResponse", 
	/* fields */ std::move(std::vector<ReflectField>{ReflectField( /* typeID */ ReflectTypeID::VectorOfUint8, /* name */ "hmac", /* offset */ offsetof(LoginCryptoDiffieHellmanResponse, hmac), /* protobuf tag */ 10),
}), 
	/* size */ sizeof(LoginCryptoDiffieHellmanResponse), 
	__reflectConstruct<LoginCryptoDiffieHellmanResponse>,
	__reflectDestruct<LoginCryptoDiffieHellmanResponse>),

	ReflectType::ofOptional(
		/* mine typeId */ ReflectTypeID::OptionalOfClassLoginCryptoDiffieHellmanResponse,
		/* inner type id */  ReflectTypeID::ClassLoginCryptoDiffieHellmanResponse,
		/* size */ sizeof(std::optional<LoginCryptoDiffieHellmanResponse>),
		/* option */ OptionalOperations{
			.get = __OptionalManipulator<LoginCryptoDiffieHellmanResponse>::get,
			.has_value = __OptionalManipulator<LoginCryptoDiffieHellmanResponse>::has_value,
			.set = __OptionalManipulator<LoginCryptoDiffieHellmanResponse>::set,
			.reset = __OptionalManipulator<LoginCryptoDiffieHellmanResponse>::reset,
			.emplaceEmpty =  __OptionalManipulator<LoginCryptoDiffieHellmanResponse>::emplaceEmpty,
		},
		__reflectConstruct<std::optional<LoginCryptoDiffieHellmanResponse>>,
		__reflectDestruct<std::optional<LoginCryptoDiffieHellmanResponse>>
	)
	
	
	
	,
ReflectType::ofClass(
	/* mine type id */ ReflectTypeID::ClassLoginCryptoResponseUnion, 
	/* name */ "LoginCryptoResponseUnion", 
	/* fields */ std::move(std::vector<ReflectField>{ReflectField( /* typeID */ ReflectTypeID::OptionalOfClassLoginCryptoDiffieHellmanResponse, /* name */ "diffie_hellman", /* offset */ offsetof(LoginCryptoResponseUnion, diffie_hellman), /* protobuf tag */ 10),
}), 
	/* size */ sizeof(LoginCryptoResponseUnion), 
	__reflectConstruct<LoginCryptoResponseUnion>,
	__reflectDestruct<LoginCryptoResponseUnion>),
ReflectType::ofClass(
	/* mine type id */ ReflectTypeID::ClassCryptoResponseUnion, 
	/* name */ "CryptoResponseUnion", 
	/* fields */ std::move(std::vector<ReflectField>{}), 
	/* size */ sizeof(CryptoResponseUnion), 
	__reflectConstruct<CryptoResponseUnion>,
	__reflectDestruct<CryptoResponseUnion>),
ReflectType::ofClass(
	/* mine type id */ ReflectTypeID::ClassPoWResponseUnion, 
	/* name */ "PoWResponseUnion", 
	/* fields */ std::move(std::vector<ReflectField>{}), 
	/* size */ sizeof(PoWResponseUnion), 
	__reflectConstruct<PoWResponseUnion>,
	__reflectDestruct<PoWResponseUnion>),
ReflectType::ofClass(
	/* mine type id */ ReflectTypeID::ClassClientResponsePlaintext, 
	/* name */ "ClientResponsePlaintext", 
	/* fields */ std::move(std::vector<ReflectField>{ReflectField( /* typeID */ ReflectTypeID::ClassLoginCryptoResponseUnion, /* name */ "login_crypto_response", /* offset */ offsetof(ClientResponsePlaintext, login_crypto_response), /* protobuf tag */ 10),
ReflectField( /* typeID */ ReflectTypeID::ClassPoWResponseUnion, /* name */ "pow_response", /* offset */ offsetof(ClientResponsePlaintext, pow_response), /* protobuf tag */ 20),
ReflectField( /* typeID */ ReflectTypeID::ClassCryptoResponseUnion, /* name */ "crypto_response", /* offset */ offsetof(ClientResponsePlaintext, crypto_response), /* protobuf tag */ 30),
}), 
	/* size */ sizeof(ClientResponsePlaintext), 
	__reflectConstruct<ClientResponsePlaintext>,
	__reflectDestruct<ClientResponsePlaintext>),

	ReflectType::ofVector(
		/* mine typeId */ ReflectTypeID::VectorOfEnumCryptosuite,
		/* inner type id */  ReflectTypeID::EnumCryptosuite,
		/* size */ sizeof(std::vector<Cryptosuite>),
		VectorOperations{
			.push_back = __VectorManipulator<Cryptosuite>::push_back,
			.at = __VectorManipulator<Cryptosuite>::at,
			.size = __VectorManipulator<Cryptosuite>::size,
			.emplace_back =  __VectorManipulator<Cryptosuite>::emplace_back,
			.clear = __VectorManipulator<Cryptosuite>::clear,
			.reserve = __VectorManipulator<Cryptosuite>::reserve,
		},
		__reflectConstruct<std::vector<Cryptosuite>>,
		__reflectDestruct<std::vector<Cryptosuite>>
	)
	
	
	
	,

	ReflectType::ofOptional(
		/* mine typeId */ ReflectTypeID::OptionalOfClassFeatureSet,
		/* inner type id */  ReflectTypeID::ClassFeatureSet,
		/* size */ sizeof(std::optional<FeatureSet>),
		/* option */ OptionalOperations{
			.get = __OptionalManipulator<FeatureSet>::get,
			.has_value = __OptionalManipulator<FeatureSet>::has_value,
			.set = __OptionalManipulator<FeatureSet>::set,
			.reset = __OptionalManipulator<FeatureSet>::reset,
			.emplaceEmpty =  __OptionalManipulator<FeatureSet>::emplaceEmpty,
		},
		__reflectConstruct<std::optional<FeatureSet>>,
		__reflectDestruct<std::optional<FeatureSet>>
	)
	
	
	
	,
ReflectType::ofClass(
	/* mine type id */ ReflectTypeID::ClassClientHello, 
	/* name */ "ClientHello", 
	/* fields */ std::move(std::vector<ReflectField>{ReflectField( /* typeID */ ReflectTypeID::ClassBuildInfo, /* name */ "build_info", /* offset */ offsetof(ClientHello, build_info), /* protobuf tag */ 10),
ReflectField( /* typeID */ ReflectTypeID::ClassLoginCryptoHelloUnion, /* name */ "login_crypto_hello", /* offset */ offsetof(ClientHello, login_crypto_hello), /* protobuf tag */ 50),
ReflectField( /* typeID */ ReflectTypeID::VectorOfEnumCryptosuite, /* name */ "cryptosuites_supported", /* offset */ offsetof(ClientHello, cryptosuites_supported), /* protobuf tag */ 30),
ReflectField( /* typeID */ ReflectTypeID::VectorOfUint8, /* name */ "client_nonce", /* offset */ offsetof(ClientHello, client_nonce), /* protobuf tag */ 60),
ReflectField( /* typeID */ ReflectTypeID::OptionalOfVectorOfUint8, /* name */ "padding", /* offset */ offsetof(ClientHello, padding), /* protobuf tag */ 70),
ReflectField( /* typeID */ ReflectTypeID::OptionalOfClassFeatureSet, /* name */ "feature_set", /* offset */ offsetof(ClientHello, feature_set), /* protobuf tag */ 80),
}), 
	/* size */ sizeof(ClientHello), 
	__reflectConstruct<ClientHello>,
	__reflectDestruct<ClientHello>),
ReflectType::ofClass(
	/* mine type id */ ReflectTypeID::ClassHeader, 
	/* name */ "Header", 
	/* fields */ std::move(std::vector<ReflectField>{ReflectField( /* typeID */ ReflectTypeID::OptionalOfString, /* name */ "uri", /* offset */ offsetof(Header, uri), /* protobuf tag */ 1),
ReflectField( /* typeID */ ReflectTypeID::OptionalOfString, /* name */ "method", /* offset */ offsetof(Header, method), /* protobuf tag */ 3),
}), 
	/* size */ sizeof(Header), 
	__reflectConstruct<Header>,
	__reflectDestruct<Header>),
ReflectType::ofEnum(/* mine id */ ReflectTypeID::EnumAudioFormat, /* name */ "AudioFormat", /* enum values */ std::move(std::vector<ReflectEnumValue>{   ReflectEnumValue("OGG_VORBIS_96", 0),
   ReflectEnumValue("OGG_VORBIS_160", 1),
   ReflectEnumValue("OGG_VORBIS_320", 2),
   ReflectEnumValue("MP3_256", 3),
   ReflectEnumValue("MP3_320", 4),
   ReflectEnumValue("MP3_160", 5),
   ReflectEnumValue("MP3_96", 6),
   ReflectEnumValue("MP3_160_ENC", 7),
   ReflectEnumValue("AAC_24", 8),
   ReflectEnumValue("AAC_48", 9),
}), /* size */ sizeof(AudioFormat)),

	ReflectType::ofOptional(
		/* mine typeId */ ReflectTypeID::OptionalOfEnumAudioFormat,
		/* inner type id */  ReflectTypeID::EnumAudioFormat,
		/* size */ sizeof(std::optional<AudioFormat>),
		/* option */ OptionalOperations{
			.get = __OptionalManipulator<AudioFormat>::get,
			.has_value = __OptionalManipulator<AudioFormat>::has_value,
			.set = __OptionalManipulator<AudioFormat>::set,
			.reset = __OptionalManipulator<AudioFormat>::reset,
			.emplaceEmpty =  __OptionalManipulator<AudioFormat>::emplaceEmpty,
		},
		__reflectConstruct<std::optional<AudioFormat>>,
		__reflectDestruct<std::optional<AudioFormat>>
	)
	
	
	
	,
ReflectType::ofClass(
	/* mine type id */ ReflectTypeID::ClassAudioFile, 
	/* name */ "AudioFile", 
	/* fields */ std::move(std::vector<ReflectField>{ReflectField( /* typeID */ ReflectTypeID::OptionalOfVectorOfUint8, /* name */ "file_id", /* offset */ offsetof(AudioFile, file_id), /* protobuf tag */ 1),
ReflectField( /* typeID */ ReflectTypeID::OptionalOfEnumAudioFormat, /* name */ "format", /* offset */ offsetof(AudioFile, format), /* protobuf tag */ 2),
}), 
	/* size */ sizeof(AudioFile), 
	__reflectConstruct<AudioFile>,
	__reflectDestruct<AudioFile>),

	ReflectType::ofOptional(
		/* mine typeId */ ReflectTypeID::OptionalOfInt32,
		/* inner type id */  ReflectTypeID::Int32,
		/* size */ sizeof(std::optional<int32_t>),
		/* option */ OptionalOperations{
			.get = __OptionalManipulator<int32_t>::get,
			.has_value = __OptionalManipulator<int32_t>::has_value,
			.set = __OptionalManipulator<int32_t>::set,
			.reset = __OptionalManipulator<int32_t>::reset,
			.emplaceEmpty =  __OptionalManipulator<int32_t>::emplaceEmpty,
		},
		__reflectConstruct<std::optional<int32_t>>,
		__reflectDestruct<std::optional<int32_t>>
	)
	
	
	
	,

	ReflectType::ofVector(
		/* mine typeId */ ReflectTypeID::VectorOfClassAudioFile,
		/* inner type id */  ReflectTypeID::ClassAudioFile,
		/* size */ sizeof(std::vector<AudioFile>),
		VectorOperations{
			.push_back = __VectorManipulator<AudioFile>::push_back,
			.at = __VectorManipulator<AudioFile>::at,
			.size = __VectorManipulator<AudioFile>::size,
			.emplace_back =  __VectorManipulator<AudioFile>::emplace_back,
			.clear = __VectorManipulator<AudioFile>::clear,
			.reserve = __VectorManipulator<AudioFile>::reserve,
		},
		__reflectConstruct<std::vector<AudioFile>>,
		__reflectDestruct<std::vector<AudioFile>>
	)
	
	
	
	,
ReflectType::ofClass(
	/* mine type id */ ReflectTypeID::ClassTrack, 
	/* name */ "Track", 
	/* fields */ std::move(std::vector<ReflectField>{ReflectField( /* typeID */ ReflectTypeID::OptionalOfVectorOfUint8, /* name */ "gid", /* offset */ offsetof(Track, gid), /* protobuf tag */ 1),
ReflectField( /* typeID */ ReflectTypeID::OptionalOfString, /* name */ "name", /* offset */ offsetof(Track, name), /* protobuf tag */ 2),
ReflectField( /* typeID */ ReflectTypeID::OptionalOfInt32, /* name */ "duration", /* offset */ offsetof(Track, duration), /* protobuf tag */ 7),
ReflectField( /* typeID */ ReflectTypeID::VectorOfClassAudioFile, /* name */ "file", /* offset */ offsetof(Track, file), /* protobuf tag */ 12),
}), 
	/* size */ sizeof(Track), 
	__reflectConstruct<Track>,
	__reflectDestruct<Track>),
ReflectType::ofClass(
	/* mine type id */ ReflectTypeID::ClassEpisode, 
	/* name */ "Episode", 
	/* fields */ std::move(std::vector<ReflectField>{ReflectField( /* typeID */ ReflectTypeID::OptionalOfVectorOfUint8, /* name */ "gid", /* offset */ offsetof(Episode, gid), /* protobuf tag */ 1),
ReflectField( /* typeID */ ReflectTypeID::OptionalOfString, /* name */ "name", /* offset */ offsetof(Episode, name), /* protobuf tag */ 2),
ReflectField( /* typeID */ ReflectTypeID::OptionalOfInt32, /* name */ "duration", /* offset */ offsetof(Episode, duration), /* protobuf tag */ 7),
ReflectField( /* typeID */ ReflectTypeID::VectorOfClassAudioFile, /* name */ "audio", /* offset */ offsetof(Episode, audio), /* protobuf tag */ 12),
}), 
	/* size */ sizeof(Episode), 
	__reflectConstruct<Episode>,
	__reflectDestruct<Episode>),
ReflectType::ofEnum(/* mine id */ ReflectTypeID::EnumMessageType, /* name */ "MessageType", /* enum values */ std::move(std::vector<ReflectEnumValue>{   ReflectEnumValue("kMessageTypeHello", 1),
   ReflectEnumValue("kMessageTypeGoodbye", 2),
   ReflectEnumValue("kMessageTypeProbe", 3),
   ReflectEnumValue("kMessageTypeNotify", 10),
   ReflectEnumValue("kMessageTypeLoad", 20),
   ReflectEnumValue("kMessageTypePlay", 21),
   ReflectEnumValue("kMessageTypePause", 22),
   ReflectEnumValue("kMessageTypePlayPause", 23),
   ReflectEnumValue("kMessageTypeSeek", 24),
   ReflectEnumValue("kMessageTypePrev", 25),
   ReflectEnumValue("kMessageTypeNext", 26),
   ReflectEnumValue("kMessageTypeVolume", 27),
   ReflectEnumValue("kMessageTypeShuffle", 28),
   ReflectEnumValue("kMessageTypeRepeat", 29),
   ReflectEnumValue("kMessageTypeVolumeDown", 31),
   ReflectEnumValue("kMessageTypeVolumeUp", 32),
   ReflectEnumValue("kMessageTypeReplace", 33),
   ReflectEnumValue("kMessageTypeLogout", 34),
   ReflectEnumValue("kMessageTypeAction", 35),
}), /* size */ sizeof(MessageType)),
ReflectType::ofEnum(/* mine id */ ReflectTypeID::EnumPlayStatus, /* name */ "PlayStatus", /* enum values */ std::move(std::vector<ReflectEnumValue>{   ReflectEnumValue("kPlayStatusStop", 0),
   ReflectEnumValue("kPlayStatusPlay", 1),
   ReflectEnumValue("kPlayStatusPause", 2),
   ReflectEnumValue("kPlayStatusLoading", 3),
}), /* size */ sizeof(PlayStatus)),
ReflectType::ofEnum(/* mine id */ ReflectTypeID::EnumCapabilityType, /* name */ "CapabilityType", /* enum values */ std::move(std::vector<ReflectEnumValue>{   ReflectEnumValue("kSupportedContexts", 1),
   ReflectEnumValue("kCanBePlayer", 2),
   ReflectEnumValue("kRestrictToLocal", 3),
   ReflectEnumValue("kDeviceType", 4),
   ReflectEnumValue("kGaiaEqConnectId", 5),
   ReflectEnumValue("kSupportsLogout", 6),
   ReflectEnumValue("kIsObservable", 7),
   ReflectEnumValue("kVolumeSteps", 8),
   ReflectEnumValue("kSupportedTypes", 9),
   ReflectEnumValue("kCommandAcks", 10),
}), /* size */ sizeof(CapabilityType)),
ReflectType::ofClass(
	/* mine type id */ ReflectTypeID::ClassTrackRef, 
	/* name */ "TrackRef", 
	/* fields */ std::move(std::vector<ReflectField>{ReflectField( /* typeID */ ReflectTypeID::OptionalOfVectorOfUint8, /* name */ "gid", /* offset */ offsetof(TrackRef, gid), /* protobuf tag */ 1),
ReflectField( /* typeID */ ReflectTypeID::OptionalOfString, /* name */ "uri", /* offset */ offsetof(TrackRef, uri), /* protobuf tag */ 2),
ReflectField( /* typeID */ ReflectTypeID::OptionalOfBool, /* name */ "queued", /* offset */ offsetof(TrackRef, queued), /* protobuf tag */ 3),
ReflectField( /* typeID */ ReflectTypeID::OptionalOfString, /* name */ "context", /* offset */ offsetof(TrackRef, context), /* protobuf tag */ 4),
}), 
	/* size */ sizeof(TrackRef), 
	__reflectConstruct<TrackRef>,
	__reflectDestruct<TrackRef>),

	ReflectType::ofOptional(
		/* mine typeId */ ReflectTypeID::OptionalOfUint32,
		/* inner type id */  ReflectTypeID::Uint32,
		/* size */ sizeof(std::optional<uint32_t>),
		/* option */ OptionalOperations{
			.get = __OptionalManipulator<uint32_t>::get,
			.has_value = __OptionalManipulator<uint32_t>::has_value,
			.set = __OptionalManipulator<uint32_t>::set,
			.reset = __OptionalManipulator<uint32_t>::reset,
			.emplaceEmpty =  __OptionalManipulator<uint32_t>::emplaceEmpty,
		},
		__reflectConstruct<std::optional<uint32_t>>,
		__reflectDestruct<std::optional<uint32_t>>
	)
	
	
	
	,

	ReflectType::ofOptional(
		/* mine typeId */ ReflectTypeID::OptionalOfEnumPlayStatus,
		/* inner type id */  ReflectTypeID::EnumPlayStatus,
		/* size */ sizeof(std::optional<PlayStatus>),
		/* option */ OptionalOperations{
			.get = __OptionalManipulator<PlayStatus>::get,
			.has_value = __OptionalManipulator<PlayStatus>::has_value,
			.set = __OptionalManipulator<PlayStatus>::set,
			.reset = __OptionalManipulator<PlayStatus>::reset,
			.emplaceEmpty =  __OptionalManipulator<PlayStatus>::emplaceEmpty,
		},
		__reflectConstruct<std::optional<PlayStatus>>,
		__reflectDestruct<std::optional<PlayStatus>>
	)
	
	
	
	,

	ReflectType::ofOptional(
		/* mine typeId */ ReflectTypeID::OptionalOfUint64,
		/* inner type id */  ReflectTypeID::Uint64,
		/* size */ sizeof(std::optional<uint64_t>),
		/* option */ OptionalOperations{
			.get = __OptionalManipulator<uint64_t>::get,
			.has_value = __OptionalManipulator<uint64_t>::has_value,
			.set = __OptionalManipulator<uint64_t>::set,
			.reset = __OptionalManipulator<uint64_t>::reset,
			.emplaceEmpty =  __OptionalManipulator<uint64_t>::emplaceEmpty,
		},
		__reflectConstruct<std::optional<uint64_t>>,
		__reflectDestruct<std::optional<uint64_t>>
	)
	
	
	
	,

	ReflectType::ofVector(
		/* mine typeId */ ReflectTypeID::VectorOfClassTrackRef,
		/* inner type id */  ReflectTypeID::ClassTrackRef,
		/* size */ sizeof(std::vector<TrackRef>),
		VectorOperations{
			.push_back = __VectorManipulator<TrackRef>::push_back,
			.at = __VectorManipulator<TrackRef>::at,
			.size = __VectorManipulator<TrackRef>::size,
			.emplace_back =  __VectorManipulator<TrackRef>::emplace_back,
			.clear = __VectorManipulator<TrackRef>::clear,
			.reserve = __VectorManipulator<TrackRef>::reserve,
		},
		__reflectConstruct<std::vector<TrackRef>>,
		__reflectDestruct<std::vector<TrackRef>>
	)
	
	
	
	,
ReflectType::ofClass(
	/* mine type id */ ReflectTypeID::ClassState, 
	/* name */ "State", 
	/* fields */ std::move(std::vector<ReflectField>{ReflectField( /* typeID */ ReflectTypeID::OptionalOfString, /* name */ "context_uri", /* offset */ offsetof(State, context_uri), /* protobuf tag */ 2),
ReflectField( /* typeID */ ReflectTypeID::OptionalOfUint32, /* name */ "index", /* offset */ offsetof(State, index), /* protobuf tag */ 3),
ReflectField( /* typeID */ ReflectTypeID::OptionalOfUint32, /* name */ "position_ms", /* offset */ offsetof(State, position_ms), /* protobuf tag */ 4),
ReflectField( /* typeID */ ReflectTypeID::OptionalOfEnumPlayStatus, /* name */ "status", /* offset */ offsetof(State, status), /* protobuf tag */ 5),
ReflectField( /* typeID */ ReflectTypeID::OptionalOfUint64, /* name */ "position_measured_at", /* offset */ offsetof(State, position_measured_at), /* protobuf tag */ 7),
ReflectField( /* typeID */ ReflectTypeID::OptionalOfString, /* name */ "context_description", /* offset */ offsetof(State, context_description), /* protobuf tag */ 8),
ReflectField( /* typeID */ ReflectTypeID::OptionalOfBool, /* name */ "shuffle", /* offset */ offsetof(State, shuffle), /* protobuf tag */ 13),
ReflectField( /* typeID */ ReflectTypeID::OptionalOfBool, /* name */ "repeat", /* offset */ offsetof(State, repeat), /* protobuf tag */ 14),
ReflectField( /* typeID */ ReflectTypeID::OptionalOfUint32, /* name */ "playing_track_index", /* offset */ offsetof(State, playing_track_index), /* protobuf tag */ 26),
ReflectField( /* typeID */ ReflectTypeID::VectorOfClassTrackRef, /* name */ "track", /* offset */ offsetof(State, track), /* protobuf tag */ 27),
}), 
	/* size */ sizeof(State), 
	__reflectConstruct<State>,
	__reflectDestruct<State>),

	ReflectType::ofOptional(
		/* mine typeId */ ReflectTypeID::OptionalOfEnumCapabilityType,
		/* inner type id */  ReflectTypeID::EnumCapabilityType,
		/* size */ sizeof(std::optional<CapabilityType>),
		/* option */ OptionalOperations{
			.get = __OptionalManipulator<CapabilityType>::get,
			.has_value = __OptionalManipulator<CapabilityType>::has_value,
			.set = __OptionalManipulator<CapabilityType>::set,
			.reset = __OptionalManipulator<CapabilityType>::reset,
			.emplaceEmpty =  __OptionalManipulator<CapabilityType>::emplaceEmpty,
		},
		__reflectConstruct<std::optional<CapabilityType>>,
		__reflectDestruct<std::optional<CapabilityType>>
	)
	
	
	
	,

	ReflectType::ofVector(
		/* mine typeId */ ReflectTypeID::VectorOfInt64,
		/* inner type id */  ReflectTypeID::Int64,
		/* size */ sizeof(std::vector<int64_t>),
		VectorOperations{
			.push_back = __VectorManipulator<int64_t>::push_back,
			.at = __VectorManipulator<int64_t>::at,
			.size = __VectorManipulator<int64_t>::size,
			.emplace_back =  __VectorManipulator<int64_t>::emplace_back,
			.clear = __VectorManipulator<int64_t>::clear,
			.reserve = __VectorManipulator<int64_t>::reserve,
		},
		__reflectConstruct<std::vector<int64_t>>,
		__reflectDestruct<std::vector<int64_t>>
	)
	
	
	
	,

	ReflectType::ofVector(
		/* mine typeId */ ReflectTypeID::VectorOfString,
		/* inner type id */  ReflectTypeID::String,
		/* size */ sizeof(std::vector<std::string>),
		VectorOperations{
			.push_back = __VectorManipulator<std::string>::push_back,
			.at = __VectorManipulator<std::string>::at,
			.size = __VectorManipulator<std::string>::size,
			.emplace_back =  __VectorManipulator<std::string>::emplace_back,
			.clear = __VectorManipulator<std::string>::clear,
			.reserve = __VectorManipulator<std::string>::reserve,
		},
		__reflectConstruct<std::vector<std::string>>,
		__reflectDestruct<std::vector<std::string>>
	)
	
	
	
	,
ReflectType::ofClass(
	/* mine type id */ ReflectTypeID::ClassCapability, 
	/* name */ "Capability", 
	/* fields */ std::move(std::vector<ReflectField>{ReflectField( /* typeID */ ReflectTypeID::OptionalOfEnumCapabilityType, /* name */ "typ", /* offset */ offsetof(Capability, typ), /* protobuf tag */ 1),
ReflectField( /* typeID */ ReflectTypeID::VectorOfInt64, /* name */ "intValue", /* offset */ offsetof(Capability, intValue), /* protobuf tag */ 2),
ReflectField( /* typeID */ ReflectTypeID::VectorOfString, /* name */ "stringValue", /* offset */ offsetof(Capability, stringValue), /* protobuf tag */ 3),
}), 
	/* size */ sizeof(Capability), 
	__reflectConstruct<Capability>,
	__reflectDestruct<Capability>),

	ReflectType::ofOptional(
		/* mine typeId */ ReflectTypeID::OptionalOfInt64,
		/* inner type id */  ReflectTypeID::Int64,
		/* size */ sizeof(std::optional<int64_t>),
		/* option */ OptionalOperations{
			.get = __OptionalManipulator<int64_t>::get,
			.has_value = __OptionalManipulator<int64_t>::has_value,
			.set = __OptionalManipulator<int64_t>::set,
			.reset = __OptionalManipulator<int64_t>::reset,
			.emplaceEmpty =  __OptionalManipulator<int64_t>::emplaceEmpty,
		},
		__reflectConstruct<std::optional<int64_t>>,
		__reflectDestruct<std::optional<int64_t>>
	)
	
	
	
	,

	ReflectType::ofVector(
		/* mine typeId */ ReflectTypeID::VectorOfClassCapability,
		/* inner type id */  ReflectTypeID::ClassCapability,
		/* size */ sizeof(std::vector<Capability>),
		VectorOperations{
			.push_back = __VectorManipulator<Capability>::push_back,
			.at = __VectorManipulator<Capability>::at,
			.size = __VectorManipulator<Capability>::size,
			.emplace_back =  __VectorManipulator<Capability>::emplace_back,
			.clear = __VectorManipulator<Capability>::clear,
			.reserve = __VectorManipulator<Capability>::reserve,
		},
		__reflectConstruct<std::vector<Capability>>,
		__reflectDestruct<std::vector<Capability>>
	)
	
	
	
	,
ReflectType::ofClass(
	/* mine type id */ ReflectTypeID::ClassDeviceState, 
	/* name */ "DeviceState", 
	/* fields */ std::move(std::vector<ReflectField>{ReflectField( /* typeID */ ReflectTypeID::OptionalOfString, /* name */ "sw_version", /* offset */ offsetof(DeviceState, sw_version), /* protobuf tag */ 1),
ReflectField( /* typeID */ ReflectTypeID::OptionalOfBool, /* name */ "is_active", /* offset */ offsetof(DeviceState, is_active), /* protobuf tag */ 10),
ReflectField( /* typeID */ ReflectTypeID::OptionalOfBool, /* name */ "can_play", /* offset */ offsetof(DeviceState, can_play), /* protobuf tag */ 11),
ReflectField( /* typeID */ ReflectTypeID::OptionalOfUint32, /* name */ "volume", /* offset */ offsetof(DeviceState, volume), /* protobuf tag */ 12),
ReflectField( /* typeID */ ReflectTypeID::OptionalOfString, /* name */ "name", /* offset */ offsetof(DeviceState, name), /* protobuf tag */ 13),
ReflectField( /* typeID */ ReflectTypeID::OptionalOfUint32, /* name */ "error_code", /* offset */ offsetof(DeviceState, error_code), /* protobuf tag */ 14),
ReflectField( /* typeID */ ReflectTypeID::OptionalOfInt64, /* name */ "became_active_at", /* offset */ offsetof(DeviceState, became_active_at), /* protobuf tag */ 15),
ReflectField( /* typeID */ ReflectTypeID::OptionalOfString, /* name */ "error_message", /* offset */ offsetof(DeviceState, error_message), /* protobuf tag */ 16),
ReflectField( /* typeID */ ReflectTypeID::VectorOfClassCapability, /* name */ "capabilities", /* offset */ offsetof(DeviceState, capabilities), /* protobuf tag */ 17),
ReflectField( /* typeID */ ReflectTypeID::VectorOfString, /* name */ "local_uris", /* offset */ offsetof(DeviceState, local_uris), /* protobuf tag */ 18),
}), 
	/* size */ sizeof(DeviceState), 
	__reflectConstruct<DeviceState>,
	__reflectDestruct<DeviceState>),

	ReflectType::ofOptional(
		/* mine typeId */ ReflectTypeID::OptionalOfEnumMessageType,
		/* inner type id */  ReflectTypeID::EnumMessageType,
		/* size */ sizeof(std::optional<MessageType>),
		/* option */ OptionalOperations{
			.get = __OptionalManipulator<MessageType>::get,
			.has_value = __OptionalManipulator<MessageType>::has_value,
			.set = __OptionalManipulator<MessageType>::set,
			.reset = __OptionalManipulator<MessageType>::reset,
			.emplaceEmpty =  __OptionalManipulator<MessageType>::emplaceEmpty,
		},
		__reflectConstruct<std::optional<MessageType>>,
		__reflectDestruct<std::optional<MessageType>>
	)
	
	
	
	,

	ReflectType::ofOptional(
		/* mine typeId */ ReflectTypeID::OptionalOfClassDeviceState,
		/* inner type id */  ReflectTypeID::ClassDeviceState,
		/* size */ sizeof(std::optional<DeviceState>),
		/* option */ OptionalOperations{
			.get = __OptionalManipulator<DeviceState>::get,
			.has_value = __OptionalManipulator<DeviceState>::has_value,
			.set = __OptionalManipulator<DeviceState>::set,
			.reset = __OptionalManipulator<DeviceState>::reset,
			.emplaceEmpty =  __OptionalManipulator<DeviceState>::emplaceEmpty,
		},
		__reflectConstruct<std::optional<DeviceState>>,
		__reflectDestruct<std::optional<DeviceState>>
	)
	
	
	
	,

	ReflectType::ofOptional(
		/* mine typeId */ ReflectTypeID::OptionalOfClassState,
		/* inner type id */  ReflectTypeID::ClassState,
		/* size */ sizeof(std::optional<State>),
		/* option */ OptionalOperations{
			.get = __OptionalManipulator<State>::get,
			.has_value = __OptionalManipulator<State>::has_value,
			.set = __OptionalManipulator<State>::set,
			.reset = __OptionalManipulator<State>::reset,
			.emplaceEmpty =  __OptionalManipulator<State>::emplaceEmpty,
		},
		__reflectConstruct<std::optional<State>>,
		__reflectDestruct<std::optional<State>>
	)
	
	
	
	,
ReflectType::ofClass(
	/* mine type id */ ReflectTypeID::ClassFrame, 
	/* name */ "Frame", 
	/* fields */ std::move(std::vector<ReflectField>{ReflectField( /* typeID */ ReflectTypeID::OptionalOfUint32, /* name */ "version", /* offset */ offsetof(Frame, version), /* protobuf tag */ 1),
ReflectField( /* typeID */ ReflectTypeID::OptionalOfString, /* name */ "ident", /* offset */ offsetof(Frame, ident), /* protobuf tag */ 2),
ReflectField( /* typeID */ ReflectTypeID::OptionalOfString, /* name */ "protocol_version", /* offset */ offsetof(Frame, protocol_version), /* protobuf tag */ 3),
ReflectField( /* typeID */ ReflectTypeID::OptionalOfUint32, /* name */ "seq_nr", /* offset */ offsetof(Frame, seq_nr), /* protobuf tag */ 4),
ReflectField( /* typeID */ ReflectTypeID::OptionalOfEnumMessageType, /* name */ "typ", /* offset */ offsetof(Frame, typ), /* protobuf tag */ 5),
ReflectField( /* typeID */ ReflectTypeID::OptionalOfClassDeviceState, /* name */ "device_state", /* offset */ offsetof(Frame, device_state), /* protobuf tag */ 7),
ReflectField( /* typeID */ ReflectTypeID::OptionalOfClassState, /* name */ "state", /* offset */ offsetof(Frame, state), /* protobuf tag */ 12),
ReflectField( /* typeID */ ReflectTypeID::OptionalOfUint32, /* name */ "position", /* offset */ offsetof(Frame, position), /* protobuf tag */ 13),
ReflectField( /* typeID */ ReflectTypeID::OptionalOfUint32, /* name */ "volume", /* offset */ offsetof(Frame, volume), /* protobuf tag */ 14),
ReflectField( /* typeID */ ReflectTypeID::OptionalOfInt64, /* name */ "state_update_id", /* offset */ offsetof(Frame, state_update_id), /* protobuf tag */ 17),
ReflectField( /* typeID */ ReflectTypeID::VectorOfString, /* name */ "recipient", /* offset */ offsetof(Frame, recipient), /* protobuf tag */ 18),
}), 
	/* size */ sizeof(Frame), 
	__reflectConstruct<Frame>,
	__reflectDestruct<Frame>),
};

