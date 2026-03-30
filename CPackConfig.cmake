set(CPACK_GENERATOR "ZIP")
if(WIN32)
  list(APPEND CPACK_GENERATOR "NSIS")
elseif(UNIX AND NOT APPLE)
  list(APPEND CPACK_GENERATOR "DEB")
endif()

set(CPACK_PACKAGE_NAME "numgeom")
set(CPACK_PACKAGE_VENDOR "numgeom.ru")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Numgeom - Modern CAx Framework")
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_PACKAGE_INSTALL_DIRECTORY "numgeom")
set(CPACK_STRIP_FILES ON)

if(CPACK_GENERATOR MATCHES "NSIS")
  set(CPACK_NSIS_MODIFY_PATH ON)
  set(CPACK_NSIS_DEFINES "RequestExecutionLevel user")
  set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS
    "  CreateShortCut '$DESKTOP\\\\numgeom.lnk' '$INSTDIR\\\\bin\\\\app-qt.exe' "
  )
endif()

if(CPACK_GENERATOR MATCHES "DEB")
  set(CPACK_DEBIAN_PACKAGE_MAINTAINER "https://github.com/wolodyx")
  set(CPACK_DEBIAN_PACKAGE_DESCRIPTION [[
NumGeom -- это кроссплатформенный фреймворк, объединяющий передовые технологии
3D-моделирования в единой среде. Проект предоставляет готовую инфраструктуру для
создания интерактивных приложений инженерного анализа, автоматизированного
проектирования и подготовки производства.
  ]])
  set(CPACK_DEBIAN_PACKAGE_SECTION "devel")
  set(CPACK_DEBIAN_FILE_NAME "DEB-DEFAULT")
  set(CPACK_PACKAGING_INSTALL_PREFIX "/opt/${PROJECT_NAME}")
endif()
