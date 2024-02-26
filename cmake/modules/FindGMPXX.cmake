include(FindPackageHandleStandardArgs)

find_library(
    GMP_CXX_LIBRARIES
        NAMES gmpxx
        DOC   "GMP C++ libraries"
)

find_path(
    GMP_CXX_INCLUDES
        NAMES gmpxx.h
        DOC   "GMP C++ header"
)

find_package_handle_standard_args(
    GMPXX
	REQUIRED_VARS
        GMP_CXX_LIBRARIES
        GMP_CXX_INCLUDES
)

if (GMPXX_FOUND)
    add_library(
        GMP::GMPXX UNKNOWN IMPORTED
    )

    set_target_properties(
        GMP::GMPXX PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES ${GMP_CXX_INCLUDES}
        IMPORTED_LOCATION             ${GMP_CXX_LIBRARIES}
    )
endif()