#include "doctest.h"
#include <cstddef>
#include <utility>
#include <vector>

#include "mneme/displs.hpp"

using mneme::Displs;

TEST_CASE("testing displs") {
    Displs<int> displs;
    std::vector<int> count{0, 4, 0, 0, 1, 0, 2, 0};

    SUBCASE("make displs") {
        displs.make(count);

        REQUIRE(displs.size() == count.size());
        for (int p = 0; p < displs.size(); ++p) {
            CHECK(count[p] == displs.count(p));
        }
        CHECK(displs[displs.size()] == 7);
    }

    SUBCASE("empty displs iterator") {
        std::size_t numIterations = 0;
        for (auto [p, i] : displs) {
            ++numIterations;
        }
        CHECK(numIterations == 0);
    }

    SUBCASE("displs iterator") {
        displs.make(count);
        std::size_t numIterations = 0;
        std::vector<std::pair<int, int>> pairs;
        for (auto pi : displs) {
            ++numIterations;
            pairs.emplace_back(pi);
        }
        CHECK(numIterations == 7);

        auto it = pairs.begin();
        for (int p = 0; p < count.size(); ++p) {
            for (int i = displs[p]; i < displs[p + 1]; ++i) {
                CHECK(it->first == p);
                CHECK(it->second == i);
                ++it;
            }
        }
    }
}
