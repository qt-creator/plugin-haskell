find_package(QtCreator COMPONENTS Core TextEditor ProjectExplorer REQUIRED)
find_package(Qt5 COMPONENTS Widgets REQUIRED)

add_qtc_plugin(Haskell
  PLUGIN_DEPENDS
    QtCreator::Core QtCreator::TextEditor QtCreator::ProjectExplorer
  DEPENDS Qt5::Widgets
  SOURCES
    filecache.cpp filecache.h
    followsymbol.cpp followsymbol.h
    ghcmod.cpp ghcmod.h
    haskell.qrc
    haskell_global.h
    haskellbuildconfiguration.cpp haskellbuildconfiguration.h
    haskellcompletionassist.cpp haskellcompletionassist.h
    haskellconstants.h
    haskelldocument.cpp haskelldocument.h
    haskelleditorfactory.cpp haskelleditorfactory.h
    haskelleditorwidget.cpp haskelleditorwidget.h
    haskellhighlighter.cpp haskellhighlighter.h
    haskellhoverhandler.cpp haskellhoverhandler.h
    haskellmanager.cpp haskellmanager.h
    haskellplugin.cpp haskellplugin.h
    haskellproject.cpp haskellproject.h
    haskellrunconfiguration.cpp haskellrunconfiguration.h
    haskelltokenizer.cpp haskelltokenizer.h
    optionspage.cpp optionspage.h
    stackbuildstep.cpp stackbuildstep.h
)
