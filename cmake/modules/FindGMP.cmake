include(FindPackageHandleStandardArgs)

find_library(
    GMP_C_LIBRARIES
        NAMES gmp
        DOC   "GMP C libraries"
)

find_package_handle_standard_args(
    GMP
	REQUIRED_VARS
        GMP_C_LIBRARIES
)

if (GMP_FOUND)
    add_library(
        GMP::GMP UNKNOWN IMPORTED
    )

    set_target_properties(
        GMP::GMP PROPERTIES
        IMPORTED_LOCATION ${GMP_C_LIBRARIES}
    )
endif()
