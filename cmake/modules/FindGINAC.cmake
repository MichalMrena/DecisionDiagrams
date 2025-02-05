include(FindPackageHandleStandardArgs)

find_library(
    GINAC_LIBRARY
        NAMES ginac
        DOC   "GiNaC libraries"
)

find_path(
    GINAC_INCLUDES
        NAMES ginac.h
        DOC   "GiNaC headers"
        PATH_SUFFIXES ginac
)

find_package_handle_standard_args(
    GINAC
	REQUIRED_VARS
        GINAC_LIBRARY
        GINAC_INCLUDES
)

if (GINAC_FOUND)
    add_library(
        GINAC::GINAC UNKNOWN IMPORTED
    )

    set_target_properties(
        GINAC::GINAC PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES ${GINAC_INCLUDES}
        IMPORTED_LOCATION             ${GINAC_LIBRARY}
    )
endif()
