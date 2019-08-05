  set(BUILDROOT_DIR /home/erik/Theaterwagen-br)

  set(BUILD_DIR     ${BUILDROOT_DIR}/output)
  set(STAGING_DIR   ${BUILD_DIR}/staging)
  set(HOST_DIR      ${BUILD_DIR}/host)
  set(TARGET_DIR    ${BUILD_DIR}/target)

  # CMake toolchain

  set(CMAKE_SYSTEM_NAME Linux)
  set(CMAKE_SYSTEM_VERSION 1)

  set(TOOLCHAIN_DIR      ${HOST_DIR}/bin)

#  these are for glibc:
  set(CMAKE_C_COMPILER   ${TOOLCHAIN_DIR}/arm-theaterwagen-linux-gnueabihf-gcc)
  set(CMAKE_CXX_COMPILER ${TOOLCHAIN_DIR}/arm-theaterwagen-linux-gnueabihf-g++)

# these are for uclibc-ng:
#  set(CMAKE_C_COMPILER   ${TOOLCHAIN_DIR}/arm-linux-gcc)
#  set(CMAKE_CXX_COMPILER ${TOOLCHAIN_DIR}/arm-linux-g++)

  set(
    CMAKE_FIND_ROOT_PATH
    ${TARGET_DIR}
    ${TARGET_DIR}/usr/lib
  )

  include_directories(
    ${HOST_DIR}/usr/include
    ${TARGET_DIR}/usr/include
    ${HOST_DIR}/usr/arm-buildroot-linux-gnueabihf/sysroot/usr/include
  )

  set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
  set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
  set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
