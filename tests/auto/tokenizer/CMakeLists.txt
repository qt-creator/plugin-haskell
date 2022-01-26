add_qtc_test(tst_tokenizer
  DEPENDS Qt5::Core Qt5::Test
  INCLUDES ../../../plugins/haskell
  SOURCES
    tst_tokenizer.cpp
    ../../../plugins/haskell/haskelltokenizer.cpp
    ../../../plugins/haskell/haskelltokenizer.h
)
