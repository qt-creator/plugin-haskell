find_package(QtCreator COMPONENTS Core TextEditor ProjectExplorer REQUIRED)
find_package(Qt5 COMPONENTS Widgets REQUIRED)

add_qtc_plugin(Haskell
  PLUGIN_DEPENDS
    QtCreator::Core QtCreator::TextEditor QtCreator::ProjectExplorer
  DEPENDS Qt5::Widgets
  SOURCES
    haskell.qrc
    haskell_global.h
    haskellbuildconfiguration.cpp haskellbuildconfiguration.h
    haskellconstants.h
    haskelleditorfactory.cpp haskelleditorfactory.h
    haskellhighlighter.cpp haskellhighlighter.h
    haskellmanager.cpp haskellmanager.h
    haskellplugin.cpp haskellplugin.h
    haskellproject.cpp haskellproject.h
    haskellrunconfiguration.cpp haskellrunconfiguration.h
    haskelltokenizer.cpp haskelltokenizer.h
    optionspage.cpp optionspage.h
    stackbuildstep.cpp stackbuildstep.h
)
