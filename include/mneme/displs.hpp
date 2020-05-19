#ifndef MNEME_DISPLS_H
#define MNEME_DISPLS_H

#include <cassert>
#include <cstddef>
#include <iterator>
#include <utility>
#include <vector>

namespace mneme {

template <typename IntT> class DisplsIterator {
public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = std::pair<std::size_t, std::size_t>;
    using pointer = value_type*;
    using reference = value_type&;

    DisplsIterator(std::vector<IntT> const& displs, bool end = false) : displs(displs), p(0), i(0) {
        if (end) {
            p = displs.size() - 1;
            i = displs[p];
        } else {
            i = displs[0];
            next();
        }
    }

    bool operator!=(DisplsIterator const& other) {
        return i != other.i || p != other.p || &displs != &other.displs;
    }
    bool operator==(DisplsIterator const& other) { return !(*this != other); }

    DisplsIterator& operator++() {
        assert(i >= displs[p] && i < displs[p + 1]);
        ++i;
        next();
        return *this;
    }
    DisplsIterator operator++(int) {
        DisplsIterator copy(*this);
        ++(*this);
        return copy;
    }

    value_type operator*() { return std::make_pair(p, i); }

private:
    void next() {
        while (i >= displs[p + 1] && p < displs.size() - 1) {
            ++p;
            i = displs[p];
        }
    }

    std::vector<IntT> const& displs;
    std::size_t p, i;
};

/**
 * @brief General data layout class
 *
 * Assume you have ids i=0,...,N-1 and with each id you want to associate n_i items.
 * The items shall be stored contiguously in array A.
 *
 * So if
 * Displs<int> displs({n_0,...,n_{N-1}});
 * then
 * A[displs[i]] is the first item of of id i and A[displs[i+1]-1] is the last.
 *
 * Note also that displs[N] is defined as the total number of items, such that you may also recover
 * n_i = displs[i+1] - displs[i];
 *
 * Moreover, an iterator may be used for the pattern
 * for (int i = 0; i < N; ++i) {
 *     for (int j = displs[i]; j < displs[i+1]; ++j) {
 *         ...
 *     }
 * }
 * i.e.
 * for (auto [i,j] : displs) {
 *     ...
 * }
 *
 * This class can also be used in conjunction with MPI_Alltoallv.
 *
 * @tparam IntT integer type
 */
template <typename IntT> class Displs {
public:
    Displs() : displs(1, 0) {}

    Displs(std::vector<IntT> const& count) { make(count); }

    void make(std::vector<IntT> const& count) {
        displs.resize(count.size() + 1);
        displs[0] = 0;
        for (IntT p = 0; p < count.size(); ++p) {
            displs[p + 1] = displs[p] + count[p];
        }
    }

    std::size_t size() const { return displs.size() - 1; }
    DisplsIterator<IntT> begin() const { return DisplsIterator(displs, false); }
    DisplsIterator<IntT> end() const { return DisplsIterator(displs, true); }

    IntT operator[](std::size_t p) const { return displs[p]; }
    IntT count(std::size_t p) const { return displs[p + 1] - displs[p]; }

    IntT* data() { return displs.data(); }
    IntT const* data() const { return displs.data(); }

    void swap(Displs& other) { displs.swap(other.displs); }

    IntT back() const { return displs.back(); }

private:
    std::vector<IntT> displs;
};

} // namespace mneme

#endif // MNEME_DISPLS_H
