#ifndef MNEME_PLAN_H_
#define MNEME_PLAN_H_

#include <cstddef>
#include <vector>

#include "displacements.hpp"
#include "tagged_tuple.hpp"

namespace mneme {

class Plan {
public:
    explicit Plan(std::size_t numElements) : dofs(numElements, 0) {}

    void setDof(std::size_t elementNo, std::size_t dof) { dofs[elementNo] = dof; }

    void resize(std::size_t newSize) { dofs.resize(newSize); }
    [[nodiscard]] Displacements<std::size_t> getLayout() const { return Displacements(dofs); }

private:
    std::vector<std::size_t> dofs;
};

class Layer {
public:
    Layer() = default;
    Layer(std::size_t numElements, std::size_t offset) : numElements(numElements), offset(offset) {}
    size_t numElements;
    size_t offset = -1;
};

template <typename... Layers> class LayeredPlan {
public:
    constexpr LayeredPlan() : plan(0){};

    template <typename Layer, typename Func> auto setDofs(size_t numElements, Func func) {
        auto newPlan = LayeredPlan<Layers..., Layer>();
        // TODO(Lukas) Copy constructor
        newPlan.curOffset = curOffset;
        newPlan.numElements = this->numElements;
        newPlan.plan = plan;
        std::apply(
            [&](auto&&... args) {
                (((std::get<typename std::remove_reference<decltype(args)>::type>(newPlan.layers)) =
                      std::get<typename std::remove_reference<decltype(args)>::type>(layers)),
                 ...);
                ((std::cout << args.offset << '\n'), ...);
            },
            layers);

        auto& layer = std::get<Layer>(newPlan.layers);
        layer.numElements = numElements;
        layer.offset = newPlan.curOffset;
        newPlan.curOffset += numElements;
        newPlan.numElements += numElements;

        newPlan.plan.resize(newPlan.numElements);
        for (size_t i = 0; i < layer.numElements; ++i) {
            newPlan.plan.setDof(i + layer.offset, func(i));
        }
        return newPlan;
    }

    auto getLayout() const { return plan.getLayout(); }

public:
    // TODO(Lukas) Private
    std::tuple<Layers...> layers;
    size_t curOffset = 0;
    size_t numElements = 0;
    Plan plan;
};

} // namespace mneme

#endif // MNEME_PLAN_H_
