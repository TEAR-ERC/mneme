#include "mneme/allocators.hpp"
#include "doctest.h"
#include "mneme/plan.hpp"
#include "mneme/storage.hpp"
#include "mneme/view.hpp"
#include <vector>

using namespace mneme;

template <typename T> void checkPointerAlignment(T* ptr, std::size_t alignment) {
    CHECK(reinterpret_cast<std::size_t>(ptr) % alignment == 0);
}

TEST_CASE("Aligned allocator works") {
    // Note: Alignment has to be carefully chosen here,
    // as often pointers are aligned even with default allocator.
    // E.g. the test would pass with alignment=32 even with malloc
    // on my machine.
    constexpr static auto alignment = 1024u;
    using value_t = int;
    using allocator_t = AlignedAllocator<value_t, alignment>;
    using allocator_traits_t = std::allocator_traits<allocator_t>;
    auto allocator = allocator_t();
    auto mem = allocator_traits_t::allocate(allocator, 100);
    checkPointerAlignment(mem, alignment);

    SUBCASE("Aligned allocator can be used with std containers") {
        auto alignedVec = std::vector<value_t, allocator_t>(1000);
        checkPointerAlignment(alignedVec.data(), alignment);
    }
}

constexpr static std::size_t alignment = 2048 * 4;
struct dofs {
    using type = double;
    using allocator = AlignedAllocator<type, alignment>;
};

TEST_CASE("Storage works with aligned allocator") {
    constexpr std::size_t numElements = 10;
    constexpr std::size_t numDofs = 7;
    Plan dofsPlan(numElements);
    int idx = 0;
    for (std::size_t i = 0; i < numElements; ++i) {
        dofsPlan.setDof(idx++, numDofs);
    }
    auto dofsLayout = dofsPlan.getLayout();
    using dofs_storage_t = SingleStorage<dofs>;
    auto dofsC = dofs_storage_t(dofsLayout.back());
    auto dofsV = StridedView<dofs_storage_t, numDofs>(dofsLayout, dofsC, 0, numElements);

    checkPointerAlignment(dofsV[0].data(), alignment);
}
