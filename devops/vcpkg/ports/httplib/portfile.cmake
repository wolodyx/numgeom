vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO yhirose/cpp-httplib
    REF v0.48.0
    SHA512 bc9474b0069b84b33f96790d48cee5b52eb1b9555ea6321ce878679c4469f1c56b87b34e74786e47d09cad5a00539cdd6726058bd0ffb2087255f02d7bdabe51
    HEAD_REF master
)

# httplib is a header-only library. Its CMake package name is "httplib"
# (not "cpp-httplib"), so the generated config lives in lib/cmake/httplib.
vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DHTTPLIB_INSTALL=ON
        # Disable automatic detection of optional third-party deps so the
        # default build is self-contained and matches plain header-only usage.
        -DHTTPLIB_USE_OPENSSL_IF_AVAILABLE=OFF
        -DHTTPLIB_USE_ZLIB_IF_AVAILABLE=OFF
        -DHTTPLIB_USE_BROTLI_IF_AVAILABLE=OFF
        -DHTTPLIB_USE_ZSTD_IF_AVAILABLE=OFF
        -DHTTPLIB_USE_WOLFSSL_IF_AVAILABLE=OFF
        -DHTTPLIB_USE_MBEDTLS_IF_AVAILABLE=OFF
        -DHTTPLIB_TEST=OFF
)

vcpkg_cmake_install()
vcpkg_cmake_config_fixup(CONFIG_PATH lib/cmake/httplib)

file(REMOVE_RECURSE
    "${CURRENT_PACKAGES_DIR}/debug/include"
    "${CURRENT_PACKAGES_DIR}/debug/share"
)

file(INSTALL "${SOURCE_PATH}/LICENSE"
     DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}"
     RENAME copyright)
