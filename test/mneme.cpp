#include <array>
#include <functional>
#include <iostream>
#include <optional>

#include "mneme/plan.hpp"
#include "mneme/storage.hpp"
#include "mneme/view.hpp"

#include "doctest.h"

using namespace mneme;

struct ElasticMaterial {
    double rho;
    double mu;
    double lambda;
};

struct dofs {
    using type = double;
};
struct material {
    using type = ElasticMaterial;
};
struct bc {
    using type = std::array<int, 4>;
};

struct Ghost : public Layer {};
struct Interior : public Layer {};
struct Copy : public Layer {};

TEST_CASE("Data structure works") {
    int NghostP1 = 5;
    int NinteriorP1 = 100;
    int NinteriorP2 = 50;
    int N = NghostP1 + NinteriorP1 + NinteriorP2;

    Plan localPlan(N);
    for (int i = NghostP1; i < NghostP1 + NinteriorP1 + NinteriorP2; ++i) {
        localPlan.setDof(i, 1); // Store one object per i
    }
    auto localLayout = localPlan.getLayout();

    Plan dofsPlan(N);
    int idx = 0;
    for (int i = 0; i < NghostP1 + NinteriorP1; ++i) {
        dofsPlan.setDof(idx++, 4); // Store P1 element per i
    }
    for (int i = 0; i < NinteriorP2; ++i) {
        dofsPlan.setDof(idx++, 10); // Store P2 element per i
    }
    auto dofsLayout = dofsPlan.getLayout();

    SUBCASE("MultiStorage works") {
        using local_storage_t = MultiStorage<DataLayout::SoA, material, bc>;
        local_storage_t localC(localLayout.back());
        DenseView<local_storage_t> localView(localLayout, localC, NghostP1, N);
        for (int i = 0; i < static_cast<int>(localView.size()); ++i) {
            localView[i].get<material>() = ElasticMaterial{1.0 * i, 1.0 * i, 2.0 * i};
        }
        int i = 0;
        for (auto&& v : localView) {
            REQUIRE(v.get<material>().lambda == 2.0 * i++);
        }
    }

    SUBCASE("SingleStorage works") {
        using dofs_storage_t = SingleStorage<dofs>;
        dofs_storage_t dofsC(dofsLayout.back());
        StridedView<dofs_storage_t, 4U> dofsV(dofsLayout, dofsC, 0, NghostP1 + NinteriorP1);
        int k = 0;
        int l = 0;
        for (auto&& v : dofsV) {
            l = 0;
            for (auto&& vv : v) {
                vv = k + 4 * l++;
            }
            ++k;
        }
        for (int j = 0; j < NghostP1 + NinteriorP1; ++j) {
            REQUIRE(dofsC[j] == j / 4 + 4 * (j % 4));
        }
    }
}

TEST_CASE("Layered Plans") {
    constexpr size_t numInterior = 100;
    constexpr size_t numCopy = 30;
    constexpr size_t numGhost = 20;
    constexpr size_t numDofsInterior = 10;
    constexpr size_t numDofsCopy = 7;
    constexpr size_t numDofsGhost = 4;
    constexpr auto dofsInterior = [numDofsInterior](auto) { return numDofsInterior; };
    constexpr auto dofsCopy = [numDofsCopy](auto) { return numDofsCopy; };
    constexpr auto dofsGhost = [numDofsGhost](auto) { return numDofsGhost; };
    const auto plan = LayeredPlan()
                          .setDofs<Interior>(numInterior, dofsInterior)
                          .setDofs<Copy>(numCopy, dofsCopy)
                          .setDofs<Ghost>(numGhost, dofsGhost);
    CHECK(std::is_same_v<decltype(plan), const LayeredPlan<Interior, Copy, Ghost>>);
    const auto localLayout = plan.getLayout();

    CHECK(localLayout.size() == numInterior + numCopy + numGhost);
    size_t curOffset = 0;
    size_t i = 0;
    for (; i < numInterior; ++i) {
        CHECK(localLayout[i] == curOffset);
        curOffset += numDofsInterior;
    }
    for (; i < numCopy; ++i) {
        CHECK(localLayout[i] == curOffset);
        curOffset += numDofsCopy;
    }
    for (; i < numGhost; ++i) {
        CHECK(localLayout[i] == curOffset);
        curOffset += numDofsGhost;
    }

    SUBCASE("SingleStorage works") {
        using dofs_storage_t = SingleStorage<dofs>;
        dofs_storage_t dofsC(localLayout.back());

        auto viewFactory = createView().withStride<10>().withPlan(plan).withStorage(dofsC);
        // auto view = viewFactory.createView<Ghost>();
        // auto dofsV = viewFactory.createView<Interior>();
        auto dofsV =
            stridedViewFromLayer<Interior, decltype(plan), dofs_storage_t, 10U>(plan, dofsC);
        // StridedView<dofs_storage_t, numDofsInterior> dofsV(localLayout, dofsC, 0, numInterior);
        int k = 0;
        int l = 0;
        for (auto&& v : dofsV) {
            l = 0;
            for (auto&& vv : v) {
                vv = k + 10 * l++;
            }
            ++k;
        }
        for (int j = 0; j < numInterior; ++j) {
            CHECK(dofsC[j] == j / 10 + 10 * (j % 10));
        }
    }
}
