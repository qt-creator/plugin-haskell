TEMPLATE = subdirs

SUBDIRS += \
    plugins/haskell

!isEmpty(BUILD_TESTS): SUBDIRS += tests/auto

