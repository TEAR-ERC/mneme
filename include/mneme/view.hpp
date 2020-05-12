#ifndef MNEME_VIEW_H_
#define MNEME_VIEW_H_

#include <cstddef>
#include <stdexcept>
#include <sstream>
#include <vector>

#include "iterator.hpp"

namespace mneme {

template<typename Storage, std::size_t Stride = dynamic_extent>
class StridedView {
public:
    using offset_type = typename Storage::offset_type;
	using iterator = Iterator<StridedView<Storage, Stride>>;

    template<class Layout>
    StridedView(Layout const& layout, Storage& container, std::size_t from, std::size_t to)
        : size_(to-from), container(container),
          offset(container.offset(layout[from])) {
        if (!(to > from)) {
            throw std::runtime_error("'To' must be larger than 'from'.");
        }
        if constexpr (Stride == dynamic_extent) {
            stride = layout[from+1]-layout[from];
        } else {
            stride = Stride;
        }
        for (std::size_t i = from; i < to; ++i) {
            auto stridei = layout[i+1]-layout[i];
            if (stridei != stride) {
                std::stringstream ss;
                ss << "Failed to construct strided view: Stride " << stridei << " != " << stride << " at " << from << ".";
                throw std::runtime_error(ss.str());
            }
        }
    }

    auto operator[](std::size_t localId) noexcept {
        std::size_t s;
        if constexpr (Stride == dynamic_extent) {
            s = stride;
        } else {
            s = Stride;
        }
        std::size_t from = localId*s;
        return container.template get<Stride>(offset, from, from + s);
    }

    std::size_t size() const noexcept { return size_; }

    iterator begin() { return iterator(this, 0); }
    iterator end() { return iterator(this, size()); }

private:
    std::size_t size_, stride;
    Storage& container;
    offset_type offset;
};

template<typename Storage>
using DenseView = StridedView<Storage,1u>;

template<typename Storage>
class GeneralView {
public:
    using offset_type = typename Storage::offset_type;
	using iterator = Iterator<GeneralView<Storage>>;

    template<class Layout>
    GeneralView(Layout const& layout, Storage& container, std::size_t from, std::size_t to)
        : size_(to-from),
          sl(layout.begin()+from,layout.begin()+to),
          container(container),
          offset(container.offset(layout[from])) {
        if (!(to > from)) {
            throw std::runtime_error("'To' must be larger than 'from'.");
        }
    }

    auto operator[](std::size_t localId) noexcept {
        return container.get(offset, sl[localId], sl[localId+1]);
    }

    std::size_t size() const noexcept { return size_; }

    iterator begin() { return iterator(this, 0); }
    iterator end() { return iterator(this, size()); }

private:
    std::size_t size_;
    std::vector<std::size_t> sl;
    Storage& container;
    offset_type offset;
};

}

#endif // MNEME_VIEW_H_
