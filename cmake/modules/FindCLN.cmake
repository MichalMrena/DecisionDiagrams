include(FindPackageHandleStandardArgs)

find_library(
    CLN_LIBRARY
        NAMES cln
        DOC   "CLN libraries"
)

find_path(
    CLN_INCLUDES
        NAMES cln.h
        DOC   "CLN headers"
        PATH_SUFFIXES cln
)

find_package_handle_standard_args(
    CLN
	REQUIRED_VARS
        CLN_LIBRARY
        CLN_INCLUDES
)

if (CLN_FOUND)
    add_library(
        CLN::CLN UNKNOWN IMPORTED
    )

    set_target_properties(
        CLN::CLN PROPERTIES
        IMPORTED_LOCATION ${CLN_LIBRARY}
    )
endif()