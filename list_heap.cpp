#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <cassert>
#include <cmath>
#include <iostream>  // Требуется из-за бага в библиотеке doctest: https://github.com/onqtam/doctest/issues/356
#include <iterator>
#include <list>
#include <type_traits>
#include <utility>
#include "doctest.h"

namespace lab_03 {

template <typename T, typename ComparatorType = std::less<T>>
struct list_heap {
private:
    typename std::list<T> data;
    ComparatorType comparator;
    typename std::list<T>::iterator minimum = data.begin();

public:
    list_heap() = default;
    explicit list_heap(ComparatorType const &comp) : comparator(comp){};
    list_heap(const list_heap &) = delete;
    list_heap &operator=(const list_heap &) = delete;
    list_heap(list_heap &&) noexcept = default;
    list_heap &operator=(list_heap &&) noexcept = default;
    [[nodiscard]] bool empty() const {
        return data.empty();
    }

    [[nodiscard]] T const &top() const {
        assert(!empty());
        return *minimum;
    }

    void pop() & {
        assert(!empty());
        data.erase(minimum);
        minimum = std::min_element(data.begin(), data.end(), comparator);
    }

    void push(T const &value) & {
        data.push_back(value);
        if (data.size() == 1) {
            --minimum;
        }
        if (comparator(value, *minimum)) {
            minimum = data.end();
            --minimum;
        }
    }

    void merge(list_heap &other) & {
        if (!other.empty() &&
            (empty() || comparator(*other.minimum, *minimum))) {
            minimum = other.minimum;
        }
        data.splice(data.end(), other.data);
        other.minimum = other.data.begin();
    }
    ~list_heap() = default;
};

namespace tests {
TEST_CASE("push, top, pop") {
    list_heap<int> heap;
    const auto &ch = heap;

    CHECK(heap.empty());

    heap.push(10);  // Push to empty.
    // {10}
    CHECK(!ch.empty());
    CHECK(ch.top() == 10);

    heap.push(5);  // Push and update minimum.
    // {10, 5}
    CHECK(!ch.empty());
    CHECK(ch.top() == 5);

    heap.push(15);  // Push and do not update minimum.
    // {10, 5, 15}
    CHECK(!ch.empty());
    CHECK(ch.top() == 5);

    heap.pop();  // Pop in the middle, correct minimum is beforehand.
    // {10, 15}
    CHECK(!ch.empty());
    CHECK(ch.top() == 10);

    heap.push(2);
    // {10, 15, 2}
    CHECK(!ch.empty());
    CHECK(ch.top() == 2);

    heap.pop();  // Pop the last element.
    // {10, 15}
    CHECK(!ch.empty());
    CHECK(ch.top() == 10);

    heap.pop();  // Correct minimal element is afterwards.
    // {15}
    CHECK(!ch.empty());
    CHECK(ch.top() == 15);

    heap.pop();
    CHECK(ch.empty());
}

TEST_CASE("merge empty with empty") {
    list_heap<int> heap1;
    list_heap<int> heap2;

    heap1.merge(heap2);

    CHECK(heap2.empty());
    CHECK(heap1.empty());

    heap1.push(100);
    CHECK(!heap1.empty());
    CHECK(heap1.top() == 100);
    CHECK(heap2.empty());

    // Ensure heap2 does not remember its old content.
    heap2.push(105);
    CHECK(!heap1.empty());
    CHECK(heap1.top() == 100);
    CHECK(!heap2.empty());
    CHECK(heap2.top() == 105);
}

TEST_CASE("merge empty with non-empty") {
    list_heap<int> heap1;
    list_heap<int> heap2;
    heap2.push(5);
    heap2.push(15);

    heap1.merge(heap2);

    CHECK(heap2.empty());
    CHECK(!heap1.empty());
    CHECK(heap1.top() == 5);

    // Ensure heap2 does not remember its old content.
    heap2.push(100);
    CHECK(!heap2.empty());
    CHECK(heap2.top() == 100);

    heap1.pop();
    CHECK(!heap1.empty());
    CHECK(heap1.top() == 15);

    heap1.pop();
    CHECK(heap1.empty());
}

TEST_CASE("merge non-empty and empty") {
    list_heap<int> heap1;
    list_heap<int> heap2;

    SUBCASE("left is non-empty") {
        heap1.push(5);
        heap1.push(15);
    }
    SUBCASE("right is non-empty") {
        heap2.push(5);
        heap2.push(15);
    }

    heap1.merge(heap2);

    CHECK(heap2.empty());
    CHECK(!heap1.empty());
    CHECK(heap1.top() == 5);

    // Ensure heap2 is not linked with heap1 accidentally.
    heap2.push(100);
    CHECK(!heap2.empty());
    CHECK(heap2.top() == 100);

    heap1.pop();
    CHECK(!heap1.empty());
    CHECK(heap1.top() == 15);

    heap1.pop();
    CHECK(heap1.empty());
}

TEST_CASE("merge non-empty and non-empty") {
    list_heap<int> heap1;
    list_heap<int> heap2;

    SUBCASE("left minimum is smaller") {
        heap1.push(5);
        heap2.push(10);
        heap1.push(15);
        heap2.push(20);
    }
    SUBCASE("right minimum is smaller") {
        heap2.push(5);
        heap1.push(10);
        heap2.push(15);
        heap1.push(20);
    }

    heap1.merge(heap2);
    CHECK(heap2.empty());
    CHECK(!heap1.empty());
    CHECK(heap1.top() == 5);

    // Ensure heap2 is not linked with heap1 accidentally.
    heap2.push(100);
    CHECK(!heap2.empty());
    CHECK(heap2.top() == 100);

    heap1.pop();
    CHECK(!heap1.empty());
    CHECK(heap1.top() == 10);

    heap1.pop();
    CHECK(!heap1.empty());
    CHECK(heap1.top() == 15);

    heap1.pop();
    CHECK(!heap1.empty());
    CHECK(heap1.top() == 20);

    heap1.pop();
    CHECK(heap1.empty());
}

TEST_CASE("top() returns const reference even for non-const heap") {
    list_heap<int> heap;
    CHECK(std::is_same_v<const int &, decltype(heap.top())>);
}

TEST_CASE("modifying operations return nothing") {
    list_heap<int> heap;
    CHECK(std::is_void_v<decltype(heap.push(10))>);
    CHECK(std::is_void_v<decltype(heap.pop())>);
    CHECK(std::is_void_v<decltype(heap.merge(heap))>);
}

TEST_CASE("with std::greater") {
    list_heap<int, std::greater<>>
        heap;  // std::greater<> is a "transparent functor".
    const auto &ch = heap;

    heap.push(5);
    CHECK(!ch.empty());
    CHECK(ch.top() == 5);

    heap.push(15);
    CHECK(!ch.empty());
    CHECK(ch.top() == 15);

    heap.push(10);
    CHECK(!ch.empty());
    CHECK(ch.top() == 15);

    heap.pop();
    CHECK(!ch.empty());
    CHECK(ch.top() == 10);

    heap.pop();
    CHECK(!ch.empty());
    CHECK(ch.top() == 5);

    heap.pop();
    CHECK(ch.empty());
}

TEST_CASE("non-DefaultConstructible and non-CopyAssignable comparator") {
    struct CloserTo {
    private:
        int center_;

    public:
        explicit CloserTo(int center) : center_(center) {
        }
        CloserTo(const CloserTo &) = default;
        CloserTo(CloserTo &&) = default;
        CloserTo &operator=(const CloserTo &) = delete;
        CloserTo &operator=(CloserTo &&) = delete;
        ~CloserTo() = default;
        bool operator()(int a, int b) const {
            return std::abs(a - center_) < std::abs(b - center_);
        }
    };
    list_heap<int, CloserTo> heap(CloserTo(10));
    list_heap<int, CloserTo> heap_other(CloserTo(10));
    const auto &ch = heap;

    heap.push(0);
    heap.push(15);
    heap.push(100);

    heap_other.push(0);
    heap_other.push(15);
    heap_other.push(100);

    CHECK(!ch.empty());
    CHECK(ch.top() == 15);

    heap.pop();
    CHECK(!ch.empty());
    CHECK(ch.top() == 0);

    heap.pop();
    CHECK(!ch.empty());
    CHECK(ch.top() == 100);

    heap.pop();
    CHECK(ch.empty());

    heap.merge(heap_other);
    CHECK(!heap.empty());
    CHECK(heap.top() == 15);
    CHECK(heap_other.empty());
}

TEST_CASE("compiles with no operator< and lambda") {
    struct Wrapper {
        int value_;  // NOLINT(misc-non-private-member-variables-in-classes)
        explicit Wrapper(int value) : value_(value) {
        }
    };
    auto compare = [](const Wrapper &a, const Wrapper &b) {
        return a.value_ < b.value_;
    };

    list_heap<Wrapper, decltype(compare)> heap(compare);
    const auto &ch = heap;

    heap.push(Wrapper(5));
    heap.push(Wrapper(15));
    CHECK(!ch.empty());
    CHECK(ch.top().value_ == 5);

    heap.pop();

    list_heap<Wrapper, decltype(compare)> heap2(compare);
    heap.merge(heap2);
}

TEST_SUITE("provides first minimum") {
    auto compare = [](const std::pair<int, int> &a,
                      const std::pair<int, int> &b) {
        return a.first > b.first;
    };

    TEST_CASE("single heap") {
        list_heap<std::pair<int, int>, decltype(compare)> heap(compare);
        heap.push(std::pair(15, 1));
        heap.push(std::pair(5, 2));
        heap.push(std::pair(15, 3));
        heap.push(std::pair(5, 4));

        CHECK(!heap.empty());
        CHECK(heap.top() == std::pair(15, 1));

        heap.pop();
        CHECK(!heap.empty());
        CHECK(heap.top() == std::pair(15, 3));

        heap.pop();
        CHECK(!heap.empty());
        CHECK(heap.top() == std::pair(5, 2));

        heap.pop();
        CHECK(!heap.empty());
        CHECK(heap.top() == std::pair(5, 4));
    }

    TEST_CASE("merging heaps") {
        list_heap<std::pair<int, int>, decltype(compare)> heap1(compare);
        list_heap<std::pair<int, int>, decltype(compare)> heap2(compare);
        heap1.push(std::pair(5, 1));
        heap2.push(std::pair(5, 2));

        heap1.merge(heap2);
        CHECK(heap2.empty());
        CHECK(!heap1.empty());
        CHECK(heap1.top() == std::pair(5, 1));

        heap1.pop();
        CHECK(!heap1.empty());
        CHECK(heap1.top() == std::pair(5, 2));

        heap1.pop();
        CHECK(heap1.empty());
    }
}

TEST_CASE("[no-valgrind] lots of empty+push+top" * doctest::timeout(20)) {
    const int STEPS = 1'000'000;
    list_heap<int> heap;

    for (int i = 0; i < STEPS; i++) {
        heap.push(STEPS - i);
        CHECK(!heap.empty());
        CHECK(heap.top() == STEPS - i);
    }
}

TEST_CASE("[no-valgrind] lots of merge" * doctest::timeout(20)) {
    const int SIZE = 1'000'000;
    const int STEPS = 1'000'000;
    list_heap<int> heap1;
    list_heap<int> heap2;

    for (int i = 0; i < SIZE; i++) {
        heap1.push(i);
    }

    for (int i = 0; i < STEPS; i++) {
        heap2.merge(heap1);
        CHECK(heap1.empty());
        CHECK(!heap2.empty());

        heap1.merge(heap2);
        CHECK(!heap1.empty());
        CHECK(heap2.empty());
    }
}
}  // namespace tests
}  // namespace lab_03

TEST_CASE("in correct namespace") {
    [[maybe_unused]] lab_03::list_heap<int> heap;
}
