#ifndef MNEME_PLAN_H_
#define MNEME_PLAN_H_

#include <cstddef>
#include <vector>

#include "displacements.hpp"

namespace mneme {

class Plan {
public:
    Plan(std::size_t numElements) : dofs(numElements, 0) {}

    void setDof(std::size_t elementNo, std::size_t dof) { dofs[elementNo] = dof; }

    Displacements<std::size_t> getLayout() { return Displacements(dofs); }

private:
    std::vector<std::size_t> dofs;
};

} // namespace mneme

#endif // MNEME_PLAN_H_
