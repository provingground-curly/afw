// -*- lsst-c++ -*-
#ifndef LSST_AFW_fits_h_INCLUDED
#define LSST_AFW_fits_h_INCLUDED

#include <string>
#include <vector>

#include <boost/format.hpp>

#include "lsst/pex/exceptions.h"

namespace lsst { namespace afw { namespace fits {

/**
 * @brief An exception thrown when problems are found when reading or writing FITS files.
 */
LSST_EXCEPTION_TYPE(FitsError, lsst::pex::exceptions::Exception, lsst::afw::fits::FitsError)

/**
 * @brief An exception thrown when a FITS file has the wrong type.
 */
LSST_EXCEPTION_TYPE(FitsTypeError, lsst::afw::fits::FitsError, lsst::afw::fits::FitsTypeError)

//@{
/**
 *  @brief Return an error message reflecting FITS I/O errors.
 *
 *  These are intended as replacements for afw::image::cfitsio::err_msg.
 *
 *  @param[in] fileName   FITS filename to be included in the error message.
 *  @param[in] fptr       A cfitsio fitsfile pointer to be inspected for a filename.
 *                        Passed as void* to avoid including fitsio.h in the header file.
 *  @param[in] status     The last status value returned by the cfitsio library; if nonzero,
 *                        the error message will include a description from cfitsio.
 *  @param[in] msg        An additional custom message to include.
 */
std::string makeErrorMessage(std::string const & fileName="", int status=0, std::string const & msg="");
std::string makeErrorMessage(void * fptr, int status=0, std::string const & msg="");
inline std::string makeErrorMessage(std::string const & fileName, int status, boost::format const & fmt) {
    return makeErrorMessage(fileName, status, fmt.str());
}
inline std::string makeErrorMessage(void * fptr, int status, boost::format const & fmt) {
    return makeErrorMessage(fptr, status, fmt.str());
}
//@}

/**
 *  @brief A simple struct that combines the two arguments that must be passed to most cfitsio routines
 *         and contains thin and/or templated wrappers around common cfitsio routines.
 *
 *  This is NOT intended to be an object-oriented C++ wrapper around cfitsio; it's simply a thin layer that
 *  saves a lot of repetition and const/reinterpret casts.
 */
struct Fits {

    Fits & updateKey(char const * key, char const * value, char const * comment=0);

    Fits & writeKey(char const * key, char const * value, char const * comment=0);

    template <typename T>
    Fits & updateKey(char const * key, T value, char const * comment=0);

    template <typename T>
    Fits & writeKey(char const * key, T value, char const * comment=0);

    Fits & createTable(
        long nRows,
        std::vector<std::string> const & ttype,
        std::vector<std::string> const & tform,
        char const * extname = 0
    );

    static Fits createFile(char const * filename);

    static Fits openFile(char const * filename, bool writeable);

    Fits & closeFile();

    void checkStatus() const {
        if (status != 0) throw LSST_EXCEPT(FitsError, makeErrorMessage(fptr, status));
    }

    void * fptr;
    int status;
}; 

}}} /// namespace lsst::afw::fits

#endif // !LSST_AFW_fits_h_INCLUDED
