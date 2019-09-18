#pragma once
#include <magic_enum.hpp>
#include <nlohmann/json.hpp>

#define JSON_ENUM(ENUM_TYPE)                                                                                                     \
    template <typename BasicJsonType>                                                                                            \
    inline void to_json(BasicJsonType& j, const ENUM_TYPE& e) {                                                                  \
        static_assert(std::is_enum<ENUM_TYPE>::value, #ENUM_TYPE " must be an enum!");                                           \
        static const auto m = magic_enum::enum_entries<ENUM_TYPE>();                                                             \
        auto it = std::find_if(std::begin(m), std::end(m),                                                                       \
                               [e](const std::pair<ENUM_TYPE, BasicJsonType>& ej_pair) -> bool { return ej_pair.first == e; });  \
        j = ((it != std::end(m)) ? it : std::begin(m))->second;                                                                  \
    }                                                                                                                            \
    template <typename BasicJsonType>                                                                                            \
    inline void from_json(const BasicJsonType& j, ENUM_TYPE& e) {                                                                \
        static_assert(std::is_enum<ENUM_TYPE>::value, #ENUM_TYPE " must be an enum!");                                           \
        static const auto m = magic_enum::enum_entries<ENUM_TYPE>();                                                             \
        auto it = std::find_if(std::begin(m), std::end(m),                                                                       \
                               [j](const std::pair<ENUM_TYPE, BasicJsonType>& ej_pair) -> bool { return ej_pair.second == j; }); \
        e = ((it != std::end(m)) ? it : std::begin(m))->first;                                                                   \
    }