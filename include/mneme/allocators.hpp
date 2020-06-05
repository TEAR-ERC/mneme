#ifndef MNEME_ALLOCATORS_H
#define MNEME_ALLOCATORS_H

#include <cstdlib>
#include <iostream>
#include <limits>
#include <new>

namespace mneme {

template <class T, std::size_t Alignment> struct AlignedAllocator {
    using value_type = T;

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
bool operator!=(const AlignedAllocator<T, Alignment>& a,
                const AlignedAllocator<U, OtherAlignment>& b) {
    return false;
}

} // namespace mneme

#endif // MNEME_ALLOCATORS_H
