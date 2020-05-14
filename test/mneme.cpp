#include <iostream>
#include <array>

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

struct dofs { using type = double; };
struct material { using type = ElasticMaterial; };
struct bc { using type = std::array<int, 4>; };

TEST_CASE( "Data structure works") {
    int NghostP1 = 5;
    int NinteriorP1 = 100;
    int NinteriorP2 = 50;
    int N = NghostP1 + NinteriorP1 + NinteriorP2;

    Plan localPlan(N);
    for (int i = NghostP1; i < NghostP1 + NinteriorP1 + NinteriorP2; ++i) {
        localPlan.setDof(i,1); // Store one object per i
    }
    auto localLayout = localPlan.getLayout();

    Plan dofsPlan(N);
    int idx = 0;
    for (int i = 0; i < NghostP1 + NinteriorP1; ++i) {
        dofsPlan.setDof(idx++,4); // Store P1 element per i
    }
    for (int i = 0; i < NinteriorP2; ++i) {
        dofsPlan.setDof(idx++,10); // Store P2 element per i
    }
    auto dofsLayout = dofsPlan.getLayout();

    SUBCASE ("MultiStorage works") {
	using local_storage_t = MultiStorage<DataLayout::SoA, material, bc>;
	local_storage_t localC(localLayout.back());
	DenseView<local_storage_t> localView(localLayout, localC, NghostP1, N);
	for (int i = 0; i < localView.size(); ++i) {
	    localView[i].get<material>() = ElasticMaterial{1.0*i,1.0*i,2.0*i};
	}
	int i = 0;
	for (auto&& v : localView) {
	    REQUIRE(v.get<material>().lambda == 2.0*i++);
	}
    }

    SUBCASE ("SingleStorage works") {
	using dofs_storage_t = SingleStorage<dofs>;
	dofs_storage_t dofsC(dofsLayout.back());
	StridedView<dofs_storage_t,4u> dofsV(dofsLayout, dofsC, 0, NghostP1 + NinteriorP1);
	int k = 0, l;
	for (auto&& v : dofsV) {
	    l = 0;
	    for (auto&& vv : v) {
		vv = k + 4*l++;
	    }
	    ++k;
	}
	for (std::size_t j = 0; j < NghostP1 + NinteriorP1; ++j) {
	    REQUIRE(dofsC[j] == j/4 + 4*(j%4));
	}
    }

}
