#ifndef MNEME_PLAN_H_
#define MNEME_PLAN_H_

#include <cstddef>
#include <vector>

#include "displacements.hpp"
#include "span.hpp"
#include "tagged_tuple.hpp"
#include "view.hpp"

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
    constexpr Layer() = default;
    constexpr Layer(std::size_t numElements, std::size_t offset)
        : numElements(numElements), offset(offset) {}
    size_t numElements = 0;
    size_t offset = 0;
};

template <typename... Layers> class LayeredPlan {
public:
    LayeredPlan() : plan(0){};

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

    template <typename Layer, typename Func> auto setDofs(size_t numElementsLayer, Func func) {
        auto newPlan = LayeredPlan<Layers..., Layer>(*this);

        auto& layer = std::get<Layer>(newPlan.layers);
        layer.numElements = numElementsLayer;
        layer.offset = newPlan.curOffset;
        newPlan.curOffset += numElementsLayer;
        newPlan.numElements += numElementsLayer;

        newPlan.plan.resize(newPlan.numElements);
        for (size_t i = 0; i < layer.numElements; ++i) {
            newPlan.plan.setDof(i + layer.offset, func(i));
        }
        return newPlan;
    }

    auto getLayout() const { return plan.getLayout(); }

    template <typename T> auto getLayer() const { return std::get<T>(layers); }

public:
    // TODO(Lukas) Private
    std::tuple<Layers...> layers;
    size_t curOffset = 0;
    size_t numElements = 0;
    Plan plan;
};

struct StaticNothing {};

template <typename T> struct StaticSome {
    constexpr explicit StaticSome(T value) : value(value) {}
    using type = T;
    T value;
};

template <typename MaybeStride = StaticNothing, typename MaybePlan = StaticNothing,
          typename MaybeStorage = StaticNothing>
class LayeredViewFactory {

public:
    MaybePlan maybePlan;
    MaybeStorage maybeStorage;

    constexpr LayeredViewFactory() = default;

    constexpr LayeredViewFactory(MaybePlan maybePlan, MaybeStorage maybeStorage)
        : maybePlan(maybePlan), maybeStorage(maybeStorage) {}

    template <std::size_t Stride> constexpr auto withStride() {
        return LayeredViewFactory<std::integral_constant<size_t, Stride>, MaybePlan, MaybeStorage>(
            maybePlan, maybeStorage);
    }

    constexpr auto withDynamicStride() { return withStride<dynamic_extent>(); }

    template <typename LayeredPlanT> auto withPlan(LayeredPlanT& plan) {
        const auto somePlan = StaticSome<LayeredPlanT>(plan);
        return LayeredViewFactory<MaybeStride, StaticSome<LayeredPlanT>, MaybeStorage>(
            somePlan, maybeStorage);
    }

    template <typename StorageT> constexpr auto withStorage(StorageT& storage) {
        const auto someStorage = StaticSome<StorageT*>(&storage);
        return LayeredViewFactory<MaybeStride, MaybePlan, StaticSome<StorageT*>>(maybePlan,
                                                                                 someStorage);
    }

    template <
        typename Layer, typename MaybeStride_ = MaybeStride, typename MaybePlan_ = MaybePlan,
        typename MaybeStorage_ = MaybeStorage,
        typename std::enable_if<!std::is_same<MaybeStride_, StaticNothing>::value, int>::type = 0,
        typename std::enable_if<!std::is_same<MaybePlan_, StaticNothing>::value, int>::type = 0,
        typename std::enable_if<!std::is_same<MaybeStorage_, StaticNothing>::value, int>::type = 0>
    constexpr auto createStridedView() {
        auto layout = maybePlan.value.getLayout();
        const auto& layer = maybePlan.value.template getLayer<Layer>();
        size_t from = layer.offset;
        size_t to = from + layer.numElements;
        return StridedView<std::remove_pointer_t<typename MaybeStorage_::type>,
                MaybeStride::value>(layout, *(maybeStorage.value), from, to);
    }

    template <
        typename Layer, typename MaybeStride_ = MaybeStride, typename MaybePlan_ = MaybePlan,
        typename MaybeStorage_ = MaybeStorage,
        typename std::enable_if<std::is_same<MaybeStride_, StaticNothing>::value, int>::type = 0,
        typename std::enable_if<!std::is_same<MaybePlan_, StaticNothing>::value, int>::type = 0,
        typename std::enable_if<!std::is_same<MaybeStorage_, StaticNothing>::value, int>::type = 0>
    constexpr auto createDenseView() {
        auto layout = maybePlan.value.getLayout();
        const auto& layer = maybePlan.value.template getLayer<Layer>();
        size_t from = layer.offset;
        size_t to = from + layer.numElements;
        return DenseView<std::remove_pointer_t<typename MaybeStorage_::type>>(layout, *(maybeStorage.value), from, to);
    }
};

constexpr auto createView() {
    return LayeredViewFactory<StaticNothing, StaticNothing, StaticNothing>();
}

} // namespace mneme

#endif // MNEME_PLAN_H_
