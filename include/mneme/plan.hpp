#ifndef MNEME_PLAN_H_
#define MNEME_PLAN_H_

#include <cstddef>
#include <memory>
#include <numeric>
#include <optional>
#include <utility>
#include <vector>

#include "displacements.hpp"
#include "span.hpp"
#include "tagged_tuple.hpp"

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
    std::size_t numElements = 0;
    std::size_t offset = 0;
};

class LayeredPlanBase {};

template <typename... Layers> class LayeredPlan : public LayeredPlanBase {
    template <typename... OtherLayers> friend class LayeredPlan;

public:
    using LayoutT = Displacements<std::size_t>;
    LayeredPlan() : plan(0){};

    LayeredPlan(std::size_t curOffset, std::size_t numElements, Plan plan,
                std::tuple<Layers...> layers)
        : curOffset(curOffset), numElements(numElements), plan(std::move(plan)), layers(layers),
          layout(std::nullopt) {}

    template <typename OtherPlanT>
    explicit LayeredPlan(OtherPlanT otherPlan) : plan(otherPlan.plan) {
        this->curOffset = otherPlan.curOffset;
        this->numElements = otherPlan.numElements;
        this->plan = otherPlan.plan;
        // Copy all entries of old tuple into new tuple.
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
    LayeredPlan<Layers..., Layer> withDofs(std::size_t numElementsLayer, Func func) const {
        auto newPlan = LayeredPlan<Layers..., Layer>(*this);
        auto& newLayer = std::get<Layer>(newPlan.layers);
        newLayer.numElements = numElementsLayer;
        newLayer.offset = curOffset;
        const auto newCurOffset = curOffset + numElementsLayer;
        const auto newNumElements = numElements + numElementsLayer;
        newPlan.curOffset = newCurOffset;
        newPlan.numElements = newNumElements;

        newPlan.plan.resize(newNumElements);

        for (std::size_t i = 0; i < newLayer.numElements; ++i) {
            newPlan.plan.setDof(i + newLayer.offset, func(i));
        }
        return newPlan;
    }

    const LayoutT& getLayout() const {
        // if (layout == std::nullopt) {
        layout = plan.getLayout();
        //}
        return *layout;
    }

    template <typename T> T getLayer() const { return std::get<T>(layers); }
    std::size_t getOffset() const { return curOffset; }
    size_t size() const { return numElements; };

private:
    std::tuple<Layers...> layers;
    std::size_t curOffset = 0;
    std::size_t numElements = 0;
    Plan plan;
    mutable std::optional<LayoutT> layout;
};

class CombinedLayeredPlanBase {};

template <typename... Layers> class CombinedLayeredPlan : public CombinedLayeredPlanBase {
public:
    using PlanT = LayeredPlan<Layers...>;
    using LayoutT = Displacements<size_t>;

    explicit CombinedLayeredPlan(std::vector<PlanT> plans) : plans(plans), offsets(plans.size()) {
        std::size_t offset = 0;
        for (std::size_t i = 0; i < plans.size(); ++i) {
            const auto& plan = plans[i];
            offsets[i] = offset;
            offset += plan.size();
        }
    }

    [[nodiscard]] LayoutT getLayout() const {
        const std::size_t totalNumberOfDofs =
            std::accumulate(plans.begin(), plans.end(), 0U,
                            [](auto count, auto& vec) { return count + vec.size(); });

        std::vector<size_t> combinedDofs(totalNumberOfDofs);
        std::size_t idx = 0;
        for (auto i = 0U; i < plans.size(); ++i) {
            const auto& plan = plans[i];
            const auto& curLayout = plan.getLayout();
            for (auto j = 0U; j < curLayout.size(); ++j) {
                combinedDofs[idx] = curLayout.count(j);
                ++idx;
            }
        }
        return {combinedDofs};
    }

    template <typename T> T getLayer(std::size_t clusterId) const {
        const auto& cluster = plans[clusterId];
        auto layer = cluster.template getLayer<T>();
        layer.offset += offsets[clusterId];
        return layer;
    }

private:
    std::vector<PlanT> plans;
    std::vector<std::size_t> offsets;
};
} // namespace mneme

#endif // MNEME_PLAN_H_
