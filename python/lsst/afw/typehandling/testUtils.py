# This file is part of afw.
#
# Developed for the LSST Data Management System.
# This product includes software developed by the LSST Project
# (https://www.lsst.org).
# See the COPYRIGHT file at the top-level directory of this distribution
# for details of code ownership.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

__all__ = []

from frozendict import frozendict

import numpy as np

import lsst.utils.tests
from ._storable import Storable


class HeteroMapTestBaseClass(lsst.utils.tests.TestCase):
    """Base class for unit tests of HeteroMap.

    Subclasses must call `HeteroMapTestBaseClass.setUp(self)`
    if they provide their own version.

    This class is not *quite* a generic Mapping testbed, because it assumes
    that the map being tested only accepts keys of a particular type.
    """

    class SimpleStorable(Storable):
        """A subclass of Storable for testing purposes.
        """
        def __str__(self):
            return "Simplest possible representation"

        def __eq__(self, other):
            """Warning: violates both  substitution and equality symmetry!
            """
            return self.__class__ == other.__class__

    class ComplexStorable(SimpleStorable):
        """A subclass of Storable for testing purposes.
        """
        def __init__(self, storage):
            self._storage = storage

        def __str__(self):
            return "ComplexStorable(" + self._storage + ")"

        def __hash__(self):
            return hash(self._storage)

        def __eq__(self, other):
            """Warning: violates both substitution and equality symmetry!
            """
            if self.__class == other.__class__:
                return self._storage == other._storage
            else:
                return False

    testData = frozendict({
        0: True,
        1: 42,
        2: 42.0,
        3: "How many roads must a man walk down?",
        4: SimpleStorable(),
        5: ComplexStorable(-100.0),
    })
    """Generic dataset for testing HeteroMap classes that can handle it
    """

    def setUp(self):
        """Set up a test

        Subclasses must call this method if they override setUp.
        """
        super().setUp()
        # tell unittest to use the msg argument of asserts as a supplement
        # to the error message, rather than as the whole error message
        self.longMessage = True

    # Mapping must have:
    #   __str__
    #   __repr__
    #   __eq__
    #   __ne__
    #   __contains__
    #   __getitem__
    #   get
    #   __iter__
    #   __len__
    #   __bool__
    #   keys
    #   items
    #   values

    def checkContains(self, heteroMap, contents, msg=""):
        """Check the contents of a HeteroMap.

        Parameters
        ----------
        heteroMap : `lsst.afw.typehandling.HeteroMap`
            The map to test.
        contents : `Mapping`
            The key-value pairs that should be present in ``heteroMap``
        msg : `str`
            Error message suffix describing test parameters
        """
        for key in contents:
            self.assertIn(key, heteroMap, msg=msg)

        keyType = heteroMap.key_type
        for key in (keyType(key) for key in range(30) if keyType(key) not in contents):
            self.assertNotIn(key, heteroMap, msg=msg)

        wrongType = float if keyType is not float else int
        with self.assertRaises(TypeError):
            wrongType(0) in heteroMap

    def checkContents(self, heteroMap, contents, msg=""):
        """Check the contents of a HeteroMap.

        Parameters
        ----------
        heteroMap : `lsst.afw.typehandling.HeteroMap`
            The map to test.
        contents : `Mapping`
            The key-value pairs that should be present in ``heteroMap``
        msg : `str`
            Error message suffix describing test parameters
        """
        for key, value in contents:
            self.assertEqual(heteroMap[key], value, msg=msg)

        keyType = heteroMap.key_type
        for key in (keyType(key) for key in range(30) if keyType(key) not in contents):
            with self.assertRaises(KeyError, msg=msg):
                heteroMap[key]

        wrongType = float if keyType is not float else int
        with self.assertRaises(TypeError):
            heteroMap[wrongType(0)]

    def checkGet(self, heteroMap, contents, msg=""):
        """Check that HeteroMap.get works correctly.

        Parameters
        ----------
        heteroMap : `lsst.afw.typehandling.HeteroMap`
            The map to test.
        contents : `Mapping`
            The key-value pairs that should be present in ``heteroMap``
        msg : `str`
            Error message suffix describing test parameters
        """
        default = "Not a default value"
        for key, value in contents:
            self.assertEqual(heteroMap.get(key), value, msg=msg)
            self.assertEqual(heteroMap.get(key, default), value, msg=msg)

        keyType = heteroMap.key_type
        for key in (keyType(key) for key in range(30) if keyType(key) not in contents):
            self.assertEqual(heteroMap.get(key), None, msg=msg)
            self.assertEqual(heteroMap.get(key, default), default, msg=msg)

        wrongType = float if keyType is not float else int
        with self.assertRaises(TypeError):
            heteroMap.get(wrongType(0))

    def checkIteration(self, heteroMap, contents, msg=""):
        """Check the result of iterating over a HeteroMap.

        Parameters
        ----------
        heteroMap : `lsst.afw.typehandling.HeteroMap`
            The map to test.
        contents : `Mapping`
            The key-value pairs that should be present in ``heteroMap``
        msg : `str`
            Error message suffix describing test parameters
        """
        self.assertEqual({key: heteroMap[key] for key in heteroMap}, dict(contents), msg=msg)

    def checkViews(self, heteroMap, contents, msg=""):
        """Check the views provided by a HeteroMap.

        Parameters
        ----------
        heteroMap : `lsst.afw.typehandling.HeteroMap`
            The map to test.
        contents : `Mapping`
            The key-value pairs that should be present in ``heteroMap``
        msg : `str`
            Error message suffix describing test parameters
        """
        self.assertEqual(set(heteroMap.keys()), set(contents.keys()), msg=msg)
        self.assertEqual(sorted(list(heteroMap.values())), sorted(list(contents.values())), msg=msg)
        self.assertEqual(sorted(list(heteroMap.items())), sorted(list(contents.items())), msg=msg)


class MutableHeteroMapTestBaseClass(HeteroMapTestBaseClass):
    """Base class for unit tests of HeteroMap that allow insertion/deletion.

    Subclasses must call `MutableHeteroMapTestBaseClass.setUp(self)`
    if they provide their own version.
    """

    class NotAStorable:
        """A class that should not be a legal value in a HeteroMap.
        """
        def __str__(self):
            return "Non-Storable"

    @classmethod
    def _fillMap(cls, mapFactory, contents):
        """Create a new HeteroMap with particular contents.

        Parameters
        ----------
        mapFactory : callable
            A zero-argument callable that creates an empty
            `lsst.afw.typehandling.HeteroMap` object
        contents : `Mapping`
            The key-value pairs that should be present in the new map.

        Returns
        -------
        map : `lsst.afw.typehandling.HeteroMap`
            a HeteroMap equivalent to ``contents``
        """
        return cls._fillPartialMap(mapFactory, contents, len(contents))

    @classmethod
    def _fillPartialMap(cls, mapFactory, contents, numElements):
        """Create a new HeteroMap with particular contents.

        Parameters
        ----------
        mapFactory : callable
            A zero-argument callable that creates an empty
            `lsst.afw.typehandling.HeteroMap` object
        contents : `Mapping`
            The key-value pairs that should be present in the new map.
        numElements : `int`
            The number of elements from ``contents`` to be inserted.

        Returns
        -------
        map : `lsst.afw.typehandling.HeteroMap`
            a HeteroMap containing ``numElements`` of ``contents`` or all of
            ``contents``, whichever is smaller
        """
        newMap = mapFactory()
        for i, (key, value) in enumerate(contents.items()):
            if i < numElements:
                newMap.insert(key, value)
            else:
                break
        return newMap

    # MutableMapping must have:
    #   __setitem__
    #   setdefault
    #   __delitem__
    #   pop
    #   popitem
    #   clear
    #   update

    def checkInsertItem(self, mapFactory, contents, msg=""):
        """Check element insertion in a HeteroMap.

        Parameters
        ----------
        mapFactory : callable
            A zero-argument callable that creates an empty
            `lsst.afw.typehandling.HeteroMap` object of the type to be tested
        contents : `Mapping`
            The key-value pairs to insert into the map
        msg : `str`
            Error message suffix describing test parameters
        """
        heteroMap = mapFactory()

        for length, (key, value) in enumerate(contents.items()):
            loopMsg = msg + " Inserting %s=%s" % (key, value)
            heteroMap[key] = value
            self.assertEqual(len(heteroMap), length+1, msg=loopMsg)
            self.assertEqual(heteroMap[key], value, msg=loopMsg)

        self.assertEqual(dict(heteroMap), dict(self.testdata), msg=msg)

        keyType = heteroMap.key_type
        with self.assertRaises(TypeError, msg=msg):
            heteroMap[keyType(0)] = MutableHeteroMapTestBaseClass.NotAStorable()

        wrongType = float if keyType is not float else int
        with self.assertRaises(TypeError):
            heteroMap[wrongType(0)] = 0

    def checkUpdateMapping(self, mapFactory, contents, msg=""):
        """Check bulk insertion from a mapping into a HeteroMap.

        Parameters
        ----------
        mapFactory : callable
            A zero-argument callable that creates an empty
            `lsst.afw.typehandling.HeteroMap` object of the type to be tested
        contents : `Mapping`
            The key-value pairs to insert into the map
        msg : `str`
            Error message suffix describing test parameters
        """
        heteroMap = self._fillMap(mapFactory, dict.fromkeys(contents, 0), len(contents)/2)
        self.assertLess(len(heteroMap), len(contents), msg=msg)

        heteroMap.update(contents)
        self.assertEqual(dict(heteroMap), dict(contents), msg=msg)

        keyType = heteroMap.key_type
        with self.assertRaises(TypeError, msg=msg):
            heteroMap.update({keyType(0): MutableHeteroMapTestBaseClass.NotAStorable()})

        wrongType = float if keyType is not float else int
        with self.assertRaises(TypeError, msg=msg):
            heteroMap.update({wrongType(0): 0})

    def checkUpdatePairs(self, mapFactory, contents, msg=""):
        """Check bulk insertion from an iterable of pairs into a HeteroMap.

        Parameters
        ----------
        mapFactory : callable
            A zero-argument callable that creates an empty
            `lsst.afw.typehandling.HeteroMap` object of the type to be tested
        contents : `Mapping`
            The key-value pairs to insert into the map
        msg : `str`
            Error message suffix describing test parameters
        """
        heteroMap = self._fillMap(mapFactory, dict.fromkeys(contents, 0), len(contents)/2)
        self.assertLess(len(heteroMap), len(contents), msg=msg)

        heteroMap.update(contents.items())
        self.assertEqual(dict(heteroMap), dict(contents), msg=msg)

        keyType = heteroMap.key_type
        with self.assertRaises(TypeError, msg=msg):
            heteroMap.update([(keyType(0), MutableHeteroMapTestBaseClass.NotAStorable())])

        wrongType = float if keyType is not float else int
        with self.assertRaises(TypeError, msg=msg):
            heteroMap.update([(wrongType(0), 0)])

    def checkUpdateKwargs(self, mapFactory, contents, msg=""):
        """Check bulk insertion from keywords into a HeteroMap.

        Parameters
        ----------
        mapFactory : callable
            A zero-argument callable that creates an empty
            `lsst.afw.typehandling.HeteroMap` object of the type to be tested
            Must allow string keys.
        contents : `Mapping`
            The key-value pairs to insert into the map
        msg : `str`
            Error message suffix describing test parameters
        """
        heteroMap = self._fillMap(mapFactory, dict.fromkeys(contents, 0), len(contents)/2)
        self.assertLess(len(heteroMap), len(contents), msg=msg)

        heteroMap.update(**contents)
        self.assertEqual(dict(heteroMap), dict(contents), msg=msg)

        with self.assertRaises(TypeError, msg=msg):
            heteroMap.update(notAKey=MutableHeteroMapTestBaseClass.NotAStorable())

    def checkReplaceItem(self, heteroMap, msg=""):
        """Check element replacement in a HeteroMap.

        Parameters
        ----------
        heteroMap : `lsst.afw.typehandling.HeteroMap`
            The map to test. Must be empty.
        msg : `str`
            Error message suffix describing test parameters
        """
        self.assertFalse(heteroMap, msg=msg)
        key = heteroMap.key_type(42)

        for value in self.testdata.values():
            loopMsg = msg + " Inserting %s=%s" % (key, value)
            heteroMap[key] = value  # value may be of a different type
            self.assertEqual(len(heteroMap), 1, msg=loopMsg)
            self.assertEqual(heteroMap[key], value, msg=loopMsg)

        self.assertEqual(heteroMap, {key, value}, msg=msg)

        with self.assertRaises(TypeError, msg=msg):
            heteroMap[key] = MutableHeteroMapTestBaseClass.NotAStorable()

    def checkRemoveItem(self, mapFactory, contents, msg=""):
        """Check element removal from a HeteroMap.

        Parameters
        ----------
        mapFactory : callable
            A zero-argument callable that creates an empty
            `lsst.afw.typehandling.HeteroMap` object of the type to be tested
        contents : `Mapping`
            The key-value pairs initially occupying the map
        msg : `str`
            Error message suffix describing test parameters
        """
        heteroMap = self._fillMap(mapFactory, contents)
        keyType = heteroMap.key_type

        with self.assertRaises(KeyError, msg=msg):
            del heteroMap[keyType(2019)]

        for numRemoved, rawKey in enumerate(np.random.shuffle(list(contents.keys()))):
            key = keyType(rawKey)
            loopMsg = msg + " Deleting %s" % (key)
            del heteroMap[key]
            self.assertEqual(len(heteroMap), len(contents)-numRemoved-1, msg=loopMsg)
            self.assertNotIn(key, heteroMap, msg=loopMsg)

        wrongType = float if keyType is not float else int
        with self.assertRaises(TypeError):
            del heteroMap[wrongType(0)]

    def checkPop(self, mapFactory, contents, msg=""):
        """Check that HeteroMap.pop works correctly.

        Parameters
        ----------
        mapFactory : callable
            A zero-argument callable that creates an empty
            `lsst.afw.typehandling.HeteroMap` object of the type to be tested
        contents : `Mapping`
            The key-value pairs initially occupying the map
        msg : `str`
            Error message suffix describing test parameters
        """
        heteroMap = self._fillMap(mapFactory, contents)
        keyType = heteroMap.key_type

        with self.assertRaises(KeyError, msg=msg):
            heteroMap.pop(keyType(2019))

        default = "This is a default"
        result = heteroMap.pop(keyType(2019), default)
        self.assertEqual(dict(heteroMap), dict(contents), msg=msg)
        self.assertEqual(result, default)

        wrongType = float if keyType is not float else int
        with self.assertRaises(TypeError):
            heteroMap.pop(wrongType(0))

        for numRemoved, (rawKey, value) in enumerate(np.random.shuffle(list(self.testdata.items()))):
            key = keyType(rawKey)
            loopMsg = msg + " Popping %s=%s" % (key, value)
            result = heteroMap.pop(key)
            self.assertEqual(len(heteroMap), len(self.testdata)-numRemoved-1, msg=loopMsg)
            self.assertNotIn(key, heteroMap, msg=loopMsg)
            self.assertEqual(result, value, msg=loopMsg)

    def checkPopitem(self, mapFactory, contents, msg=""):
        """Check that HeteroMap.popitem works correctly.

        Parameters
        ----------
        mapFactory : callable
            A zero-argument callable that creates an empty
            `lsst.afw.typehandling.HeteroMap` object of the type to be tested
        contents : `Mapping`
            The key-value pairs initially occupying the map
        msg : `str`
            Error message suffix describing test parameters
        """
        heteroMap = self._fillMap(mapFactory, contents)

        for numRemoved in range(1, len(heteroMap)):
            key, value = heteroMap.popitem()
            loopMsg = msg + " Popping %s=%s" % (key, value)
            self.assertIn((key, value), contents, msg=loopMsg)
            self.assertEqual(len(heteroMap), len(self.testdata)-numRemoved, msg=loopMsg)
            self.assertNotIn(key, heteroMap, msg=loopMsg)

        with self.assertRaises(KeyError, msg=msg):
            heteroMap.popitem()

    def checkClear(self, mapFactory, contents, msg=""):
        """Check erasing a HeteroMap.

        Parameters
        ----------
        mapFactory : callable
            A zero-argument callable that creates an empty
            `lsst.afw.typehandling.HeteroMap` object of the type to be tested
        contents : `Mapping`
            The key-value pairs initially occupying the map
        msg : `str`
            Error message suffix describing test parameters
        """
        heteroMap = self._fillMap(mapFactory, contents)
        self.assertTrue(heteroMap, msg=msg)

        heteroMap.clear()
        self.assertFalse(heteroMap, msg=msg)
        self.assertEqual(len(heteroMap), 0, msg=msg)
        for key in heteroMap:
            self.fail("Unexpected key: %s" % key, msg=msg)
