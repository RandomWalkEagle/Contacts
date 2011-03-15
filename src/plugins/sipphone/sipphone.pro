
LIBS  += -L../../thirdparty/siplibraries/SipLib/lib
LIBS  += -L../../thirdparty/siplibraries/VoIPMediaLib/lib
LIBS  += -L../../thirdparty/siplibraries/VoIPVideoLib/lib
LIBS  += -lWs2_32 -lVoIPVideoLib -lVoIPMedia -lSipProtocol
INCLUDEPATH += ../../thirdparty/siplibraries/SipLib/inc ../../thirdparty/siplibraries/VoIPMediaLib/inc ../../thirdparty/siplibraries/VoIPVideoLib/inc
INCLUDEPATH += ../../thirdparty/siplibraries/SPEEX/include
INCLUDEPATH += ../../thirdparty/siplibraries/VoIPMediaLib/Inc/iLBC

LIBS += -lole32 -loleaut32 -luuid -lodbc32 -lodbccp32

QT    += multimedia
TARGET = sipphone
include(sipphone.pri)
include(../plugins.inc)
