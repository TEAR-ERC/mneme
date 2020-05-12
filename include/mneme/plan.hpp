#ifndef MNEME_PLAN_H_
#define MNEME_PLAN_H_

#include <cstddef>
#include <vector>

namespace mneme {

using GeneralLayout = std::vector<std::size_t>;

class Plan {
public:
    Plan(std::size_t numElements)
        : dofs(numElements, 0) {}

    void setDof(std::size_t elementNo, std::size_t dof) {
        dofs[elementNo] = dof;
    }

    GeneralLayout getLayout() {
        GeneralLayout layout(dofs.size()+1);
        layout[0] = 0;
        for (std::size_t i = 0; i < dofs.size(); ++i) {
            layout[i+1] = layout[i] + dofs[i];
        }
        return layout;
    }

private:
    std::vector<std::size_t> dofs;
};

}

#endif // MNEME_PLAN_H_

