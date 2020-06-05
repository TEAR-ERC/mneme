#ifndef MNEME_ALLOCATORS_H
#define MNEME_ALLOCATORS_H

#include <cstdlib>
#include <iostream>
#include <limits>
#include <new>

namespace mneme {

struct AlignedAllocatorBase {};
template <class T, std::size_t Alignment> struct AlignedAllocator : public AlignedAllocatorBase {
    using value_type = T;
    constexpr static std::size_t alignment = Alignment;

    AlignedAllocator() = default;
    template <class U, std::size_t OtherAlignment>
    constexpr explicit AlignedAllocator(const AlignedAllocator<U, OtherAlignment>&) noexcept {}

    template <class U> struct rebind { typedef AlignedAllocator<U, Alignment> other; };

    [[nodiscard]] T* allocate(std::size_t n) {
        if (n > std::numeric_limits<std::size_t>::max() / sizeof(T))
            throw std::bad_alloc();
        T* ptr;
        // Note: The alignment has to satisfy the following requirements
        // See also: https://www.man7.org/linux/man-pages/man3/posix_memalign.3.html
        constexpr bool isPowerOfTwo = (Alignment > 0 && ((Alignment & (Alignment - 1)) == 0));
        static_assert(isPowerOfTwo, "Alignment has to be a power of two");
        constexpr bool isFactorOfPtrSize = Alignment % sizeof(void*) == 0;
        static_assert(isFactorOfPtrSize, "Alignment has to be a constant of void ptr size");
        ptr = reinterpret_cast<T*>(std::aligned_alloc(Alignment, n * sizeof(T)));
        auto isError = ptr == nullptr;

        if (isError) {
            throw std::bad_alloc();
        }

        return ptr;
    }
    void deallocate(T* ptr, std::size_t) noexcept { std::free(ptr); }
};

template <class T, std::size_t Alignment, class U, std::size_t OtherAlignment>
bool operator==(const AlignedAllocator<T, Alignment>&, const AlignedAllocator<U, OtherAlignment>&) {
    return true;
}
template <class T, std::size_t Alignment, class U, std::size_t OtherAlignment>
bool operator!=(const AlignedAllocator<T, Alignment>&, const AlignedAllocator<U, OtherAlignment>&) {
    return false;
}

template <typename... List> struct AllocatorInfo;

template <typename Head, typename... Tail> struct AllocatorInfo<Head, Tail...> {
    template <typename Allocator> static constexpr bool allSameAllocatorAs() {
        using own_t = typename Head::allocator;
        return std::is_base_of_v<Allocator, own_t> &&
               AllocatorInfo<Tail...>::template allSameAllocatorAs<Allocator>();
    }

    static constexpr bool allSameAllocator() {
        using own_t = typename Head::allocator;
        if constexpr (std::is_base_of_v<AlignedAllocatorBase, own_t>) {
            return allSameAllocatorAs<AlignedAllocatorBase>();
        } else {
            return allSameAllocatorAs<own_t>();
        }
    }

    static constexpr std::size_t getMaxAlignment() {
        using own_t = typename Head::allocator;
        static_assert(std::is_base_of_v<AlignedAllocatorBase, own_t>,
                      "Maximum alignment is only defined if all allocators are AlignedAllocator");
        return std::max(AllocatorInfo<Tail...>::getMaxAlignment(), own_t::alignment);
    }
};

template <> struct AllocatorInfo<> {
    template <typename Allocator> static auto allSameAllocatorAs() { return true; }

    static constexpr bool allSameAllocator() { return true; }
    static constexpr std::size_t getMaxAlignment() { return 0; }
};

template <typename... Args> bool allSameAllocator() {
    return AllocatorInfo<Args...>::allSameAllocator();
}
template <typename... Args> constexpr size_t getMaxAlignment() {
    constexpr auto maxAlignment = AllocatorInfo<Args...>::getMaxAlignment();
    return maxAlignment;
}
} // namespace mneme

#endif // MNEME_ALLOCATORS_H
