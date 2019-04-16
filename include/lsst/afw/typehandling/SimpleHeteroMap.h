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

#ifndef LSST_AFW_TYPEHANDLING_SIMPLEHETEROMAP_H
#define LSST_AFW_TYPEHANDLING_SIMPLEHETEROMAP_H

#include <exception>
#include <sstream>
#include <unordered_map>
#include <utility>

#include "boost/variant.hpp"

#include "lsst/afw/typehandling/HeteroMap.h"

namespace lsst {
namespace afw {
namespace typehandling {

/**
 * A HeteroMap that allows insertion and deletion of arbitrary values.
 *
 * @tparam K the key type of the map. Must be hashable.
 *
 * @note This class offers no guarantees, such as thread-safety, beyond those
 *       provided by MutableHeteroMap.
 */
template <typename K>
class SimpleHeteroMap final : public MutableHeteroMap<K> {
public:
    SimpleHeteroMap() = default;
    SimpleHeteroMap(SimpleHeteroMap const&) = default;
    SimpleHeteroMap(SimpleHeteroMap&&) = default;
    virtual ~SimpleHeteroMap() = default;

    SimpleHeteroMap& operator=(SimpleHeteroMap const&) = default;
    SimpleHeteroMap& operator=(SimpleHeteroMap&&) = default;

    typename HeteroMap<K>::size_type size() const noexcept override { return _storage.size(); }

    bool empty() const noexcept override { return _storage.empty(); }

    typename HeteroMap<K>::size_type max_size() const noexcept override { return _storage.max_size(); }

    bool contains(K const& key) const override { return _storage.count(key) > 0; }

    std::vector<K> keys() const override {
        std::vector<K> keySnapshot;
        keySnapshot.reserve(_storage.size());
        for (auto const& pair : _storage) {
            keySnapshot.push_back(pair.first);
        }
        return keySnapshot;
    }

    void clear() noexcept override { _storage.clear(); }

protected:
    typename HeteroMap<K>::ValueReference unsafeLookup(K key) const override {
        try {
            // TODO: simplify this code once we're sure it's safe
            StorageType const& value = _storage.at(key);
            typename HeteroMap<K>::ValueReference reference = value;
            return reference;
        } catch (std::out_of_range& e) {
            std::stringstream message;
            message << "Key not found: " << key;
            std::throw_with_nested(LSST_EXCEPT(pex::exceptions::OutOfRangeError, message.str()));
        }
    }

    bool unsafeInsert(K key, typename MutableHeteroMap<K>::InputType&& value) override {
        return _storage.insert(std::make_pair(key, StorageType(std::move(value)))).second;
    }

    bool unsafeErase(K key) override { return _storage.erase(key); }

private:
    // Icky TMP, but I can't find another way to get at the template arguments for variant :(
    // Method has no definition but can't be deleted without breaking definition of StorageType
    /// @cond
    template <typename... Types>
    static boost::variant<std::remove_cv_t<std::remove_reference_t<Types>>...> _variadicTranslator(
            boost::variant<Types...> const&) noexcept;
    /// @endcond

    /**
     * The set of types that may be stored in this map.
     *
     * These are the modifiable value equivalents of HeteroMap<K>::ValueReference.
     */
    // this mouthful is shorter than the equivalent expression with result_of
    using StorageType = decltype(_variadicTranslator(std::declval<typename HeteroMap<K>::ValueReference>()));
    std::unordered_map<K, StorageType> _storage;
};

}  // namespace typehandling
}  // namespace afw
}  // namespace lsst

#endif
