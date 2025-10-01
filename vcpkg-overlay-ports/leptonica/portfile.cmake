vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO DanBloomberg/leptonica
    REF "${VERSION}"
    SHA512 49e387eae37fda02242ff093c6effa92f59e0761640c71a5c79f0c02923486dc96472ff99a17763cbecc6396966cbc5c0d7f5c8fd3a61f9a65a34339f930735a
    HEAD_REF master
    PATCHES
        fix-pc-and-config.patch
)

vcpkg_find_acquire_program(PKGCONFIG)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DSW_BUILD=OFF
        "-DPKG_CONFIG_EXECUTABLE=${PKGCONFIG}"
        -DENABLE_ZLIB=OFF
        -DENABLE_PNG=OFF
        -DENABLE_GIF=OFF
        -DENABLE_JPEG=OFF
        -DENABLE_TIFF=OFF
        -DENABLE_WEBP=OFF
        -DENABLE_OPENJPEG=OFF
)

vcpkg_cmake_install()

vcpkg_fixup_pkgconfig()

vcpkg_cmake_config_fixup(CONFIG_PATH lib/cmake/leptonica)

vcpkg_copy_pdbs()

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/leptonica-license.txt")
