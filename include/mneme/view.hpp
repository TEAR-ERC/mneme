#ifndef MNEME_VIEW_H_
#define MNEME_VIEW_H_

#include "iterator.hpp"
#include "span.hpp"

#include <cstddef>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <vector>

namespace mneme {

template <typename Storage, std::size_t Stride = dynamic_extent> class StridedView {
public:
    using offset_type = typename Storage::offset_type;
    using iterator = Iterator<StridedView<Storage, Stride>>;

    StridedView() : size_(0), container_(nullptr) {}

    template <class Layout>
    StridedView(Layout const& layout, std::shared_ptr<Storage> container, std::size_t from,
                std::size_t to) {
        setStorage(layout, std::move(container), from, to);
    }

    template <class Layout>
    void setStorage(Layout const& layout, std::shared_ptr<Storage> container, std::size_t from,
                    std::size_t to) {
        size_ = to - from;
        container_ = std::move(container);
        if (!container_) {
            return;
        }
        offset = container_->offset(layout[from]);
        if (!(to > from)) {
            throw std::runtime_error("'To' must be larger than 'from'.");
        }
        if constexpr (Stride == dynamic_extent) {
            stride = layout[from + 1] - layout[from];
        } else {
            stride = Stride;
        }
        for (std::size_t i = from; i < to; ++i) {
            auto stridei = layout[i + 1] - layout[i];
            if (stridei != stride) {
                std::stringstream ss;
                ss << "Failed to construct strided view: Stride " << stridei << " != " << stride
                   << " at " << from << ".";
                throw std::runtime_error(ss.str());
            }
        }
    }

    void setStorage(std::shared_ptr<Storage> container, std::size_t from, std::size_t to,
                    std::size_t strd = 1u) {
        size_ = to - from;
        container_ = std::move(container);
        if (!container_) {
            return;
        }
        if constexpr (Stride == dynamic_extent) {
            stride = strd;
        } else {
            stride = Stride;
        }
        assert(container_->size() % stride == 0);

        offset = container_->offset(from * stride);
    }

    auto operator[](std::size_t localId) noexcept -> typename Storage::template value_type<Stride> {
        assert(container_ != nullptr);
        std::size_t s;
        if constexpr (Stride == dynamic_extent) {
            s = stride;
        } else {
            s = Stride;
        }
        std::size_t from = localId * s;
        return container_->template get<Stride>(offset, from, from + s);
    }

    std::size_t size() const noexcept { return size_; }
    Storage const& storage() const noexcept { return *container_; }

    iterator begin() { return iterator(this, 0); }
    iterator end() { return iterator(this, size()); }

private:
    std::size_t size_ = 0, stride;
    std::shared_ptr<Storage> container_;
    offset_type offset;
};

template <typename Storage> using DenseView = StridedView<Storage, 1u>;

template <typename Storage> class GeneralView {
public:
    using offset_type = typename Storage::offset_type;
    using iterator = Iterator<GeneralView<Storage>>;

    GeneralView() : size_(0), container_(nullptr) {}

    template <class Layout>
    GeneralView(Layout const& layout, std::shared_ptr<Storage> container, std::size_t from,
                std::size_t to) {
        setStorage(layout, std::move(container), from, to);
    }

    template <class Layout>
    void setStorage(Layout const& layout, std::shared_ptr<Storage> container, std::size_t from,
                    std::size_t to) {
        size_ = to - from;
        sl.clear();
        sl.insert(sl.begin(), layout.begin() + from, layout.begin() + to);
        container_ = std::move(container);
        if (container_ == nullptr) {
            return;
        }
        offset = container_->offset(layout[from]);
        if (!(to > from)) {
            throw std::runtime_error("'To' must be larger than 'from'.");
        }
    }

    auto operator[](std::size_t localId) noexcept ->
        typename Storage::template value_type<dynamic_extent> {
        assert(container_ != nullptr);
        return container_->get<dynamic_extent>(offset, sl[localId], sl[localId + 1]);
    }

    std::size_t size() const noexcept { return size_; }
    Storage const& storage() const noexcept { return *container_; }

    iterator begin() { return iterator(this, 0); }
    iterator end() { return iterator(this, size()); }

private:
    std::size_t size_ = 0;
    std::vector<std::size_t> sl;
    std::shared_ptr<Storage> container_;
    offset_type offset;
};

} // namespace mneme

#endif // MNEME_VIEW_H_
