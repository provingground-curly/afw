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

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE SimpleHeteroMapCpp
#define BOOST_TEST_NO_MAIN
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-variable"
#include "boost/test/unit_test.hpp"
#pragma clang diagnostic pop

#include <memory>
#include <string>

#include "lsst/afw/typehandling/SimpleHeteroMap.h"
#include "lsst/afw/typehandling/test.h"

using namespace std::string_literals;

namespace lsst {
namespace afw {
namespace typehandling {

class SimpleHeteroMapFactory final : public test::HeteroFactory {
public:
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
    virtual std::unique_ptr<HeteroMap<int>> makeHeteroMap() const {
        auto map = std::make_unique<SimpleHeteroMap<int>>();
        map->insert(test::KEY0, test::VALUE0);
        map->insert(test::KEY1, test::VALUE1);
        map->insert<double>(test::KEY2, test::VALUE2);
        map->insert(test::KEY3, test::VALUE3);
        map->insert(test::KEY4, std::make_shared<test::SimpleStorable>(test::VALUE4));
        map->insert(test::KEY5, test::VALUE5);
        return map;
    }

    /// Create an empty map.
    virtual std::unique_ptr<MutableHeteroMap<std::string>> makeMutableHeteroMap() const {
        return std::make_unique<SimpleHeteroMap<std::string>>();
    }
};

/// Boost::test initialization function
// Yes, we have to do it this way if we want the HeteroMap tests to be defined
// before we have a concrete HeteroMap class
// https://www.boost.org/doc/libs/1_68_0/libs/test/doc/html/boost_test/tests_organization/test_cases/test_organization_templates.html#ref_BOOST_TEST_CASE_TEMPLATE
bool init_unit_test() {
    test::addMutableHeteroMapTestCases<SimpleHeteroMapFactory>();
    return true;
}

}  // namespace typehandling
}  // namespace afw
}  // namespace lsst

/// Boost::test entry point
// Must be customized to call our init_unit_test
// https://www.boost.org/doc/libs/1_68_0/libs/test/doc/html/boost_test/adv_scenarios/shared_lib_customizations/init_func.html
int main(int argc, char* argv[]) {
    return boost::unit_test::unit_test_main(&lsst::afw::typehandling::init_unit_test, argc, argv);
}
