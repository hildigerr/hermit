SET(PO_FILES
  ca.po
  da.po
  de.po
  es.po
  fr.po
  hu.po
  ja.po
  ko.po
  nl.po
  pl.po
  ru.po
  sv.po
  zh_TW.po)

SET(COPYRIGHT_HOLDER "Evgeny Ratnikov")
SET(POT_FILE termit.pot)

INCLUDE(${TERMIT_SOURCE_DIR}/src/Sources.cmake)

FIND_PROGRAM(XGETTEXT_EXECUTABLE xgettext)
FIND_PROGRAM(MSGFMT_EXECUTABLE msgfmt)
FIND_PROGRAM(MSGMERGE_EXECUTABLE msgmerge)
SET(ENABLE_NLS FALSE)
IF(XGETTEXT_EXECUTABLE AND MSGFMT_EXECUTABLE AND MSGMERGE_EXECUTABLE)
  SET(ENABLE_NLS TRUE)
ENDIF(XGETTEXT_EXECUTABLE AND MSGFMT_EXECUTABLE AND MSGMERGE_EXECUTABLE)
IF(NOT ENABLE_NLS)
  MESSAGE(SEND_ERROR "gettext utils not found, disabling gettext")
ELSE(NOT ENABLE_NLS)
  # Create PO Template file 'termit.pot'
  SET(PO_SOURCE_FILES)
  SET(PO_SOURCE_FILES_FULL_PATH)
  FOREACH(SRC_FILE ${TERMIT_SRCS})
    SET(PO_SOURCE_FILES ${PO_SOURCE_FILES} src/${SRC_FILE})
    SET(PO_SOURCE_FILES_FULL_PATH ${PO_SOURCE_FILES_FULL_PATH} ${TERMIT_SOURCE_DIR}/src/${SRC_FILE})
  ENDFOREACH(SRC_FILE ${TERMIT_SRCS})

  ADD_CUSTOM_COMMAND(
    OUTPUT ${POT_FILE}
    COMMAND ${XGETTEXT_EXECUTABLE}
    ARGS -o ${POT_FILE} -s --directory=${TERMIT_SOURCE_DIR} --omit-header --msgid-bugs-address=${TERMIT_PACKAGE_BUGREPORT} -c --keyword=_ --copyright-holder=${COPYRIGHT_HOLDER} --no-location ${PO_SOURCE_FILES}
    DEPENDS ${PO_SOURCE_FILES_FULL_PATH})
  ADD_CUSTOM_TARGET(pot DEPENDS ${PO_SOURCE_FILES_FULL_PATH} ${POT_FILE})

  # Update .po files and compile them into .gmo files
  SET(GMO_FILES)
  FOREACH(PO_FILE ${PO_FILES})
    GET_FILENAME_COMPONENT(LANG ${PO_FILE} NAME_WE)
    SET(_gmofile ${CMAKE_CURRENT_BINARY_DIR}/${LANG}.gmo)
    ADD_CUSTOM_COMMAND(
      OUTPUT ${_gmofile}
      COMMAND ${MSGMERGE_EXECUTABLE} ARGS --quiet --update --no-location -s ${CMAKE_CURRENT_SOURCE_DIR}/${PO_FILE} ${POT_FILE}
      COMMAND ${MSGFMT_EXECUTABLE} ARGS -o ${_gmofile} ${CMAKE_CURRENT_SOURCE_DIR}/${PO_FILE}
      DEPENDS ${POT_FILE} ${CMAKE_CURRENT_SOURCE_DIR}/${PO_FILE}
      COMMENT "Language: ${LANG}")
    INSTALL(FILES ${_gmofile} DESTINATION ${TERMIT_LOCALE_DIR}/${LANG}/LC_MESSAGES RENAME termit.mo)
    SET(GMO_FILES ${GMO_FILES} ${_gmofile})
  ENDFOREACH(PO_FILE ${PO_FILES})

  ADD_CUSTOM_TARGET(gmo ALL DEPENDS ${GMO_FILES})
  ADD_DEPENDENCIES(gmo pot)

  SET(TERMIT_DISTR_FILES "${POT_FILE} ${PO_FILES} CMakeLists.txt")
ENDIF(NOT ENABLE_NLS)

