// -*- LSST-C++ -*-
/*
 * This file is part of afw.
 *
 * Developed for the LSST Data Management System.
 * This product includes software developed by the LSST Project
 * (https://www.lsst.org).
 * See the COPYRIGHT file at the top-level directory of this distribution
 * for details of code ownership.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef LSST_AFW_TYPEHANDLING_TEST_H
#define LSST_AFW_TYPEHANDLING_TEST_H

#define BOOST_TEST_DYN_LINK
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-variable"
#include "boost/test/unit_test.hpp"
#pragma clang diagnostic pop

#include <memory>
#include <set>
#include <string>

#include <boost/mpl/list.hpp>

#include "lsst/pex/exceptions.h"

#include "lsst/afw/typehandling/HeteroMap.h"
#include "lsst/afw/typehandling/Storable.h"

namespace lsst {
namespace afw {
namespace typehandling {
namespace test {

/*
 * This include file defines tests that exercise the HeteroMap and MutableHeteroMap interfaces, and ensures
 * that any implementation satisfies the requirements of these interfaces. Subclass authors should call either
 * addHeteroMapTestCases or addMutableHeteroMapTestCases in a suitable entry point, such as a global fixture
 * or a module initialization function.
 */

class SimpleStorable : public Storable {
public:
    virtual ~SimpleStorable() = default;

    std::unique_ptr<Storable> clone() const override { return std::make_unique<SimpleStorable>(); }

    std::string toString() const override { return "Simplest possible representation"; }

protected:
    bool equals(Storable const& other) const noexcept override {
        auto simpleOther = dynamic_cast<SimpleStorable const*>(&other);
        return simpleOther != nullptr;
    }
};

class ComplexStorable final : public SimpleStorable {
public:
    constexpr ComplexStorable(double storage) : SimpleStorable(), storage(storage) {}

    std::unique_ptr<Storable> clone() const override { return std::make_unique<ComplexStorable>(storage); }

    std::string toString() const override { return "ComplexStorable(" + std::to_string(storage) + ")"; }

    std::size_t hash_value() const noexcept override { return std::hash<double>()(storage); }

protected:
    // Warning: violates both substitution and equality symmetry!
    bool equals(Storable const& other) const noexcept override {
        auto complexOther = dynamic_cast<ComplexStorable const*>(&other);
        if (complexOther) {
            return this->storage == complexOther->storage;
        } else {
            return false;
        }
    }

private:
    double storage;
};

namespace {
// Would make more sense as static constants in HeteroFactory
// but neither string nor Storable qualify as literal types
// In anonymous namespace to ensure constants are internal to whatever test includes this header
auto const KEY0 = makeKey<bool>(0);
bool const VALUE0 = true;
auto const KEY1 = makeKey<int>(1);
int const VALUE1 = 42;
auto const KEY2 = makeKey<double>(2);
int const VALUE2 = VALUE1;
auto const KEY3 = makeKey<std::string>(3);
std::string const VALUE3 = "How many roads must a man walk down?";
auto const KEY4 = makeKey<std::shared_ptr<SimpleStorable>>(4);
auto const VALUE4 = SimpleStorable();
auto const KEY5 = makeKey<ComplexStorable>(5);
auto const VALUE5 = ComplexStorable(-100.0);
}  // namespace

/**
 * Abstract factory that creates HeteroMap and MutableHeteroMap instances as needed.
 */
class HeteroFactory {
public:
    virtual ~HeteroFactory() = default;

    /**
     * Create a map containing the following state:
     *
     * * `KEY0: VALUE0`
     * * `KEY1: VALUE1`
     * * `KEY2: VALUE2`
     * * `KEY3: VALUE3`
     * * `KEY4: std::shared_ptr<>(VALUE4)`
     * * `KEY5: VALUE5`
     */
    virtual std::unique_ptr<HeteroMap<int>> makeHeteroMap() const = 0;

    /// Create an empty map.
    virtual std::unique_ptr<MutableHeteroMap<std::string>> makeMutableHeteroMap() const = 0;
};

BOOST_TEST_CASE_TEMPLATE_FUNCTION(TestConstAt, HeteroMapFactory) {
    static HeteroMapFactory const factory;
    std::unique_ptr<HeteroMap<int> const> demoMap = factory.makeHeteroMap();

    BOOST_TEST(demoMap->at(KEY0) == VALUE0);
    BOOST_TEST(demoMap->at(KEY1) == VALUE1);
    BOOST_TEST(demoMap->at(KEY2) == VALUE2);
    BOOST_TEST(demoMap->at(KEY3) == VALUE3);
    BOOST_TEST(*(demoMap->at(KEY4)) == VALUE4);
    BOOST_TEST(demoMap->at(KEY5) == VALUE5);
}

BOOST_TEST_CASE_TEMPLATE_FUNCTION(TestAt, HeteroMapFactory) {
    using namespace std::string_literals;

    static HeteroMapFactory const factory;
    std::unique_ptr<HeteroMap<int>> demoMap = factory.makeHeteroMap();

    BOOST_TEST(demoMap->at(KEY0) == VALUE0);
    demoMap->at(KEY0) = false;
    BOOST_TEST(demoMap->at(KEY0) == false);
    BOOST_CHECK_THROW(demoMap->at(makeKey<int>(KEY0.getId())), pex::exceptions::OutOfRangeError);

    BOOST_TEST(demoMap->at(KEY1) == VALUE1);
    demoMap->at(KEY1)++;
    BOOST_TEST(demoMap->at(KEY1) == VALUE1 + 1);
    BOOST_CHECK_THROW(demoMap->at(makeKey<bool>(KEY1.getId())), pex::exceptions::OutOfRangeError);

    BOOST_TEST(demoMap->at(KEY2) == VALUE2);
    demoMap->at(KEY2) = 0.0;
    BOOST_TEST(demoMap->at(KEY2) == 0.0);
    // VALUE2 is of a different type than KEY2, check that alternate key is absent
    using Type2 = std::remove_const_t<decltype(VALUE2)>;
    BOOST_CHECK_THROW(demoMap->at(makeKey<Type2>(KEY2.getId())), pex::exceptions::OutOfRangeError);

    BOOST_TEST(demoMap->at(KEY3) == VALUE3);
    demoMap->at(KEY3).append(" Oops, wrong question."s);
    BOOST_TEST(demoMap->at(KEY3) == VALUE3 + " Oops, wrong question."s);

    BOOST_TEST(*(demoMap->at(KEY4)) == VALUE4);
    // VALUE4 is of a different type than KEY4, check that alternate key is absent
    using Type4 = std::remove_const_t<decltype(VALUE4)>;
    BOOST_CHECK_THROW(demoMap->at(makeKey<Type4>(KEY4.getId())), pex::exceptions::OutOfRangeError);

    BOOST_TEST(demoMap->at(KEY5) == VALUE5);
    BOOST_TEST(demoMap->at(makeKey<SimpleStorable>(KEY5.getId())) == VALUE5);
    // Use BOOST_CHECK_EQUAL to avoid BOOST_TEST bug from Storable being abstract
    BOOST_CHECK_EQUAL(demoMap->at(makeKey<Storable>(KEY5.getId())), VALUE5);

    ComplexStorable newValue(5.0);
    demoMap->at(KEY5) = newValue;
    BOOST_TEST(demoMap->at(KEY5) == newValue);
}

BOOST_TEST_CASE_TEMPLATE_FUNCTION(TestSize, HeteroMapFactory) {
    static HeteroMapFactory const factory;
    std::unique_ptr<HeteroMap<int>> demoMap = factory.makeHeteroMap();

    BOOST_TEST(demoMap->size() == 6);
    BOOST_TEST(!demoMap->empty());
}

BOOST_TEST_CASE_TEMPLATE_FUNCTION(TestMutableSize, HeteroMapFactory) {
    using namespace std::string_literals;

    static HeteroMapFactory const factory;
    std::unique_ptr<MutableHeteroMap<std::string>> demoMap = factory.makeMutableHeteroMap();

    BOOST_TEST_REQUIRE(demoMap->size() == 0);
    BOOST_TEST_REQUIRE(demoMap->empty());

    demoMap->insert(makeKey<int>("Negative One"s), -1);
    BOOST_TEST(demoMap->size() == 1);
    BOOST_TEST(!demoMap->empty());

    demoMap->erase(makeKey<int>("Negative One"s));
    BOOST_TEST(demoMap->size() == 0);
    BOOST_TEST(demoMap->empty());
}

BOOST_TEST_CASE_TEMPLATE_FUNCTION(TestWeakContains, HeteroMapFactory) {
    using namespace std::string_literals;

    static HeteroMapFactory const factory;
    std::unique_ptr<HeteroMap<int> const> demoMap = factory.makeHeteroMap();

    BOOST_TEST(demoMap->contains(KEY0.getId()));
    BOOST_TEST(demoMap->contains(KEY1.getId()));
    BOOST_TEST(demoMap->contains(KEY2.getId()));
    BOOST_TEST(demoMap->contains(KEY3.getId()));
    BOOST_TEST(demoMap->contains(KEY4.getId()));
    BOOST_TEST(demoMap->contains(KEY5.getId()));
    BOOST_TEST(!demoMap->contains(6));
}

BOOST_TEST_CASE_TEMPLATE_FUNCTION(TestContains, HeteroMapFactory) {
    static HeteroMapFactory const factory;
    std::unique_ptr<HeteroMap<int> const> demoMap = factory.makeHeteroMap();

    BOOST_TEST(demoMap->contains(KEY0));
    BOOST_TEST(!demoMap->contains(makeKey<int>(KEY0.getId())));

    BOOST_TEST(demoMap->contains(KEY1));
    BOOST_TEST(!demoMap->contains(makeKey<bool>(KEY1.getId())));

    BOOST_TEST(demoMap->contains(KEY2));
    // VALUE2 is of a different type than KEY2, check that alternate key is absent
    BOOST_TEST(!demoMap->contains(makeKey<decltype(VALUE2)>(KEY2.getId())));

    BOOST_TEST(demoMap->contains(KEY3));

    BOOST_TEST(demoMap->contains(KEY4));
    // VALUE4 is of a different type than KEY4, check that alternate key is absent
    BOOST_TEST(!demoMap->contains(makeKey<decltype(VALUE4)>(KEY4.getId())));

    BOOST_TEST(demoMap->contains(KEY5));
    BOOST_TEST(demoMap->contains(makeKey<SimpleStorable>(KEY5.getId())));
    BOOST_TEST(demoMap->contains(makeKey<Storable>(KEY5.getId())));
}

BOOST_TEST_CASE_TEMPLATE_FUNCTION(TestKeys, HeteroMapFactory) {
    static HeteroMapFactory const factory;
    std::unique_ptr<HeteroMap<int> const> demoMap = factory.makeHeteroMap();
    auto orderedKeys = demoMap->keys();
    // HeteroMaps don't have an iteration order yet
    std::set<int> keys(orderedKeys.begin(), orderedKeys.end());

    BOOST_TEST(keys == std::set<int>({KEY0.getId(), KEY1.getId(), KEY2.getId(), KEY3.getId(), KEY4.getId(),
                                      KEY5.getId()}));
}

BOOST_TEST_CASE_TEMPLATE_FUNCTION(TestClearIdempotent, HeteroMapFactory) {
    static HeteroMapFactory const factory;
    std::unique_ptr<MutableHeteroMap<std::string>> demoMap = factory.makeMutableHeteroMap();

    BOOST_TEST_REQUIRE(demoMap->empty());
    demoMap->clear();
    BOOST_TEST(demoMap->empty());
}

BOOST_TEST_CASE_TEMPLATE_FUNCTION(TestClear, HeteroMapFactory) {
    using namespace std::string_literals;

    static HeteroMapFactory const factory;
    std::unique_ptr<MutableHeteroMap<std::string>> demoMap = factory.makeMutableHeteroMap();

    demoMap->insert(makeKey<int>("prime"s), 3);
    demoMap->insert(makeKey<std::string>("foo"s), "bar"s);

    BOOST_TEST_REQUIRE(!demoMap->empty());
    demoMap->clear();
    BOOST_TEST(demoMap->empty());
}

BOOST_TEST_CASE_TEMPLATE_FUNCTION(TestInsertInt, HeteroMapFactory) {
    using namespace std::string_literals;

    static HeteroMapFactory const factory;
    std::unique_ptr<MutableHeteroMap<std::string>> demoMap = factory.makeMutableHeteroMap();

    BOOST_TEST_REQUIRE(demoMap->empty());

    BOOST_TEST(demoMap->insert(makeKey<int>("cube"s), 27) == true);
    int x = 0;
    BOOST_TEST(demoMap->insert(makeKey<int>("cube"s), x) == false);

    BOOST_TEST(!demoMap->empty());
    BOOST_TEST(demoMap->size() == 1);
    BOOST_TEST(demoMap->contains("cube"s));
    BOOST_TEST(demoMap->contains(makeKey<int>("cube"s)));
    BOOST_TEST(!demoMap->contains(makeKey<double>("cube"s)));
    BOOST_TEST(demoMap->at(makeKey<int>("cube"s)) == 27);

    demoMap->at(makeKey<int>("cube"s)) = 0;
    BOOST_TEST(demoMap->at(makeKey<int>("cube"s)) == 0);
}

BOOST_TEST_CASE_TEMPLATE_FUNCTION(TestInsertString, HeteroMapFactory) {
    using namespace std::string_literals;

    static HeteroMapFactory const factory;
    std::unique_ptr<MutableHeteroMap<std::string>> demoMap = factory.makeMutableHeteroMap();

    BOOST_TEST_REQUIRE(demoMap->empty());

    BOOST_TEST(demoMap->insert(makeKey<std::string>("Ultimate answer"s), "Something philosophical"s) == true);
    BOOST_TEST(demoMap->insert(makeKey<std::string>("OK"s), "Ook!"s) == true);
    std::string answer(
            "I have a most elegant and wonderful proof, but this string is too small to contain it."s);
    BOOST_TEST(demoMap->insert(makeKey<std::string>("Ultimate answer"s), answer) == false);

    BOOST_TEST(!demoMap->empty());
    BOOST_TEST(demoMap->size() == 2);
    BOOST_TEST(demoMap->contains("OK"s));
    BOOST_TEST(demoMap->contains(makeKey<std::string>("Ultimate answer"s)));
    BOOST_TEST(demoMap->at(makeKey<std::string>("Ultimate answer"s)) == "Something philosophical"s);
    BOOST_TEST(demoMap->at(makeKey<std::string>("OK"s)) == "Ook!"s);
    answer = "I don't know"s;
    BOOST_TEST(demoMap->at(makeKey<std::string>("Ultimate answer"s)) != answer);
}

BOOST_TEST_CASE_TEMPLATE_FUNCTION(TestInsertStorable, HeteroMapFactory) {
    using namespace std::string_literals;

    static HeteroMapFactory const factory;
    std::unique_ptr<MutableHeteroMap<std::string>> demoMap = factory.makeMutableHeteroMap();

    BOOST_TEST_REQUIRE(demoMap->empty());

    ComplexStorable object(3.1416);
    BOOST_TEST(demoMap->insert<Storable>(makeKey<Storable>("foo"s), object) == true);
    BOOST_TEST(demoMap->insert(makeKey<std::shared_ptr<ComplexStorable>>("bar"s),
                               std::make_shared<ComplexStorable>(3.141)) == true);
    BOOST_TEST(demoMap->insert<Storable>(makeKey<Storable>("foo"s), SimpleStorable()) == false);
    BOOST_TEST(demoMap->insert(makeKey<std::shared_ptr<SimpleStorable>>("bar"s),
                               std::make_shared<SimpleStorable>()) == false);

    BOOST_TEST(!demoMap->empty());
    BOOST_TEST(demoMap->size() == 2);
    BOOST_TEST(demoMap->contains("foo"s));
    BOOST_TEST(demoMap->contains(makeKey<Storable>("foo"s)));
    BOOST_TEST(demoMap->contains(makeKey<std::shared_ptr<ComplexStorable>>("bar"s)));

    // ComplexStorable::operator== is asymmetric
    // Use BOOST_CHECK_EQUAL to avoid BOOST_TEST bug from Storable being abstract
    BOOST_CHECK_EQUAL(object, demoMap->at(makeKey<Storable>("foo"s)));
    object = ComplexStorable(1.4);
    BOOST_CHECK_NE(object, demoMap->at(makeKey<Storable>("foo"s)));
    BOOST_CHECK_EQUAL(*(demoMap->at(makeKey<std::shared_ptr<ComplexStorable>>("bar"s))),
                      ComplexStorable(3.141));
}

BOOST_TEST_CASE_TEMPLATE_FUNCTION(TestInterleavedInserts, HeteroMapFactory) {
    using namespace std::string_literals;

    static HeteroMapFactory const factory;
    std::unique_ptr<MutableHeteroMap<std::string>> demoMap = factory.makeMutableHeteroMap();

    BOOST_TEST_REQUIRE(demoMap->empty());

    BOOST_TEST(demoMap->insert(makeKey<int>("key1"s), 3) == true);
    BOOST_TEST(demoMap->insert(makeKey<double>("key1"s), 1.0) == false);
    BOOST_TEST(demoMap->insert<Storable>(makeKey<Storable>("key2"s), SimpleStorable()) == true);
    BOOST_TEST(demoMap->insert(makeKey<std::string>("key3"s), "Test value"s) == true);
    BOOST_TEST(demoMap->insert(makeKey<std::string>("key4"s), "This is some text"s) == true);
    std::string const message = "Unknown value for key5."s;
    BOOST_TEST(demoMap->insert(makeKey<std::string>("key5"s), message) == true);
    BOOST_TEST(demoMap->insert(makeKey<int>("key3"s), 20) == false);
    BOOST_TEST(demoMap->insert<double>(makeKey<double>("key6"s), 42) == true);

    BOOST_TEST(!demoMap->empty());
    BOOST_TEST(demoMap->size() == 6);
    BOOST_TEST(demoMap->at(makeKey<int>("key1"s)) == 3);
    BOOST_TEST(demoMap->at(makeKey<double>("key6"s)) == 42);
    // Use BOOST_CHECK_EQUAL to avoid BOOST_TEST bug from Storable being abstract
    BOOST_CHECK_EQUAL(demoMap->at(makeKey<Storable>("key2"s)), SimpleStorable());
    BOOST_CHECK_EQUAL(demoMap->at(makeKey<std::string>("key3"s)), "Test value"s);
    BOOST_CHECK_EQUAL(demoMap->at(makeKey<std::string>("key4"s)), "This is some text"s);
    BOOST_CHECK_EQUAL(demoMap->at(makeKey<std::string>("key5"s)), message);
}

BOOST_TEST_CASE_TEMPLATE_FUNCTION(TestErase, HeteroMapFactory) {
    using namespace std::string_literals;

    static HeteroMapFactory const factory;
    std::unique_ptr<MutableHeteroMap<std::string>> demoMap = factory.makeMutableHeteroMap();

    demoMap->insert(makeKey<int>("Ultimate answer"s), 42);
    BOOST_TEST_REQUIRE(demoMap->size() == 1);

    BOOST_TEST(demoMap->erase(makeKey<std::string>("Ultimate answer"s)) == false);
    BOOST_TEST(demoMap->size() == 1);
    BOOST_TEST(demoMap->erase(makeKey<int>("Ultimate answer"s)) == true);
    BOOST_TEST(demoMap->size() == 0);
}

BOOST_TEST_CASE_TEMPLATE_FUNCTION(TestInsertEraseInsert, HeteroMapFactory) {
    using namespace std::string_literals;

    static HeteroMapFactory const factory;
    std::unique_ptr<MutableHeteroMap<std::string>> demoMap = factory.makeMutableHeteroMap();

    BOOST_TEST_REQUIRE(demoMap->empty());

    BOOST_TEST(demoMap->insert(makeKey<int>("Ultimate answer"s), 42) == true);
    BOOST_TEST(demoMap->insert(makeKey<int>("OK"s), 200) == true);
    BOOST_TEST(demoMap->erase(makeKey<int>("Ultimate answer"s)) == true);
    BOOST_TEST(demoMap->insert(makeKey<double>("Ultimate answer"s), 3.1415927) == true);

    BOOST_TEST(!demoMap->empty());
    BOOST_TEST(demoMap->size() == 2);
    BOOST_TEST(demoMap->contains("OK"s));
    BOOST_TEST(!demoMap->contains(makeKey<int>("Ultimate answer"s)));
    BOOST_TEST(demoMap->contains(makeKey<double>("Ultimate answer"s)));
    BOOST_TEST(demoMap->at(makeKey<double>("Ultimate answer"s)) == 3.1415927);
}

/**
 * Create generic test cases for a specific HeteroMap implementation.
 *
 * @tparam HeteroMapFactory a subclass of HeteroFactory that creates the desired implementation. Must be
 * default-constructible.
 * @param suite the test suite to add the tests to.
 */
template <class HeteroMapFactory>
void addHeteroMapTestCases(boost::unit_test::test_suite* const suite) {
    using factories = boost::mpl::list<HeteroMapFactory>;

    suite->add(BOOST_TEST_CASE_TEMPLATE(TestConstAt, factories));
    suite->add(BOOST_TEST_CASE_TEMPLATE(TestAt, factories));
    suite->add(BOOST_TEST_CASE_TEMPLATE(TestSize, factories));
    suite->add(BOOST_TEST_CASE_TEMPLATE(TestWeakContains, factories));
    suite->add(BOOST_TEST_CASE_TEMPLATE(TestContains, factories));
    suite->add(BOOST_TEST_CASE_TEMPLATE(TestKeys, factories));
}

/**
 * Create generic test cases for a specific MutableHeteroMap implementation.
 *
 * The tests will include all those added by addHeteroMapTestCases.
 *
 * @tparam HeteroMapFactory a subclass of HeteroFactory that creates the desired implementation. Must be
 * default-constructible.
 * @param suite the test suite to add the tests to.
 */
template <class HeteroMapFactory>
void addMutableHeteroMapTestCases(boost::unit_test::test_suite* const suite) {
    using factories = boost::mpl::list<HeteroMapFactory>;

    addHeteroMapTestCases<HeteroMapFactory>(suite);

    suite->add(BOOST_TEST_CASE_TEMPLATE(TestMutableSize, factories));
    suite->add(BOOST_TEST_CASE_TEMPLATE(TestClear, factories));
    suite->add(BOOST_TEST_CASE_TEMPLATE(TestClearIdempotent, factories));
    suite->add(BOOST_TEST_CASE_TEMPLATE(TestInsertInt, factories));
    suite->add(BOOST_TEST_CASE_TEMPLATE(TestInsertString, factories));
    suite->add(BOOST_TEST_CASE_TEMPLATE(TestInsertStorable, factories));
    suite->add(BOOST_TEST_CASE_TEMPLATE(TestInterleavedInserts, factories));
    suite->add(BOOST_TEST_CASE_TEMPLATE(TestErase, factories));
    suite->add(BOOST_TEST_CASE_TEMPLATE(TestInsertEraseInsert, factories));
}

/**
 * Create generic test cases for a specific HeteroMap implementation.
 *
 * The tests will be added to the master test suite.
 *
 * @tparam HeteroMapFactory a subclass of HeteroFactory that creates the desired implementation. Must be
 * default-constructible.
 */
template <class HeteroMapFactory>
inline void addHeteroMapTestCases() {
    addHeteroMapTestCases<HeteroMapFactory>(&(boost::unit_test::framework::master_test_suite()));
}

/**
 * Create generic test cases for a specific MutableHeteroMap implementation.
 *
 * The tests will be added to the master test suite. They will include all tests added by
 * addHeteroMapTestCases.
 *
 * @tparam HeteroMapFactory a subclass of HeteroFactory that creates the desired implementation. Must be
 * default-constructible.
 */
template <class HeteroMapFactory>
inline void addMutableHeteroMapTestCases() {
    addMutableHeteroMapTestCases<HeteroMapFactory>(&(boost::unit_test::framework::master_test_suite()));
}

}  // namespace test
}  // namespace typehandling
}  // namespace afw
}  // namespace lsst

#endif
