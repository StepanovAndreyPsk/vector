#include "vector.h"
#include <cstddef>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>
#include "doctest.h"

#ifdef TEST_STD_VECTOR
using std::vector;
#else
using lab_07::vector;
#endif

TEST_CASE(
    "Default-initialize "
#ifdef TEST_STD_VECTOR
    "std::vector<std::string>"
#else
    "lab_07::vector<std::string>"
#endif
) {
    vector<std::string> x;
    CHECK(x.empty());
    CHECK(x.size() == 0);
    CHECK(x.capacity() == 0);
    CHECK_THROWS_AS(x.at(0), std::out_of_range);
    CHECK_THROWS_AS(x.at(1), std::out_of_range);
}

TEST_CASE("Default-copy-initialize") {
    vector<std::string> x = {};
    CHECK(x.empty());
    CHECK(x.size() == 0);
    CHECK(x.capacity() == 0);
    CHECK_THROWS_AS(x.at(0), std::out_of_range);
    CHECK_THROWS_AS(x.at(1), std::out_of_range);
}

TEST_CASE("Constructor from size_t is explicit") {
    CHECK(std::is_constructible_v<vector<std::string>, std::size_t>);
    CHECK(!std::is_convertible_v<std::size_t, vector<std::string>>);
}

TEST_CASE("Constructor from (size_t, T) is implicit (since C++11)") {
    // For compatibility with std::vector<>.
    vector<std::string> vec = {5, std::string("hi")};
    CHECK(vec.size() == 5);
}

struct MinimalObj {
    int id;  // NOLINT(misc-non-private-member-variables-in-classes)

    // Draft check for leaks, double-frees and non-inits.
    // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
    std::string data = std::string(500U, 'x');

    explicit MinimalObj(int id_) : id(id_) {
        CHECK(data.size() == 500U);
    }
    MinimalObj(MinimalObj &&) = default;
    MinimalObj &operator=(MinimalObj &&) = default;

    MinimalObj(const MinimalObj &) = delete;
    MinimalObj &operator=(const MinimalObj &) = delete;

    ~MinimalObj() = default;
};

struct ObjWithDefaultCtor : MinimalObj {
    using MinimalObj::MinimalObj;

    explicit ObjWithDefaultCtor() : MinimalObj(100) {
    }
};
static_assert(std::is_default_constructible_v<ObjWithDefaultCtor>);

struct ObjWithCopyCtor : MinimalObj {
    using MinimalObj::MinimalObj;

    ObjWithCopyCtor(const ObjWithCopyCtor &other) : MinimalObj(other.id) {
    }
    ObjWithCopyCtor &operator=(const ObjWithCopyCtor &) = delete;

    ObjWithCopyCtor(ObjWithCopyCtor &&) = default;
    ObjWithCopyCtor &operator=(ObjWithCopyCtor &&) = default;

    ~ObjWithCopyCtor() = default;
};

struct ObjWithCopyAssignment : MinimalObj {
    using MinimalObj::MinimalObj;

    ObjWithCopyAssignment(const ObjWithCopyAssignment &other)
        : MinimalObj(other.id) {
    }
    ObjWithCopyAssignment &operator=(const ObjWithCopyAssignment &other) {
        id = other.id;
        return *this;
    }

    ObjWithCopyAssignment(ObjWithCopyAssignment &&) = default;
    ObjWithCopyAssignment &operator=(ObjWithCopyAssignment &&) = default;

    ~ObjWithCopyAssignment() = default;
};

TEST_CASE("Construct empty") {
    SUBCASE("explicit") {
        vector<MinimalObj> v;

        CHECK(v.empty());
    }

    SUBCASE("implicit") {
        vector<MinimalObj> v = {};

        CHECK(v.empty());
    }
}

TEST_CASE("Construct zero elements") {
    vector<ObjWithDefaultCtor> v(0);
    CHECK(v.empty());
    CHECK(v.size() == 0);
#ifndef TEST_STD_VECTOR
    CHECK(v.capacity() == 0);
#endif
}

TEST_CASE("Construct n elements and read") {
    vector<ObjWithDefaultCtor> v(5);

    CHECK(!v.empty());
    REQUIRE(v.size() == 5);
#ifndef TEST_STD_VECTOR
    CHECK(v.capacity() == 8);
#endif

    CHECK(v[0].id == 100);
    CHECK(v[1].id == 100);
    CHECK(v[2].id == 100);
    CHECK(v[3].id == 100);
    CHECK(v[4].id == 100);
}

TEST_CASE("Construct n copies elements and read") {
    const ObjWithCopyCtor obj(10);
    vector<ObjWithCopyCtor> v(5, obj);

    CHECK(!v.empty());
    REQUIRE(v.size() == 5);
#ifndef TEST_STD_VECTOR
    CHECK(v.capacity() == 8);
#endif

    CHECK(v[0].id == 10);
    CHECK(v[1].id == 10);
    CHECK(v[2].id == 10);
    CHECK(v[3].id == 10);
    CHECK(v[4].id == 10);
}

TEST_CASE("push_back moves") {
    vector<MinimalObj> v;
    v.push_back(MinimalObj(10));
    v.push_back(MinimalObj(11));
    v.push_back(MinimalObj(12));
    v.push_back(MinimalObj(13));
    v.push_back(MinimalObj(14));

    CHECK(!v.empty());
    REQUIRE(v.size() == 5);
#ifndef TEST_STD_VECTOR
    CHECK(v.capacity() == 8);
#endif
    CHECK(v[0].id == 10);
    CHECK(v[1].id == 11);
    CHECK(v[2].id == 12);
    CHECK(v[3].id == 13);
    CHECK(v[4].id == 14);
}

TEST_CASE("push_back copies") {
    vector<ObjWithCopyCtor> v;
    const ObjWithCopyCtor obj(10);
    v.push_back(obj);
    v.push_back(obj);
    v.push_back(obj);

    CHECK(!v.empty());
    REQUIRE(v.size() == 3);
#ifndef TEST_STD_VECTOR
    CHECK(v.capacity() == 4);
#endif
    CHECK(v[0].id == 10);
    CHECK(v[1].id == 10);
    CHECK(v[2].id == 10);
}

TEST_CASE("Copy-construct") {
    vector<ObjWithCopyCtor> orig;
    orig.push_back(ObjWithCopyCtor(10));
    orig.push_back(ObjWithCopyCtor(11));
    orig.push_back(ObjWithCopyCtor(12));
    orig.push_back(ObjWithCopyCtor(13));
    orig.push_back(ObjWithCopyCtor(14));
    // To check whether copy chooses minimal possible capacity.
    orig.pop_back();
    orig.pop_back();

    auto check_vec = [](const vector<ObjWithCopyCtor> &v) {
        CHECK(!v.empty());
        REQUIRE(v.size() == 3);
        CHECK(v[0].id == 10);
        CHECK(v[1].id == 11);
        CHECK(v[2].id == 12);
    };

    const auto &orig_const = orig;
    // NOLINTNEXTLINE(performance-unnecessary-copy-initialization)
    vector<ObjWithCopyCtor> v = orig_const;

    INFO("target");
    check_vec(v);

    INFO("origin");
    check_vec(orig);

#ifndef TEST_STD_VECTOR
    CHECK(orig.capacity() == 8);
    CHECK(v.capacity() == 4);
#endif
}

TEST_CASE("Copy-assign") {
    vector<ObjWithCopyAssignment> orig;
    orig.push_back(ObjWithCopyAssignment(10));
    orig.push_back(ObjWithCopyAssignment(11));
    orig.push_back(ObjWithCopyAssignment(12));
    orig.push_back(ObjWithCopyAssignment(13));
    orig.push_back(ObjWithCopyAssignment(14));
    // To check whether copy chooses minimal possible capacity.
    orig.pop_back();
    orig.pop_back();

    auto check_vec = [](const vector<ObjWithCopyAssignment> &v) {
        CHECK(!v.empty());
        REQUIRE(v.size() == 3);
        CHECK(v[0].id == 10);
        CHECK(v[1].id == 11);
        CHECK(v[2].id == 12);
    };

    auto test_copy_to = [&](vector<ObjWithCopyAssignment> &v) {
        const auto &orig_const = orig;
        CHECK(&(v = orig_const) == &v);
        INFO("target");
        check_vec(v);
        INFO("origin");
        check_vec(orig);
    };

    SUBCASE("to empty") {
        vector<ObjWithCopyAssignment> v;
        test_copy_to(v);
#ifndef TEST_STD_VECTOR
        CHECK(orig.capacity() == 8);
        CHECK(v.capacity() == 4);
#endif
    }

    SUBCASE("to shorter non-empty") {
        vector<ObjWithCopyAssignment> v(3, ObjWithCopyAssignment(20));
        test_copy_to(v);
#ifndef TEST_STD_VECTOR
        CHECK(orig.capacity() == 8);
        CHECK(v.capacity() == 4);
#endif
    }

    SUBCASE("to longer non-empty") {
        vector<ObjWithCopyAssignment> v(7, ObjWithCopyAssignment(20));
        test_copy_to(v);
#ifndef TEST_STD_VECTOR
        CHECK(orig.capacity() == 8);

        // We have to re-create buffer to provide strong exception safety,
        // hence we choose the minimal possible capacity.
        CHECK(v.capacity() == 4);
#endif
    }

    SUBCASE("to self") {
        ObjWithCopyAssignment *orig_buf = &orig[0];
        test_copy_to(orig);
        // Ensure there were no extra copies.
        CHECK(&orig[0] == orig_buf);
#ifndef TEST_STD_VECTOR
        CHECK(orig.capacity() == 8);
#endif
    }
}

TEST_CASE("Move-construct") {
    vector<MinimalObj> orig;
    orig.push_back(MinimalObj(10));
    orig.push_back(MinimalObj(11));
    orig.push_back(MinimalObj(12));
    orig.push_back(MinimalObj(13));
    orig.push_back(MinimalObj(14));

    MinimalObj *orig_buf = &orig[0];

    vector<MinimalObj> v = std::move(orig);
    CHECK(!v.empty());
    REQUIRE(v.size() == 5);
#ifndef TEST_STD_VECTOR
    CHECK(v.capacity() == 8);
#endif
    CHECK(v[0].id == 10);
    CHECK(v[1].id == 11);
    CHECK(v[2].id == 12);
    CHECK(v[3].id == 13);
    CHECK(v[4].id == 14);

    // Ensure there were no extra copies.
    CHECK(&v[0] == orig_buf);

    // NOLINTNEXTLINE(bugprone-use-after-move)
    CHECK(orig.empty());
    // NOLINTNEXTLINE(bugprone-use-after-move)
    CHECK(orig.size() == 0);
    // NOLINTNEXTLINE(bugprone-use-after-move)
    CHECK(orig.capacity() == 0);
}

TEST_CASE("Move-assign") {
    vector<MinimalObj> orig;
    orig.push_back(MinimalObj(10));
    orig.push_back(MinimalObj(11));
    orig.push_back(MinimalObj(12));
    orig.push_back(MinimalObj(13));
    orig.push_back(MinimalObj(14));

    MinimalObj *orig_buf = &orig[0];

    auto test_move_to_with_expected_capacity =
        [&](vector<MinimalObj> &v,
            [[maybe_unused]] std::size_t expected_capacity) {
            CHECK(&(v = std::move(orig)) == &v);

            CHECK(!v.empty());
            REQUIRE(v.size() == 5);
#ifndef TEST_STD_VECTOR
            CHECK(v.capacity() == 8);
#endif
            CHECK(v[0].id == 10);
            CHECK(v[1].id == 11);
            CHECK(v[2].id == 12);
            CHECK(v[3].id == 13);
            CHECK(v[4].id == 14);

            // Ensure there were no extra copies.
            CHECK(&v[0] == orig_buf);

            // NOLINTNEXTLINE(bugprone-use-after-move)
            CHECK(orig.empty());
            // NOLINTNEXTLINE(bugprone-use-after-move)
            CHECK(orig.size() == 0);
#ifndef TEST_STD_VECTOR
            // NOLINTNEXTLINE(bugprone-use-after-move)
            CHECK(orig.capacity() == expected_capacity);
#endif
        };

    SUBCASE("to empty") {
        vector<MinimalObj> v;
        test_move_to_with_expected_capacity(v, 0);
    };

    SUBCASE("to non-empty") {
        vector<MinimalObj> v;
        v.push_back(MinimalObj(100));
        v.push_back(MinimalObj(101));
        v.push_back(MinimalObj(102));
        test_move_to_with_expected_capacity(v, 4);
    };

    SUBCASE("to self") {
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wself-move"
#endif
        orig = std::move(orig);
#ifdef __clang__
#pragma clang diagnostic pop
#endif
        // No further checks on the state.
    };
}

TEST_CASE("Elements are consecutive") {
    vector<MinimalObj> v;
    v.push_back(MinimalObj(10));
    v.push_back(MinimalObj(11));
    v.push_back(MinimalObj(12));
    v.push_back(MinimalObj(13));
    v.push_back(MinimalObj(14));

    CHECK(&v[0] + 1 == &v[1]);
    CHECK(&v[0] + 2 == &v[2]);
    CHECK(&v[0] + 3 == &v[3]);
    CHECK(&v[0] + 4 == &v[4]);
}

TEST_CASE("Write to non-const") {
    vector<MinimalObj> v;
    v.push_back(MinimalObj(10));
    v.push_back(MinimalObj(11));
    v.push_back(MinimalObj(12));
    v.push_back(MinimalObj(13));
    v.push_back(MinimalObj(14));

    v[0].id = 15;
    CHECK(v[0].id == 15);

    v.at(1).id = 16;
    CHECK(v[1].id == 16);

    CHECK(&v[0] == &v.at(0));
    CHECK(&v[1] == &v.at(1));
    CHECK_THROWS_AS(v.at(5), std::out_of_range);
    CHECK_THROWS_AS(v.at(1'000'000'000), std::out_of_range);
}

TEST_CASE("Read from const") {
    vector<MinimalObj> orig;
    orig.push_back(MinimalObj(10));
    orig.push_back(MinimalObj(11));
    orig.push_back(MinimalObj(12));
    orig.push_back(MinimalObj(13));
    orig.push_back(MinimalObj(14));

    const vector<MinimalObj> &v = orig;
    CHECK(!v.empty());
    CHECK(v.size() == 5);
#ifndef TEST_STD_VECTOR
    CHECK(v.capacity() == 8);
#endif
    CHECK(v[0].id == 10);
    CHECK(v.at(0).id == 10);
    CHECK_THROWS_AS(v.at(5), std::out_of_range);
    CHECK_THROWS_AS(v.at(1'000'000'000), std::out_of_range);
}

TEST_CASE("reserve") {
    vector<MinimalObj> v;
    v.push_back(MinimalObj(10));
    v.push_back(MinimalObj(11));
    v.push_back(MinimalObj(12));
    v.push_back(MinimalObj(13));
    v.push_back(MinimalObj(14));

    auto check_vec_and_capacity =
        [&v]([[maybe_unused]] std::size_t expected_capacity) {
            CHECK(!v.empty());
            REQUIRE(v.size() == 5);
#ifndef TEST_STD_VECTOR
            CHECK(v.capacity() == expected_capacity);
#endif
            CHECK(v[0].id == 10);
            CHECK(v[1].id == 11);
            CHECK(v[2].id == 12);
            CHECK(v[3].id == 13);
            CHECK(v[4].id == 14);
        };

    SUBCASE("reserve to size") {
        v.reserve(5);
        check_vec_and_capacity(8);  // cppcheck-suppress invalidContainer
    }

    SUBCASE("reserve decreases") {
        v.reserve(1);
        check_vec_and_capacity(8);  // cppcheck-suppress invalidContainer
    }

    SUBCASE("reserve to capacity") {
        v.reserve(8);
        check_vec_and_capacity(8);  // cppcheck-suppress invalidContainer
    }

    SUBCASE("reserve bigger than capacity") {
        v.reserve(9);
        check_vec_and_capacity(16);  // cppcheck-suppress invalidContainer
    }

    SUBCASE("reserve much bigger than capacity") {
        v.reserve(100);
        check_vec_and_capacity(128);  // cppcheck-suppress invalidContainer
    }
}

TEST_CASE("pop_back") {
    vector<MinimalObj> v;
    v.push_back(MinimalObj(10));
    v.push_back(MinimalObj(11));
    v.push_back(MinimalObj(12));
    v.push_back(MinimalObj(13));
    v.push_back(MinimalObj(14));
    v.pop_back();
    v.pop_back();

    CHECK(!v.empty());
    REQUIRE(v.size() == 3);
#ifndef TEST_STD_VECTOR
    CHECK(v.capacity() == 8);
#endif
    CHECK(v[0].id == 10);
    CHECK(v[1].id == 11);
    CHECK(v[2].id == 12);
}

TEST_CASE("pop_back with push_back") {
    vector<MinimalObj> v;
    v.push_back(MinimalObj(10));
    v.push_back(MinimalObj(11));
    v.push_back(MinimalObj(12));
    v.pop_back();
    v.pop_back();
    v.pop_back();

    CHECK(v.empty());
    CHECK(v.size() == 0);
#ifndef TEST_STD_VECTOR
    CHECK(v.capacity() == 4);
#endif

    v.push_back(MinimalObj(13));

    CHECK(!v.empty());
    REQUIRE(v.size() == 1);
#ifndef TEST_STD_VECTOR
    CHECK(v.capacity() == 4);
#endif
    CHECK(v[0].id == 13);

    v.pop_back();

    CHECK(v.empty());
    CHECK(v.size() == 0);
#ifndef TEST_STD_VECTOR
    CHECK(v.capacity() == 4);
#endif
}

TEST_CASE("clear") {
    vector<MinimalObj> v;
    v.push_back(MinimalObj(10));
    v.push_back(MinimalObj(11));
    v.push_back(MinimalObj(12));
    v.push_back(MinimalObj(13));
    v.push_back(MinimalObj(14));
    v.clear();

    CHECK(v.empty());
    CHECK(v.size() == 0);
#ifndef TEST_STD_VECTOR
    CHECK(v.capacity() == 8);
#endif
}

TEST_CASE("resize default constructible") {
    vector<ObjWithDefaultCtor> v;
    v.push_back(ObjWithDefaultCtor(10));
    v.push_back(ObjWithDefaultCtor(11));
    v.push_back(ObjWithDefaultCtor(12));
    v.push_back(ObjWithDefaultCtor(13));
    v.push_back(ObjWithDefaultCtor(14));

    SUBCASE("to size") {
        v.resize(5);

        CHECK(!v.empty());
        REQUIRE(v.size() == 5);
#ifndef TEST_STD_VECTOR
        CHECK(v.capacity() == 8);
#endif
        CHECK(v[0].id == 10);
        CHECK(v[1].id == 11);
        CHECK(v[2].id == 12);
        CHECK(v[3].id == 13);
        CHECK(v[4].id == 14);
    }

    SUBCASE("to shorter") {
        v.resize(3);

        CHECK(!v.empty());
        REQUIRE(v.size() == 3);
#ifndef TEST_STD_VECTOR
        CHECK(v.capacity() == 8);
#endif
        CHECK(v[0].id == 10);
        CHECK(v[1].id == 11);
        CHECK(v[2].id == 12);
    }

    SUBCASE("to zero") {
        v.resize(0);

        CHECK(v.empty());
        CHECK(v.size() == 0);
#ifndef TEST_STD_VECTOR
        CHECK(v.capacity() == 8);
#endif
    }

    SUBCASE("to longer without reallocation") {
        v.resize(7);

        CHECK(!v.empty());
        REQUIRE(v.size() == 7);
#ifndef TEST_STD_VECTOR
        CHECK(v.capacity() == 8);
#endif
        CHECK(v[0].id == 10);
        CHECK(v[1].id == 11);
        CHECK(v[2].id == 12);
        CHECK(v[3].id == 13);
        CHECK(v[4].id == 14);
        CHECK(v[5].id == 100);
        CHECK(v[6].id == 100);
    }

    SUBCASE("to longer with reallocation") {
        v.resize(9);

        CHECK(!v.empty());
        REQUIRE(v.size() == 9);
#ifndef TEST_STD_VECTOR
        CHECK(v.capacity() == 16);
#endif
        CHECK(v[0].id == 10);
        CHECK(v[1].id == 11);
        CHECK(v[2].id == 12);
        CHECK(v[3].id == 13);
        CHECK(v[4].id == 14);
        CHECK(v[5].id == 100);
        CHECK(v[6].id == 100);
        CHECK(v[7].id == 100);
        CHECK(v[8].id == 100);
    }
}

TEST_CASE("resize with copy") {
#ifdef TEST_STD_VECTOR
    using Obj = ObjWithCopyAssignment;
#else
    using Obj = ObjWithCopyCtor;
#endif
    vector<Obj> v;
    v.push_back(Obj(10));
    v.push_back(Obj(11));
    v.push_back(Obj(12));
    v.push_back(Obj(13));
    v.push_back(Obj(14));

    SUBCASE("to size") {
        v.resize(5, Obj(50));

        CHECK(!v.empty());
        REQUIRE(v.size() == 5);
#ifndef TEST_STD_VECTOR
        CHECK(v.capacity() == 8);
#endif
        CHECK(v[0].id == 10);
        CHECK(v[1].id == 11);
        CHECK(v[2].id == 12);
        CHECK(v[3].id == 13);
        CHECK(v[4].id == 14);
    }

    SUBCASE("to shorter") {
        v.resize(3, Obj(50));

        CHECK(!v.empty());
        REQUIRE(v.size() == 3);
#ifndef TEST_STD_VECTOR
        CHECK(v.capacity() == 8);
#endif
        CHECK(v[0].id == 10);
        CHECK(v[1].id == 11);
        CHECK(v[2].id == 12);
    }

    SUBCASE("to zero") {
        v.resize(0, Obj(50));

        CHECK(v.empty());
        CHECK(v.size() == 0);
#ifndef TEST_STD_VECTOR
        CHECK(v.capacity() == 8);
#endif
    }

    SUBCASE("to longer without reallocation") {
        v.resize(7, Obj(50));

        CHECK(!v.empty());
        REQUIRE(v.size() == 7);
#ifndef TEST_STD_VECTOR
        CHECK(v.capacity() == 8);
#endif
        CHECK(v[0].id == 10);
        CHECK(v[1].id == 11);
        CHECK(v[2].id == 12);
        CHECK(v[3].id == 13);
        CHECK(v[4].id == 14);
        CHECK(v[5].id == 50);
        CHECK(v[6].id == 50);
    }

    SUBCASE("to longer with reallocation") {
        v.resize(9, Obj(50));

        CHECK(!v.empty());
        REQUIRE(v.size() == 9);
#ifndef TEST_STD_VECTOR
        CHECK(v.capacity() == 16);
#endif
        CHECK(v[0].id == 10);
        CHECK(v[1].id == 11);
        CHECK(v[2].id == 12);
        CHECK(v[3].id == 13);
        CHECK(v[4].id == 14);
        CHECK(v[5].id == 50);
        CHECK(v[6].id == 50);
        CHECK(v[7].id == 50);
        CHECK(v[8].id == 50);
    }
}

TEST_CASE(
    "push_back copy keeps strong exception safety even for capacity when "
    "reallocating") {
    struct artificial_exception {};
    struct S {
        // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
        bool can_copy;
        // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
        std::string data = std::string(500U, 'x');
        // Fun fact: until this field was here,
        // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=96188 was triggered.

        explicit S(bool can_copy_ = true) : can_copy(can_copy_) {
        }

        S(S &&) = default;
        S &operator=(S &&) = default;

        S(const S &other) : can_copy(other.can_copy) {
            if (!other.can_copy) {
                throw artificial_exception();
            }
        }

        S &operator=(const S &other) = delete;

        ~S() = default;
    };

    vector<S> v(4);
    CHECK(!v.empty());
    REQUIRE(v.size() == 4);
#ifndef TEST_STD_VECTOR
    CHECK(v.capacity() == 4);
#endif

    const S obj(false);
    CHECK_THROWS_AS(v.push_back(obj), artificial_exception);

    CHECK(!v.empty());
    REQUIRE(v.size() == 4);
#ifndef TEST_STD_VECTOR
    CHECK(v.capacity() == 4);
#endif
    CHECK(v[0].data == std::string(500U, 'x'));
    CHECK(v[1].data == std::string(500U, 'x'));
    CHECK(v[2].data == std::string(500U, 'x'));
    CHECK(v[3].data == std::string(500U, 'x'));
}

TEST_CASE(
    "resize keeps strong exception safety even for capacity when "
    "reallocating") {
    struct artificial_exception {};
    struct S {
        // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
        std::string data = std::string(500U, 'x');

        S() {
            throw artificial_exception();
        }

        explicit S(int) {
        }
    };

    vector<S> v;
    v.push_back(S(10));
    v.push_back(S(10));
    CHECK(!v.empty());
    REQUIRE(v.size() == 2);
#ifndef TEST_STD_VECTOR
    CHECK(v.capacity() == 2);
#endif

    CHECK_THROWS_AS(v.resize(10), artificial_exception);

    CHECK(!v.empty());
    REQUIRE(v.size() == 2);
#ifndef TEST_STD_VECTOR
    CHECK(v.capacity() == 2);
#endif
    CHECK(v[0].data == std::string(500U, 'x'));
    CHECK(v[1].data == std::string(500U, 'x'));
}

#ifndef TEST_STD_VECTOR
TEST_CASE("copy assignment keeps strong exception safety") {
    struct artificial_exception {};
    struct S {
        // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
        bool can_copy = true;
        // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
        std::string data;

        explicit S(std::string data_) : data(std::move(data_)) {
        }

        S(S &&) = default;
        S &operator=(S &&) = default;

        S(const S &other) : can_copy(other.can_copy), data(other.data) {
            if (!can_copy) {
                throw artificial_exception();
            }
        }

        S &operator=(const S &other) {
            can_copy = other.can_copy;
            data = other.data;
            if (!can_copy) {
                throw artificial_exception();
            }
            return *this;
        }

        ~S() = default;
    };

    vector<S> v1;
    vector<S> v2;
    v1.push_back(S(std::string(500U, 'a')));
    v1.push_back(S(std::string(500U, 'b')));
    v1.push_back(S(std::string(500U, 'c')));
    v2.push_back(S(std::string(500U, 'd')));
    v2.push_back(S(std::string(500U, 'e')));
    v2.push_back(S(std::string(500U, 'f')));

    v1[1].can_copy = false;
    CHECK_THROWS_AS(v2 = v1, artificial_exception);

    CHECK(!v2.empty());
    REQUIRE(v2.size() == 3);
    CHECK(v2.capacity() == 4);
    CHECK(v2[0].data == std::string(500U, 'd'));
    CHECK(v2[1].data == std::string(500U, 'e'));
    CHECK(v2[2].data == std::string(500U, 'f'));
}
#endif

TEST_CASE("operator[] and at() have lvalue/rvalue overloads") {
    struct TracingObj {
        // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
        int kind = 0;

        TracingObj() = default;
        TracingObj(const TracingObj &) : kind(1) {
        }
        TracingObj(TracingObj &&other) noexcept : kind(2) {
            other.kind = -2;
        }
        TracingObj &operator=(const TracingObj &) {
            kind = 3;
            return *this;
        }
        TracingObj &operator=(TracingObj &&other) noexcept {
            kind = 4;
            other.kind = -4;
            return *this;
        }
        ~TracingObj() = default;
    };
    vector<TracingObj> v(3);
    v[0].kind = 10;
    v[1].kind = 20;
    v.at(2).kind = 30;

    SUBCASE("operator[] &") {
        TracingObj o = v[0];
        CHECK(o.kind == 1);
        CHECK(v[0].kind == 10);
    }

    SUBCASE("at() &") {
        TracingObj o = v.at(0);
        CHECK(o.kind == 1);
        CHECK(v[0].kind == 10);
    }

#ifndef TEST_STD_VECTOR
    SUBCASE("operator[] &&") {
        TracingObj o = std::move(v)[0];
        CHECK(o.kind == 2);
        // NOLINTNEXTLINE(bugprone-use-after-move)
        CHECK(v[0].kind == -2);
    }

    SUBCASE("at() &&") {
        TracingObj o = std::move(v).at(0);
        CHECK(o.kind == 2);
        // NOLINTNEXTLINE(bugprone-use-after-move)
        CHECK(v[0].kind == -2);
    }
#endif
}

TEST_CASE("new elements are value-initialized") {
    SUBCASE("in constructor") {
        for (int step = 0; step < 10; step++) {
            vector<int> vec(1000);
            for (std::size_t i = 0; i < vec.size(); i++) {
                REQUIRE(vec[i] == 0);
                vec[i] = 10;
            }
        }
    }

    SUBCASE("in resize with and without reallocation") {
        for (int step = 0; step < 10; step++) {
            vector<int> vec;
            vec.resize(500);
            for (std::size_t i = 0; i < 500; i++) {
                REQUIRE(vec[i] == 0);
                vec[i] = 10;
            }
            vec.resize(1000);
            for (std::size_t i = 500; i < 1000; i++) {
                REQUIRE(vec[i] == 0);
                vec[i] = 10;
            }
            vec.resize(0);
            vec.resize(1000);
            for (std::size_t i = 0; i < vec.size(); i++) {
                REQUIRE(vec[i] == 0);
                vec[i] = 10;
            }
        }
    }
}

namespace {
struct Counters {
    std::size_t new_count = 0;
    std::size_t new_total_elems = 0;
    std::size_t delete_count = 0;
    std::size_t delete_total_elems = 0;
} global_counters;
}  // namespace

Counters operator-(const Counters &a, const Counters &b) {
    return Counters{a.new_count - b.new_count,
                    a.new_total_elems - b.new_total_elems,
                    a.delete_count - b.delete_count,
                    a.delete_total_elems - b.delete_total_elems};
}

template <typename Fn>
Counters with_counters(Fn fn) {
    Counters start = global_counters;
    fn();
    return global_counters - start;
}

namespace {
template <typename T>
struct CounterAllocator {
    static_assert(alignof(T) <= __STDCPP_DEFAULT_NEW_ALIGNMENT__);

    using value_type = T;

    T *allocate(std::size_t count) {
        CHECK(count > 0);
        T *result = static_cast<T *>(::operator new(count * sizeof(T)));
        global_counters.new_count++;
        global_counters.new_total_elems += count;
        return result;
    }

    void deallocate(T *ptr, std::size_t count) noexcept {
        CHECK(ptr != nullptr);
        CHECK(count > 0);
        ::operator delete(ptr);
        global_counters.delete_count++;
        global_counters.delete_total_elems += count;
    }
};
}  // namespace

TEST_CASE(
    "custom allocator is used by "
#ifdef TEST_STD_VECTOR
    "std::vector<std::string>"
#else
    "lab_07::vector<std::string>"
#endif
) {
    Counters res = with_counters([]() {
        struct S {
            char buf[40]{};
        };
        vector<S, CounterAllocator<S>> vec_empty;
        vector<S, CounterAllocator<S>> vec(10);
        CHECK(vec[0].buf[0] == 0);
    });
    CHECK(res.new_count == 1);
    CHECK(res.delete_count == 1);
#ifndef TEST_STD_VECTOR
    CHECK(res.new_total_elems == 16);
    CHECK(res.delete_total_elems == 16);
#endif
}
