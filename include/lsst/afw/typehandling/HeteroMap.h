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

#ifndef LSST_AFW_TYPEHANDLING_HETEROMAP_H
#define LSST_AFW_TYPEHANDLING_HETEROMAP_H

#include <functional>
#include <ostream>
#include <memory>
#include <sstream>
#include <typeinfo>
#include <type_traits>
#include <vector>

#include "boost/core/demangle.hpp"
#include "boost/variant.hpp"

#include "lsst/pex/exceptions.h"
#include "lsst/afw/typehandling/Storable.h"

namespace lsst {
namespace afw {
namespace typehandling {

/**
 * Key for type-safe lookup in a HeteroMap.
 *
 * @tparam K the logical type of the key (e.g., a string)
 * @tparam V the type of the value mapped to this key
 *
 * Key objects are equality-comparable, hashable, sortable, or printable if and only if `K` is comparable,
 * hashable, sortable, or printable, respectively. Key can be used in compile-time expressions if and only
 * if `K` can (in particular, `Key<std::string, V>` cannot).
 *
 * @note Objects of this type are immutable.
 */
template <typename K, typename V>
class Key final {
public:
    using KeyType = K;
    using ValueType = V;

    /**
     * Construct a new key.
     *
     * @param name
     *            the name of the field. For most purposes, this value is
     *            the actual key; it can be retrieved by calling getId().
     *
     * @exceptsafe Provides the same exception safety as the copy-constructor of `K`.
     *
     * @see makeKey
     */
    constexpr Key(K name) : id(name) {}

    Key(Key const&) = default;
    Key(Key&&) = default;
    Key& operator=(Key const&) = delete;
    Key& operator=(Key&&) = delete;

    /**
     * Return the identifier of this field.
     *
     * The identifier serves as the "key" for the map abstraction
     * represented by HeteroMap.
     *
     * @returns the unique key defining this field
     */
    constexpr K const& getId() const noexcept { return id; }

    /**
     * Return the type of field queried by this key.
     *
     * @return a class object for the type of value that may be assigned to
     *         this key.
     */
    // TODO: not necessary in C++?
    constexpr std::type_info getType() const noexcept { return typeid(V); }

    /**
     * Test for key equality.
     *
     * A key is considered equal to another key if and only if their getId() are equal and their value
     * types are exactly the same (including const/volatile qualifications).
     *
     * @{
     */
    constexpr bool operator==(Key<K, V> const& other) const noexcept { return this->id == other.id; }

    template <typename U>
    constexpr std::enable_if_t<!std::is_same<U, V>::value, bool> operator==(Key<K, U> const&) const noexcept {
        return false;
    }

    template <typename U>
    constexpr bool operator!=(Key<K, U> const& other) const noexcept {
        return !(*this == other);
    }

    /** @} */

    /**
     * Define sort order for Keys.
     *
     * This must be expressed as `operator<` instead of std::less because only std::less<void> supports
     * arguments of mixed types, and it cannot be specialized.
     *
     * @param other the key, possibly of a different type, to compare to
     * @return equivalent to `this->getId() < other.getId()`
     *
     * @warning this comparison operator provides a strict weak ordering so long as `K` does, but is *not*
     * consistent with equality. In particular, keys with the same value of `getId()` will be equivalent but
     * may not be equal.
     */
    template <typename U>
    constexpr bool operator<(Key<K, U> const& other) const noexcept {
        const std::less<K> comparator;
        return comparator(this->getId(), other.getId());
    }

    /// Return a hash of this object.
    std::size_t hash_value() const noexcept { return std::hash<K>()(id); }

private:
    /** The logical key. */
    K const id;
};

/**
 * Factory function for Key, to enable type parameter inference.
 *
 * @param name the key ID to create.
 *
 * @returns a key of the desired type
 *
 * @exceptsafe Provides the same exception safety as the copy-constructor of `K`.
 *
 * @relatesalso Key
 *
 * Calling this function prevents you from having to explicitly name the key type:
 *
 *     auto key = makeKey<int>("foo");
 */
// template parameters must be reversed for inference to work correctly
template <typename V, typename K>
constexpr Key<K, V> makeKey(K name) {
    return Key<K, V>(name);
}

/**
 * Output operator for Key.
 *
 * The output will use C++ template notation for the key; for example, a key "foo" pointing to an `int` may
 * print as `"foo<int>"`.
 *
 * @param os the desired output stream
 * @param key the key to print
 *
 * @returns a reference to `os`
 *
 * @exceptsafe Provides basic exception safety if the output operator of `K` is exception-safe.
 *
 * @warning the type name is compiler-specific and may be mangled or unintuitive; for example, some compilers
 * say "i" instead of "int"
 *
 * @relatesalso Key
 */
template <typename K, typename V>
std::ostream& operator<<(std::ostream& os, Key<K, V> const& key) {
    static const std::string typeStr = boost::core::demangle(typeid(V).name());
    static std::string constStr = std::is_const<V>::value ? " const" : "";
    static std::string volatileStr = std::is_volatile<V>::value ? " volatile" : "";
    os << key.getId() << "<" << typeStr << constStr << volatileStr << ">";
    return os;
}

// Test for smart pointers as "any type with an element_type member"
// Second template parameter is a dummy to let us do some metaprogramming
template <typename, typename = void>
constexpr bool IS_SMART_PTR = false;
template <typename T>
constexpr bool IS_SMART_PTR<T, std::enable_if_t<std::is_object<typename T::element_type>::value>> = true;

/**
 * Interface for a heterogeneous map.
 *
 * Objects of type HeteroMap cannot necessarily have keys added or removed, although mutable values can be
 * modified as usual. See MutableHeteroMap for a HeteroMap that must allow insertions and deletions.
 *
 * @tparam K the key type of the map.
 *
 * A Key for the map is parameterized by both the key type `K` and a corresponding value type `V`. The map
 * is indexed uniquely by a value of type `K`; no two entries in the map may have identical values of
 * Key::getId().
 *
 * All operations are sensitive to the value type of the key: a @ref contains(Key<K,T> const&) const
 * "contains" call requesting an integer labeled "value", for example, will report no such integer if instead
 * there is a string labeled "value". For Python compatibility, a HeteroMap does not store type information
 * internally, instead relying on RTTI for type checking.
 *
 * All subclasses **must** guarantee, as a class invariant, that every value in the map is implicitly
 * nothrow-convertible to the type indicated by its key. For example, MutableHeteroMap ensures this by
 * appropriately templating all operations that create new key-value pairs.
 *
 * A HeteroMap may contain primitive types, strings, Storable, and shared pointers to Storable as
 * values. It does not support unique pointers to Storable because such pointers are read destructively. For
 * safety reasons, it may not contain references, C-style pointers, or arrays to any type. Due to
 * implementation restrictions, `const` types (particularly pointers to `const` Storable) are not
 * currently supported.
 */
// TODO: const keys should be possible in C++17 with std::variant
template <typename K>
class HeteroMap {
public:
    using key_type = K;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;

    virtual ~HeteroMap() = default;

    /**
     * Return a reference to the mapped value of the element with key equal to `key`.
     *
     * @tparam T the type of the element mapped to `key`
     * @param key the key of the element to find
     *
     * @return a reference to the `T` mapped to `key`, if one exists
     *
     * @throws pex::exceptions::OutOfRangeError Thrown if the map does not
     *         have a `T` with the specified key
     * @exceptsafe Provides strong exception safety.
     *
     * @note This implementation calls @ref unsafeLookup once, then uses templates
     *       and RTTI to determine if the value is of the expected type.
     *
     * @{
     */
    template <typename T, typename std::enable_if_t<!IS_SMART_PTR<T>, int> = 0>
    T& at(Key<K, T> const& key) {
        // Both casts are safe; see Effective C++, Item 3
        return const_cast<T&>(static_cast<const HeteroMap&>(*this).at(key));
    }

    // Can't partially specialize method templates, rely on enable_if to avoid duplicates
    template <typename T,
              typename std::enable_if_t<
                      std::is_fundamental<T>::value || std::is_base_of<std::string, T>::value, int> = 0>
    T const& at(Key<K, T> const& key) const {
        static_assert(!std::is_const<T>::value,
                      "Due to implementation constraints, const keys are not supported.");
        try {
            auto foo = unsafeLookup(key.getId());
            return boost::get<T const&>(foo);
        } catch (boost::bad_get const&) {
            std::stringstream message;
            message << "Key not found: " << key;
            throw LSST_EXCEPT(pex::exceptions::OutOfRangeError, message.str());
        }
    }

    template <typename T, typename std::enable_if_t<std::is_base_of<Storable, T>::value, int> = 0>
    T const& at(Key<K, T> const& key) const {
        static_assert(!std::is_const<T>::value,
                      "Due to implementation constraints, const keys are not supported.");
        try {
            auto foo = unsafeLookup(key.getId());
            // Don't use pointer-based get, because it won't work after migrating to std::variant
            // return unique_ptr by reference, so the pointer in internal storage won't get cleared
            auto& holder = boost::get<std::unique_ptr<Storable> const&>(foo);
            T* typedPointer = dynamic_cast<T*>(holder.get());
            if (typedPointer != nullptr) {
                return *typedPointer;
            } else {
                std::stringstream message;
                message << "Key not found: " << key;
                throw LSST_EXCEPT(pex::exceptions::OutOfRangeError, message.str());
            }
        } catch (boost::bad_get const&) {
            std::stringstream message;
            message << "Key not found: " << key;
            throw LSST_EXCEPT(pex::exceptions::OutOfRangeError, message.str());
        }
    }

    template <typename T, typename std::enable_if_t<std::is_base_of<Storable, T>::value, int> = 0>
    std::shared_ptr<T> at(Key<K, std::shared_ptr<T>> const& key) const {
        static_assert(!std::is_const<T>::value,
                      "Due to implementation constraints, const keys are not supported.");
        try {
            auto foo = unsafeLookup(key.getId());
            auto pointer = boost::get<std::shared_ptr<Storable> const&>(foo);
            std::shared_ptr<T> typedPointer = std::dynamic_pointer_cast<T>(pointer);
            if (typedPointer.use_count() > 0) {
                return typedPointer;
            } else {
                std::stringstream message;
                message << "Key not found: " << key;
                throw LSST_EXCEPT(pex::exceptions::OutOfRangeError, message.str());
            }
        } catch (boost::bad_get const&) {
            std::stringstream message;
            message << "Key not found: " << key;
            throw LSST_EXCEPT(pex::exceptions::OutOfRangeError, message.str());
        }
    }

    /** @} */

    /// Return the number of key-value pairs in the map.
    virtual size_type size() const noexcept = 0;

    /// Return `true` if this map contains no key-value pairs.
    virtual bool empty() const noexcept = 0;

    /**
     * Return the maximum number of elements the container is able to hold due to system or library
     * implementation limitations.
     *
     * @note This value typically reflects the theoretical limit on the size of the container. At runtime, the
     * size of the container may be limited to a value smaller than max_size() by the amount of RAM available.
     */
    virtual size_type max_size() const noexcept = 0;

    /**
     * Return the number of elements mapped to the specified key.
     *
     * @tparam T the value corresponding to `key`
     * @param key key value of the elements to count
     *
     * @return number of `T` with key `key`, that is, either 1 or 0.
     *
     * @exceptsafe Provides strong exception safety.
     *
     * @note This implementation calls @ref contains(Key<K,T> const&) const "contains".
     *
     */
    template <typename T>
    size_type count(Key<K, T> const& key) const {
        return contains(key) ? 1 : 0;
    }

    /**
     * Return `true` if this map contains a mapping whose key has the specified label.
     *
     * More formally, this method returns `true` if and only if this map contains a mapping with a key `k`
     * such that `k.getId() == key`. There can be at most one such mapping.
     *
     * @param key the weakly-typed key to search for
     *
     * @return `true` if this map contains a mapping for `key`, regardless of value type.
     *
     * @exceptsafe Provides strong exception safety.
     */
    virtual bool contains(K const& key) const = 0;

    /**
     * Return `true` if this map contains a mapping for the specified key.
     *
     * This is equivalent to testing whether `at(key)` would succeed.
     *
     * @tparam T the value corresponding to `key`
     * @param key the key to search for
     *
     * @return `true` if this map contains a mapping from the specified key to a `T`
     *
     * @exceptsafe Provides strong exception safety.
     *
     * @note This implementation calls contains(K const&) const. If the call returns
     *       `true`, it calls @ref unsafeLookup, then uses templates and RTTI to
     *       determine if the value is of the expected type. The performance of
     *       this method depends strongly on the performance of
     *       contains(K const&).
     *
     * @{
     */
    // Can't partially specialize method templates, rely on enable_if to avoid duplicates
    template <typename T,
              typename std::enable_if_t<
                      std::is_fundamental<T>::value || std::is_base_of<std::string, T>::value, int> = 0>
    bool contains(Key<K, T> const& key) const {
        // Avoid actually getting and casting an object, if at all possible
        if (!contains(key.getId())) {
            return false;
        }

        auto foo = unsafeLookup(key.getId());
        // boost::variant has no equivalent to std::holds_alternative
        try {
            boost::get<T const&>(foo);
            return true;
        } catch (boost::bad_get const&) {
            return false;
        }
    }

    template <typename T, typename std::enable_if_t<std::is_base_of<Storable, T>::value, int> = 0>
    bool contains(Key<K, T> const& key) const {
        // Avoid actually getting and casting an object, if at all possible
        if (!contains(key.getId())) {
            return false;
        }

        auto foo = unsafeLookup(key.getId());
        try {
            // Don't use pointer-based get, because it won't work after migrating to std::variant
            // return unique_ptr by reference, so the pointer in internal storage won't get cleared
            auto& holder = boost::get<std::unique_ptr<Storable> const&>(foo);
            return dynamic_cast<T*>(holder.get()) != nullptr;
        } catch (boost::bad_get const&) {
            return false;
        }
    }

    template <typename T, typename std::enable_if_t<std::is_base_of<Storable, T>::value, int> = 0>
    bool contains(Key<K, std::shared_ptr<T>> const& key) const {
        // Avoid actually getting and casting an object, if at all possible
        if (!contains(key.getId())) {
            return false;
        }

        auto foo = unsafeLookup(key.getId());
        try {
            auto pointer = boost::get<std::shared_ptr<Storable> const&>(foo);
            std::shared_ptr<T> typedPointer = std::dynamic_pointer_cast<T>(pointer);
            return typedPointer.use_count() > 0;
        } catch (boost::bad_get const&) {
            return false;
        }
    }

    /** @} */

    // TODO: still need an API to support RTTI queries from Python

    /**
     * Return the set of all keys, without type information.
     *
     * @return a copy of all keys currently in the map, in the same iteration order as this object. The set
     * will *not* be updated as this object changes, or vice versa.
     *
     * @note The keys are returned as a list, rather than a set, so that subclasses can give them a
     * well-defined iteration order.
     *
     * @exceptsafe Provides strong exception safety.
     */
    virtual std::vector<K> keys() const = 0;

protected:
    /**
     * The set of legal return types for @ref unsafeLookup.
     *
     * Keys of any subclass of Storable are implemented using `unique_ptr<Storable>` to preserve type.
     */
    // may need to use std::reference_wrapper when migrating to std::variant, but it confuses Boost
    using ValueReference =
            boost::variant<bool const&, int const&, float const&, double const&, std::string const&,
                           std::unique_ptr<Storable> const&, std::shared_ptr<Storable> const&>;

    /**
     * Return a reference to the mapped value of the element with key equal to `key`.
     *
     * This method is the primary way to implement the HeteroMap interface.
     *
     * @param key the key of the element to find
     *
     * @return the value mapped to `key`, if one exists
     *
     * @throws pex::exceptions::OutOfRangeError Thrown if the map does not have
     *         a value with the specified key
     * @exceptsafe Must provide strong exception safety.
     */
    virtual ValueReference unsafeLookup(K key) const = 0;
};

/**
 * Interface for a HeteroMap that allows element addition and removal.
 *
 * @note Unlike standard library maps, this class does not support `operator[]` or `insert_or_assign`. This is
 * because these operations would have surprising behavior when dealing with keys of different types but the
 * same Key::getId().
 *
 */
template <typename K>
class MutableHeteroMap : public HeteroMap<K> {
public:
    virtual ~MutableHeteroMap() = default;

    /**
     * Remove all of the mappings from this map.
     *
     * After this call, the map will be empty.
     */
    virtual void clear() noexcept = 0;

    /**
     * Insert an element into the map, if the map doesn't already contain a mapping with the same or a
     * conflicting key.
     *
     * @tparam T the type of value to insert
     * @param key key to insert
     * @param value value to insert
     *
     * @return `true` if the insertion took place, `false` otherwise
     *
     * @exceptsafe Provides strong exception safety.
     *
     * @note It is possible for a key with a value type other than `T` to prevent insertion. Callers can
     * safely assume `this->contains(key.getId())` as a postcondition, but not `this->contains(key)`.
     *
     * @note This implementation calls @ref contains(K const&) const "contains",
     *       then calls @ref unsafeInsert if there is no conflicting key.
     *
     * @{
     */
    // Can't partially specialize method templates, rely on enable_if to avoid duplicates
    template <typename T, typename std::enable_if_t<!std::is_base_of<Storable, T>::value, int> = 0>
    bool insert(Key<K, T> const& key, T const& value) {
        if (this->contains(key.getId())) {
            return false;
        }

        return unsafeInsert(key.getId(), InputType(value));
    }

    template <typename T, typename std::enable_if_t<std::is_base_of<Storable, T>::value, int> = 0>
    bool insert(Key<K, T> const& key, T const& value) {
        if (this->contains(key.getId())) {
            return false;
        }

        auto holder = value.clone();
        return unsafeInsert(key.getId(), InputType(std::move(holder)));
    }

    /** @} */

    /**
     * Remove the mapping for a key from this map, if it exists.
     *
     * @tparam T the type of value the key maps to
     * @param key the key to remove
     *
     * @return `true` if `key` was removed, `false` if it was not present
     *
     * @exceptsafe Provides strong exception safety.
     *
     * @note This implementation calls @ref contains(Key<K,T> const&) const "contains",
     *       then calls @ref unsafeErase if the key is present.
     */
    template <typename T>
    bool erase(Key<K, T> const& key) {
        if (this->contains(key)) {
            return unsafeErase(key.getId());
        } else {
            return false;
        }
    }

private:
    // Icky TMP, but I can't find another way to get at the template arguments for variant :(
    // Method has no definition but can't be deleted without breaking definition of InputType
    /// @cond
    template <typename... Types>
    static boost::variant<std::decay_t<Types>...> _variadicTranslator(
            boost::variant<Types...> const&) noexcept;
    /// @endcond

protected:
    /**
     * The set of legal input types for unsafeInsert.
     *
     * These are the input equivalents (using std::decay) of HeteroMap<K>::ValueReference.
     */
    // this mouthful is shorter than the equivalent expression with result_of
    using InputType = decltype(_variadicTranslator(std::declval<typename HeteroMap<K>::ValueReference>()));

    /**
     * Create a new mapping with key equal to `key` and value equal to `value`.
     *
     * This method is the primary way to implement the MutableHeteroMap interface.
     *
     * @param key the key of the element to insert. The method may assume that the map does not contain `key`.
     * @param value a reference to the value to insert.
     *
     * @return `true` if the insertion took place, `false` otherwise
     *
     * @exceptsafe Must provide strong exception safety.
     */
    virtual bool unsafeInsert(K key, InputType&& value) = 0;

    /**
     * Remove the mapping for a key from this map, if it exists.
     *
     * @param key the key to remove
     *
     * @return `true` if `key` was removed, `false` if it was not present
     *
     * @exceptsafe Must provide strong exception safety.
     */
    virtual bool unsafeErase(K key) = 0;
};

}  // namespace typehandling
}  // namespace afw
}  // namespace lsst

namespace std {
template <typename K, typename V>
struct hash<typename lsst::afw::typehandling::Key<K, V>> {
    using argument_type = typename lsst::afw::typehandling::Key<K, V>;
    using result_type = size_t;
    size_t operator()(argument_type const& obj) const noexcept { return obj.hash_value(); }
};
}  // namespace std

#endif
