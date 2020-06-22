#include "mneme/plan.hpp"
#include "mneme/storage.hpp"
#include "mneme/view.hpp"

#include "doctest.h"

#include <array>
#include <iostream>
#include <memory>

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

    using aos_t = MultiStorage<DataLayout::AoS, material, bc>;
    using soa_t = MultiStorage<DataLayout::SoA, material, bc>;
    auto localAoS = std::make_shared<aos_t>(localLayout.back());
    auto localSoA = std::make_shared<soa_t>(localLayout.back());

    auto testMaterial = [](auto&& X) {
        for (int i = 0; i < static_cast<int>(X.size()); ++i) {
            X[i].template get<material>() = ElasticMaterial{1.0 * i, 1.0 * i, 2.0 * i};
        }
        int i = 0;
        for (auto&& x : X) {
            REQUIRE(x.template get<material>().lambda == 2.0 * i++);
        }
    };

    SUBCASE("MultiStorage AoS works") { testMaterial(*localAoS); }

    SUBCASE("MultiStorage SoA works") { testMaterial(*localSoA); }

    SUBCASE("DenseView AoS works") {
        DenseView<aos_t> localView(localLayout, localAoS, NghostP1, N);
        testMaterial(localView);
    }

    SUBCASE("DenseView SoA works") {
        DenseView<soa_t> localView(localLayout, localSoA, NghostP1, N);
        testMaterial(localView);
    }

    SUBCASE("DenseView AoS works") {
        DenseView<aos_t> localView(localLayout, localAoS, NghostP1, N);
        testMaterial(localView);
    }

    SUBCASE("SingleStorage works") {
        using dofs_storage_t = SingleStorage<dofs>;
        auto dofsC = std::make_shared<dofs_storage_t>(dofsLayout.back());
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
            REQUIRE((*dofsC)[j] == j / 4 + 4 * (j % 4));
        }
    }
}
