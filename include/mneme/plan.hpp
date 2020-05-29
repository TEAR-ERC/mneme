#ifndef MNEME_PLAN_H_
#define MNEME_PLAN_H_

#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

#include "displacements.hpp"
#include "span.hpp"
#include "tagged_tuple.hpp"
#include "view.hpp"

namespace mneme {

class Plan {
public:
    using LayoutType = Displacements<std::size_t>;
    explicit Plan(std::size_t numElements) : dofs(numElements, 0) {}

    void setDof(std::size_t elementNo, std::size_t dof) { dofs[elementNo] = dof; }

    void resize(std::size_t newSize) { dofs.resize(newSize); }
    [[nodiscard]] LayoutType getLayout() const { return Displacements(dofs); }

private:
    std::vector<std::size_t> dofs;
};

class Layer {
public:
    constexpr Layer() = default;
    constexpr Layer(std::size_t numElements, std::size_t offset)
        : numElements(numElements), offset(offset) {}
    size_t numElements = 0;
    size_t offset = 0;
};

template <typename... Layers> class LayeredPlan {
    template <typename... OtherLayers> friend class LayeredPlan;

public:
    using LayoutT = Displacements<size_t>;
    LayeredPlan() : plan(0){};

    LayeredPlan(size_t curOffset, size_t numElements, Plan plan, std::tuple<Layers...> layers)
        : curOffset(curOffset), numElements(numElements), plan(std::move(plan)), layers(layers),
          layout(std::nullopt) {}

    template <typename OtherPlanT>
    explicit LayeredPlan(OtherPlanT otherPlan) : plan(otherPlan.plan) {
        this->curOffset = otherPlan.curOffset;
        this->numElements = otherPlan.numElements;
        this->plan = otherPlan.plan;
        std::apply(
            [&](auto&&... args) {
                (((std::get<typename std::remove_reference_t<std::remove_const_t<decltype(args)>>>(
                      layers)) =
                      std::get<typename std::remove_reference_t<decltype(args)>>(otherPlan.layers)),
                 ...);
            },
            otherPlan.layers);
    }

    template <typename Layer, typename Func>
    LayeredPlan<Layers..., Layer> withDofs(size_t numElementsLayer, Func func) const {
        auto newPlan = LayeredPlan<Layers..., Layer>(*this);
        auto& newLayer = std::get<Layer>(newPlan.layers);
        newLayer.numElements = numElementsLayer;
        newLayer.offset = curOffset;
        const auto newCurOffset = curOffset + numElementsLayer;
        const auto newNumElements = numElements + numElementsLayer;
        newPlan.curOffset = newCurOffset;
        newPlan.numElements = newNumElements;

        newPlan.plan.resize(newNumElements);

        for (size_t i = 0; i < newLayer.numElements; ++i) {
            newPlan.plan.setDof(i + newLayer.offset, func(i));
        }
        return newPlan;
    }

    const LayoutT& getLayout() const {
        if (layout == std::nullopt) {
            layout = plan.getLayout();
        }
        return *layout;
    }

    template <typename T> T getLayer() const { return std::get<T>(layers); }

private:
    std::tuple<Layers...> layers;
    size_t curOffset = 0;
    size_t numElements = 0;
    Plan plan;
    mutable std::optional<LayoutT> layout;
};

} // namespace mneme

#endif // MNEME_PLAN_H_
