# Download sonic repository in ~/.sonic/repos/sonic/<tag> and share it across
# projects.

set(SONIC_REPOSITORY_GIT_TAG "v0.1.7")

if(NOT SONIC_REPOSITORY_DIR)
  set(SONIC_REPOSITORY_DIR
      "$ENV{HOME}/.sonic/repos/sonic/${SONIC_REPOSITORY_GIT_TAG}"
      CACHE PATH "" FORCE)
endif()

if(EXISTS "${SONIC_REPOSITORY_DIR}/.git")
  message(STATUS "use cached sonic repo: ${SONIC_REPOSITORY_DIR}")
else()
  include(FetchContent)
  set(FETCHCONTENT_QUIET OFF)
  FetchContent_Declare(
    sonic_repo
    GIT_REPOSITORY https://github.com/yahiro07/sonic.git
    GIT_TAG "${SONIC_REPOSITORY_GIT_TAG}"
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE
    UPDATES_DISCONNECTED TRUE SOURCE_DIR "${SONIC_REPOSITORY_DIR}"
    SOURCE_SUBDIR "")
  FetchContent_MakeAvailable(sonic_repo)
endif()

set(SONIC_ROOT_DIR ${SONIC_REPOSITORY_DIR})
