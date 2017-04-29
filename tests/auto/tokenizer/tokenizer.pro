include(../../../plugins/haskell/config.pri)

include($$IDE_SOURCE_TREE/tests/auto/qttest.pri)

SOURCES += tst_tokenizer.cpp \
    $$PWD/../../../plugins/haskell/haskelltokenizer.cpp

HEADERS += \
    $$PWD/../../../plugins/haskell/haskelltokenizer.h

INCLUDEPATH += $$PWD/../../../plugins/haskell
