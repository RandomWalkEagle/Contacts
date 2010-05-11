TARGET_LOADER            = virtus
TARGET_UTILS             = virtusutils

macx:CONFIG             += x86

#Configurable values
isEmpty(INSTALL_PREFIX) {
  win32:INSTALL_PREFIX   = "c:"
  unix: INSTALL_PREFIX   = /usr/local
  macx: INSTALL_PREFIX   = /Applications
}

isEmpty(INSTALL_APP_DIR) {
  win32:INSTALL_APP_DIR  = virtus
  unix: INSTALL_APP_DIR  = virtus
  macx: INSTALL_APP_DIR  = virtus.app
}

isEmpty(INSTALL_LIB_DIR) {
  win32:INSTALL_LIB_DIR  = .
  unix:!macx {
    UNAME                = $$system(uname -m)
    contains( UNAME, x86_64 ): LIB_SUFFIX = 64
    INSTALL_LIB_DIR      = lib$${LIB_SUFFIX}
  }
  macx: INSTALL_LIB_DIR  = Frameworks
}

isEmpty(INSTALL_RES_DIR) {
  win32:INSTALL_RES_DIR  = .
  unix: INSTALL_RES_DIR  = share
  macx: INSTALL_RES_DIR  = Resources
}


#Preprocessor definations
CONFIG(debug, debug|release) {
  DEFINES               += PLUGINS_DIR='\\\"./plugins\\\"'
  DEFINES               += RESOURCES_DIR='\\\"./resources\\\"'
  DEFINES               += TRANSLATIONS_DIR='\\\"./translations\\\"'
} else:win32 {
  DEFINES               += PLUGINS_DIR='\\\"./$${INSTALL_LIB_DIR}/plugins\\\"'
  DEFINES               += RESOURCES_DIR='\\\"./$${INSTALL_RES_DIR}/resources\\\"'
  DEFINES               += TRANSLATIONS_DIR='\\\"./$${INSTALL_RES_DIR}/translations\\\"'
} else:unix:!macx {
  DEFINES               += PLUGINS_DIR='\\\"../$${INSTALL_LIB_DIR}/$${INSTALL_APP_DIR}/plugins\\\"'
  DEFINES               += RESOURCES_DIR='\\\"../$${INSTALL_RES_DIR}/$${INSTALL_APP_DIR}/resources\\\"'
  DEFINES               += TRANSLATIONS_DIR='\\\"../$${INSTALL_RES_DIR}/$${INSTALL_APP_DIR}/translations\\\"'
} else:macx {
  DEFINES               += PLUGINS_DIR='\\\"../PlugIns\\\"'
  DEFINES               += RESOURCES_DIR='\\\"../$${INSTALL_RES_DIR}\\\"'
  DEFINES               += TRANSLATIONS_DIR='\\\"../$${INSTALL_RES_DIR}/translations\\\"'
}