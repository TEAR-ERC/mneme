#ifndef MNEME_UTIL_H
#define MNEME_UTIL_H

struct StaticNothing {};

template <typename T> struct StaticSome {
    constexpr explicit StaticSome(T value) : value(value) {}
    using type = T;
    T value;
};
#endif // MNEME_UTIL_H
