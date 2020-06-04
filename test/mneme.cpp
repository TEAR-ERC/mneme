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
    constexpr std::size_t numInterior = 100;
    constexpr std::size_t numCopy = 30;
    constexpr std::size_t numGhost = 20;

    constexpr std::size_t numMaterial = 1;
    constexpr auto numMaterialFunc = [numMaterial](auto) { return numMaterial; };
    const auto localPlan = LayeredPlan()
                               .withDofs<Interior>(numInterior, numMaterialFunc)
                               .withDofs<Copy>(numInterior, numMaterialFunc)
                               .withDofs<Ghost>(numInterior, numMaterialFunc);
    const auto& localLayout = localPlan.getLayout();
    CHECK(std::is_same_v<decltype(localPlan), const LayeredPlan<Interior, Copy, Ghost>>);

    SUBCASE("Layout is cached") {
        const auto& localLayout2 = localPlan.getLayout();
        CHECK(&localLayout == &localLayout2);
        CHECK(localLayout.data() == localLayout2.data());
    }
    SUBCASE("Caching of layout is invalidated") {
        const auto tmpPlan = LayeredPlan()
                                 .withDofs<Interior>(numInterior, numMaterialFunc)
                                 .withDofs<Copy>(numInterior, numMaterialFunc);
        [[maybe_unused]] const auto& tmpLayout = tmpPlan.getLayout();
        const auto localPlan2 = tmpPlan.withDofs<Ghost>(numInterior, numMaterialFunc);
        const auto& localLayout2 = localPlan2.getLayout();
        CHECK(localLayout.size() == localLayout2.size());
        for (std::size_t i = 0; i < localLayout.size(); ++i) {
            CHECK(localLayout[i] == localLayout2[i]);
        }
    }

    constexpr std::size_t numDofsInterior = 10;
    constexpr std::size_t numDofsCopy = 7;
    constexpr std::size_t numDofsGhost = 4;
    constexpr auto dofsInterior = [numDofsInterior](auto) { return numDofsInterior; };
    constexpr auto dofsCopy = [numDofsCopy](auto) { return numDofsCopy; };
    constexpr auto dofsGhost = [numDofsGhost](auto) { return numDofsGhost; };
    const auto dofsPlan = LayeredPlan()
                              .withDofs<Interior>(numInterior, dofsInterior)
                              .withDofs<Copy>(numCopy, dofsCopy)
                              .withDofs<Ghost>(numGhost, dofsGhost);
    CHECK(std::is_same_v<decltype(dofsPlan), const LayeredPlan<Interior, Copy, Ghost>>);
    const auto dofsLayout = dofsPlan.getLayout();

    CHECK(dofsLayout.size() == numInterior + numCopy + numGhost);
    std::size_t curOffset = 0;
    std::size_t i = 0;
    for (; i < numInterior; ++i) {
        CHECK(dofsLayout[i] == curOffset);
        curOffset += numDofsInterior;
    }
    for (; i < numCopy; ++i) {
        CHECK(dofsLayout[i] == curOffset);
        curOffset += numDofsCopy;
    }
    for (; i < numGhost; ++i) {
        CHECK(dofsLayout[i] == curOffset);
        curOffset += numDofsGhost;
    }

    SUBCASE("MultiStorage works") {
        using local_storage_t = MultiStorage<DataLayout::SoA, material, bc>;
        local_storage_t localC(localLayout.back());
        auto materialViewFactory = createViewFactory().withPlan(localPlan).withStorage(localC);
        auto localViewCopy = materialViewFactory.createDenseView<Copy>();

        for (int i = 0; i < static_cast<int>(localViewCopy.size()); ++i) {
            localViewCopy[i].get<material>() = ElasticMaterial{1.0 * i, 1.0 * i, 2.0 * i};
        }
        int i = 0;
        for (auto&& v : localViewCopy) {
            REQUIRE(v.get<material>().lambda == 2.0 * i++);
        }
    }
    SUBCASE("SingleStorage works") {
        using dofs_storage_t = SingleStorage<dofs>;
        dofs_storage_t dofsC(dofsLayout.back());

        auto dofsViewFactory = createViewFactory().withPlan(dofsPlan).withStorage(dofsC);
        auto dofsViewInterior =
            dofsViewFactory.withStride<numDofsInterior>().createStridedView<Interior>();
        int k = 0;
        int l = 0;
        for (auto&& v : dofsViewInterior) {
            l = 0;
            for (auto&& vv : v) {
                vv = k + 10 * l++;
            }
            ++k;
        }
        for (std::size_t j = 0; j < numInterior; ++j) {
            CHECK(dofsC[j] == j / 10 + 10 * (j % 10));
        }
    }
    SUBCASE("Combined plan") {
        auto plans = std::vector{localPlan, localPlan};
        auto combinedPlan = CombinedLayeredPlan<Interior, Copy, Ghost>(plans);
        using local_storage_t = MultiStorage<DataLayout::SoA, material, bc>;
        local_storage_t localC(localLayout.back());
        auto combinedLayout = combinedPlan.getLayout();
        auto materialViewFactory = createViewFactory().withPlan(combinedPlan).withStorage(localC);
        auto localViewCopy = materialViewFactory.withClusterId(0).createDenseView<Copy>();
        for (int i = 0; i < 2; ++i) {
            std::cout << "Cluster = " << i << std::endl;
            auto interior = combinedPlan.getLayer<Interior>(i);
            std::cout << "Interior:\t" << interior.numElements << " " << interior.offset
                      << std::endl;
            auto copy = combinedPlan.getLayer<Copy>(i);
            std::cout << "Copy:\t" << copy.numElements << " " << copy.offset
                      << std::endl;
            auto ghost = combinedPlan.getLayer<Ghost>(i);
            std::cout << "Ghost:\t" << ghost.numElements << " " << ghost.offset
                      << std::endl;}
    }

}
