/*
 * LSST Data Management System
 * Copyright 2008-2016  AURA/LSST.
 *
 * This product includes software developed by the
 * LSST Project (http://www.lsst.org/).
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
 * You should have received a copy of the LSST License Statement and
 * the GNU General Public License along with this program.  If not,
 * see <https://www.lsstcorp.org/LegalNotices/>.
 */

#include "pybind11/pybind11.h"
#include "pybind11/eigen.h"

#include <memory>

#include "ndarray/pybind11.h"

#include "lsst/afw/table/BaseRecord.h"
#include "lsst/afw/table/BaseTable.h"
#include "lsst/afw/table/Simple.h"
#include "lsst/afw/table/Schema.h"
#include "lsst/afw/table/slots.h"
#include "lsst/afw/table/Source.h"
#include "lsst/afw/table/python/catalog.h"
#include "lsst/afw/table/python/columnView.h"
#include "lsst/afw/table/python/sortedCatalog.h"

namespace py = pybind11;
using namespace pybind11::literals;

namespace lsst {
namespace afw {
namespace table {
namespace {

using PySourceRecord = py::class_<SourceRecord, std::shared_ptr<SourceRecord>, SimpleRecord>;
using PySourceTable = py::class_<SourceTable, std::shared_ptr<SourceTable>, SimpleTable>;
using PySourceColumnView =
        py::class_<SourceColumnViewT<SourceRecord>, std::shared_ptr<SourceColumnViewT<SourceRecord>>,
                   ColumnViewT<SourceRecord>>;

/*
Declare member and static functions for a pybind11 wrapper of SourceRecord
*/
PySourceRecord declareSourceRecord(py::module &mod) {
    PySourceRecord cls(mod, "SourceRecord");
    cls.def("getFootprint", &SourceRecord::getFootprint);
    cls.def("setFootprint", &SourceRecord::setFootprint);
    cls.def("getTable", &SourceRecord::getTable);
    cls.def_property_readonly("table", &SourceRecord::getTable);

    cls.def("getParent", &SourceRecord::getParent);
    cls.def("setParent", &SourceRecord::setParent, "id"_a);

    cls.def("getPsfInstFlux", &SourceRecord::getPsfInstFlux);
    cls.def("getPsfInstFluxErr", &SourceRecord::getPsfInstFluxErr);
    cls.def("getPsfFluxFlag", &SourceRecord::getPsfFluxFlag);

    cls.def("getModelInstFlux", &SourceRecord::getModelInstFlux);
    cls.def("getModelInstFluxErr", &SourceRecord::getModelInstFluxErr);
    cls.def("getModelFluxFlag", &SourceRecord::getModelFluxFlag);

    cls.def("getApInstFlux", &SourceRecord::getApInstFlux);
    cls.def("getApInstFluxErr", &SourceRecord::getApInstFluxErr);
    cls.def("getApFluxFlag", &SourceRecord::getApFluxFlag);

    cls.def("getGaussianInstFlux", &SourceRecord::getGaussianInstFlux);
    cls.def("getGaussianInstFluxErr", &SourceRecord::getGaussianInstFluxErr);
    cls.def("getGaussianFluxFlag", &SourceRecord::getGaussianFluxFlag);

    cls.def("getCalibInstFlux", &SourceRecord::getCalibInstFlux);
    cls.def("getCalibInstFluxErr", &SourceRecord::getCalibInstFluxErr);
    cls.def("getCalibFluxFlag", &SourceRecord::getCalibFluxFlag);

    cls.def("getCentroid", &SourceRecord::getCentroid);
    cls.def("getCentroidErr", &SourceRecord::getCentroidErr);
    cls.def("getCentroidFlag", &SourceRecord::getCentroidFlag);

    cls.def("getShape", &SourceRecord::getShape);
    cls.def("getShapeErr", &SourceRecord::getShapeErr);
    cls.def("getShapeFlag", &SourceRecord::getShapeFlag);

    cls.def("getX", &SourceRecord::getX);
    cls.def("getY", &SourceRecord::getY);
    cls.def("getIxx", &SourceRecord::getIxx);
    cls.def("getIyy", &SourceRecord::getIyy);
    cls.def("getIxy", &SourceRecord::getIxy);
    cls.def("updateCoord", (void (SourceRecord::*)(geom::SkyWcs const &)) & SourceRecord::updateCoord,
            "wcs"_a);
    cls.def("updateCoord",
            (void (SourceRecord::*)(geom::SkyWcs const &, PointKey<double> const &)) &
                    SourceRecord::updateCoord,
            "wcs"_a, "key"_a);
    return cls;
}

/*
Declare member and static functions for a pybind11 wrapper of SourceTable
*/
PySourceTable declareSourceTable(py::module &mod) {
    PySourceTable cls(mod, "SourceTable");
    cls.def("clone", &SourceTable::clone);
    cls.def_static("make",
                   (std::shared_ptr<SourceTable>(*)(Schema const &, std::shared_ptr<IdFactory> const &)) &
                           SourceTable::make);
    cls.def_static("make", (std::shared_ptr<SourceTable>(*)(Schema const &)) & SourceTable::make);
    cls.def_static("makeMinimalSchema", &SourceTable::makeMinimalSchema);
    cls.def_static("getParentKey", &SourceTable::getParentKey);
    cls.def("copyRecord",
            (std::shared_ptr<SourceRecord>(SourceTable::*)(BaseRecord const &)) & SourceTable::copyRecord);
    cls.def("copyRecord",
            (std::shared_ptr<SourceRecord>(SourceTable::*)(BaseRecord const &, SchemaMapper const &)) &
                    SourceTable::copyRecord);
    cls.def("makeRecord", &SourceTable::makeRecord);

    cls.def("getPsfFluxSlot", &SourceTable::getPsfFluxSlot);
    cls.def("definePsfFlux", &SourceTable::definePsfFlux, "name"_a);

    cls.def("getModelFluxSlot", &SourceTable::getModelFluxSlot);
    cls.def("defineModelFlux", &SourceTable::defineModelFlux, "name"_a);

    cls.def("getApFluxSlot", &SourceTable::getApFluxSlot);
    cls.def("defineApFlux", &SourceTable::defineApFlux, "name"_a);

    cls.def("getGaussianFluxSlot", &SourceTable::getGaussianFluxSlot);
    cls.def("defineGaussianFlux", &SourceTable::defineGaussianFlux, "name"_a);

    cls.def("getCalibFluxSlot", &SourceTable::getCalibFluxSlot);
    cls.def("defineCalibFlux", &SourceTable::defineCalibFlux, "name"_a);

    cls.def("getCentroidSlot", &SourceTable::getCentroidSlot);
    cls.def("defineCentroid", &SourceTable::defineCentroid, "name"_a);
    cls.def("getCentroidDefinition", &SourceTable::getCentroidDefinition);
    cls.def("hasCentroidSlot", &SourceTable::hasCentroidSlot);
    cls.def("getCentroidKey", &SourceTable::getCentroidKey);
    cls.def("getCentroidErrKey", &SourceTable::getCentroidErrKey);
    cls.def("getCentroidFlagKey", &SourceTable::getCentroidFlagKey);

    cls.def("getShapeSlot", &SourceTable::getShapeSlot);
    cls.def("defineShape", &SourceTable::defineShape, "name"_a);
    cls.def("getShapeDefinition", &SourceTable::getShapeDefinition);
    cls.def("hasShapeSlot", &SourceTable::hasShapeSlot);
    cls.def("getShapeKey", &SourceTable::getShapeKey);
    cls.def("getShapeErrKey", &SourceTable::getShapeErrKey);
    cls.def("getShapeFlagKey", &SourceTable::getShapeFlagKey);

    return cls;
}

PySourceColumnView declareSourceColumnView(py::module &mod) {
    table::python::declareColumnView<SourceRecord>(mod, "Source", true);
    PySourceColumnView cls(mod, "SourceColumnView");
    using SourceColumnView = SourceColumnViewT<SourceRecord>;
    cls.def("getPsfInstFlux", &SourceColumnView::getPsfInstFlux);
    cls.def("getPsfInstFluxErr", &SourceColumnView::getPsfInstFluxErr);
    cls.def("getApInstFlux", &SourceColumnView::getApInstFlux);
    cls.def("getApInstFluxErr", &SourceColumnView::getApInstFluxErr);
    cls.def("getModelInstFlux", &SourceColumnView::getModelInstFlux);
    cls.def("getModelInstFluxErr", &SourceColumnView::getModelInstFluxErr);
    cls.def("getGaussianInstFlux", &SourceColumnView::getGaussianInstFlux);
    cls.def("getGaussianInstFluxErr", &SourceColumnView::getGaussianInstFluxErr);
    cls.def("getCalibInstFlux", &SourceColumnView::getCalibInstFlux);
    cls.def("getCalibInstFluxErr", &SourceColumnView::getCalibInstFluxErr);
    cls.def("getX", &SourceColumnView::getX);
    cls.def("getY", &SourceColumnView::getY);
    cls.def("getIxx", &SourceColumnView::getIxx);
    cls.def("getIyy", &SourceColumnView::getIyy);
    cls.def("getIxy", &SourceColumnView::getIxy);
    return cls;
};

PYBIND11_MODULE(source, mod) {
    py::module::import("lsst.afw.geom.ellipses");
    py::module::import("lsst.afw.table.simple");
    py::module::import("lsst.afw.table.aggregates");
    py::module::import("lsst.afw.table.slots");

    // SourceFitsFlags enum values are used as integer masks, so wrap as attributes instead of an enum
    // static_cast is required to avoid an import error (py::cast and py::int_ do not work by themselves
    // and are not required with the static_cast)
    mod.attr("SOURCE_IO_NO_FOOTPRINTS") = static_cast<int>(SourceFitsFlags::SOURCE_IO_NO_FOOTPRINTS);
    mod.attr("SOURCE_IO_NO_HEAVY_FOOTPRINTS") =
            static_cast<int>(SourceFitsFlags::SOURCE_IO_NO_HEAVY_FOOTPRINTS);

    auto clsSourceRecord = declareSourceRecord(mod);
    auto clsSourceTable = declareSourceTable(mod);
    auto clsSourceColumnView = declareSourceColumnView(mod);
    auto clsSourceCatalog = table::python::declareSortedCatalog<SourceRecord>(mod, "Source");

    clsSourceRecord.attr("Table") = clsSourceTable;
    clsSourceRecord.attr("ColumnView") = clsSourceColumnView;
    clsSourceRecord.attr("Catalog") = clsSourceCatalog;
    clsSourceTable.attr("Record") = clsSourceRecord;
    clsSourceTable.attr("ColumnView") = clsSourceColumnView;
    clsSourceTable.attr("Catalog") = clsSourceCatalog;
    clsSourceCatalog.attr("Record") = clsSourceRecord;
    clsSourceCatalog.attr("Table") = clsSourceTable;
    clsSourceCatalog.attr("ColumnView") = clsSourceColumnView;
}
}  // namespace
}  // namespace table
}  // namespace afw
}  // namespace lsst
