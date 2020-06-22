#ifndef MNEME_STORAGE_H_
#define MNEME_STORAGE_H_

#include <cstddef>
#include <tuple>
#include <type_traits>

#include "iterator.hpp"
#include "span.hpp"
#include "tagged_tuple.hpp"

namespace mneme {

// Storage classes inspired by
// https://github.com/crosetto/SoAvsAoS

enum class DataLayout { SoA, AoS };

namespace detail {
template <DataLayout TDataLayout, typename... Ids> struct DataLayoutAllocatePolicy;

template <DataLayout TDataLayout, std::size_t Extent, typename... Ids>
struct DataLayoutAccessPolicy;

template <typename... Ids> struct DataLayoutAllocatePolicy<DataLayout::AoS, Ids...> {
    using type = tagged_tuple<Ids...>*;

    constexpr static void allocate(type& c, std::size_t size) {
        c = new tagged_tuple<Ids...>[size];
    }
    constexpr static void deallocate(type& c) { delete[] c; }

    constexpr static type offset(type& c, std::size_t from) { return c + from; }

    constexpr static type null() { return nullptr; }
};

template <typename... Ids> struct DataLayoutAccessPolicy<DataLayout::AoS, 1u, Ids...> {
    using type = typename DataLayoutAllocatePolicy<DataLayout::AoS, Ids...>::type;
    using value_type = tagged_tuple<Ids...>&;

    constexpr static value_type get(type& c, std::size_t from, std::size_t) { return c[from]; }
};

template <std::size_t Extent, typename... Ids>
struct DataLayoutAccessPolicy<DataLayout::AoS, Extent, Ids...> {
    using type = typename DataLayoutAllocatePolicy<DataLayout::AoS, Ids...>::type;
    using value_type = const span<tagged_tuple<Ids...>, Extent>;

    constexpr static value_type get(type& c, std::size_t from, std::size_t to) {
        return value_type(&c[from], to - from);
    }
};

template <typename... Ids> struct DataLayoutAllocatePolicy<DataLayout::SoA, Ids...> {
    using type = detail::tt_impl<std::add_pointer, Ids...>;

    constexpr static void allocate(type& c, std::size_t size) {
        ((c.template get<Ids>() = new typename Ids::type[size]), ...);
    }

    constexpr static void deallocate(type& c) { ((delete[] c.template get<Ids>()), ...); }

    constexpr static type offset(type& c, std::size_t from) {
        return type{(c.template get<Ids>() + from)...};
    }

    constexpr static type null() {
        return type{static_cast<std::add_pointer_t<typename Ids::type>>(nullptr)...};
    }
};

template <typename... Ids> struct DataLayoutAccessPolicy<DataLayout::SoA, 1u, Ids...> {
    using type = typename DataLayoutAllocatePolicy<DataLayout::SoA, Ids...>::type;
    using value_type = const detail::tt_impl<std::add_lvalue_reference, Ids...>;

    constexpr static value_type get(type& c, std::size_t from, std::size_t) {
        return value_type{c.template get<Ids>()[from]...};
    }
};

template <std::size_t Extent, typename... Ids>
struct DataLayoutAccessPolicy<DataLayout::SoA, Extent, Ids...> {
    using type = typename DataLayoutAllocatePolicy<DataLayout::SoA, Ids...>::type;
    template <typename T> struct add_span { using type = const span<T, Extent>; };
    using value_type = const detail::tt_impl<add_span, Ids...>;

    constexpr static value_type get(type& c, std::size_t from, std::size_t to) {
        return value_type{
            span<typename Ids::type, Extent>(&c.template get<Ids>()[from], to - from)...};
    }
};
} // namespace detail

template <DataLayout TDataLayout, typename... Ids> class MultiStorage {
public:
    using allocate_policy_t = detail::DataLayoutAllocatePolicy<TDataLayout, Ids...>;
    using iterator = Iterator<MultiStorage<TDataLayout, Ids...>>;
    using type = typename allocate_policy_t::type;
    using offset_type = type;

    template <std::size_t Extent>
    using access_policy_t = detail::DataLayoutAccessPolicy<TDataLayout, Extent, Ids...>;
    template <std::size_t Extent> using value_type = typename access_policy_t<Extent>::value_type;

    MultiStorage() {}

    MultiStorage(std::size_t size) : size_(size) { allocate_policy_t::allocate(values, size); }

    ~MultiStorage() { allocate_policy_t::deallocate(values); }

    value_type<1u> operator[](std::size_t pos) noexcept {
        return access_policy_t<1u>::get(values, pos, pos + 1u);
    }

    template <std::size_t Extent = dynamic_extent>
    value_type<Extent> get(offset_type& offset, std::size_t from, std::size_t to) noexcept {
        return access_policy_t<Extent>::get(offset, from, to);
    }

    void resize(std::size_t size) {
        size_ = size;
        allocate_policy_t::deallocate(values);
        allocate_policy_t::allocate(values, size);
    }

    offset_type offset(std::size_t from) { return allocate_policy_t::offset(values, from); }

    inline std::size_t size() const noexcept { return size_; }

    iterator begin() { return iterator(this, 0); }
    iterator end() { return iterator(this, size()); }

private:
    std::size_t size_ = 0u;
    type values = allocate_policy_t::null();
};

template <typename Id> class SingleStorage : public MultiStorage<DataLayout::SoA, Id> {
public:
    using storage_t = MultiStorage<DataLayout::SoA, Id>;
    using storage_t::storage_t;
    using typename storage_t::offset_type;
    template <std::size_t Extent>
    using value_type = typename storage_t::template value_type<Extent>::template element_t<Id>;

    auto& operator[](std::size_t pos) noexcept {
        return storage_t::operator[](pos).template get<Id>();
    }

    template <std::size_t Extent = dynamic_extent>
    value_type<Extent> get(offset_type& offset, std::size_t from, std::size_t to) noexcept {
        return storage_t::template get<Extent>(offset, from, to).template get<Id>();
    }
};

} // namespace mneme

#endif // MNEME_STORAGE_H_
