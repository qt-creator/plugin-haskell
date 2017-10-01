import qbs 1.0

QtcPlugin {
    name: "Haskell"

    Depends { name: "Qt.widgets" }
    Depends { name: "Utils" }

    Depends { name: "Core" }
    Depends { name: "TextEditor" }
    Depends { name: "ProjectExplorer" }

    files: [
        "filecache.cpp", "filecache.h",
        "followsymbol.cpp", "followsymbol.h",
        "ghcmod.cpp", "ghcmod.h",
        "haskell.qrc",
        "haskellbuildconfiguration.cpp", "haskellbuildconfiguration.h",
        "haskellcompletionassist.cpp", "haskellcompletionassist.h",
        "haskellconstants.h",
        "haskelldocument.cpp", "haskelldocument.h",
        "haskelleditorfactory.cpp", "haskelleditorfactory.h",
        "haskelleditorwidget.cpp", "haskelleditorwidget.h",
        "haskell_global.h",
        "haskellhighlighter.cpp", "haskellhighlighter.h",
        "haskellhoverhandler.cpp", "haskellhoverhandler.h",
        "haskellmanager.cpp", "haskellmanager.h",
        "haskellplugin.cpp", "haskellplugin.h",
        "haskellproject.cpp", "haskellproject.h",
        "haskellrunconfiguration.cpp", "haskellrunconfiguration.h",
        "haskelltokenizer.cpp", "haskelltokenizer.h",
        "optionspage.cpp", "optionspage.h",
        "stackbuildstep.cpp", "stackbuildstep.h"
    ]
}
