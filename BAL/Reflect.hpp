#pragma once

#include <Utils/Reflect.hpp>

#ifdef BAL_PLATFORM_PEERPLAYS
#include <fc/reflect/reflect.hpp>

#define BAL_REFLECT(...) REFLECT(__VA_ARGS__) FC_REFLECT(__VA_ARGS__)
#define BAL_REFLECT_ENUM(...) FC_REFLECT_ENUM(__VA_ARGS__)
#define BAL_REFLECT_TYPENAME(...) FC_REFLECT_TYPENAME(__VA_ARGS__)
#else
#define BAL_REFLECT(...) REFLECT(__VA_ARGS__)
#define BAL_REFLECT_ENUM(...) /* NYI */
#define BAL_REFLECT_TYPENAME(...) /* NYI */
#endif
