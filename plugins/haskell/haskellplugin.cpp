/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "haskellplugin.h"

#include "ghcmod.h"
#include "haskellbuildconfiguration.h"
#include "haskellconstants.h"
#include "haskelleditorfactory.h"
#include "haskellmanager.h"
#include "haskellproject.h"
#include "haskellrunconfiguration.h"
#include "optionspage.h"
#include "stackbuildstep.h"

#include <coreplugin/icore.h>
#include <projectexplorer/projectmanager.h>
#include <texteditor/snippets/snippetprovider.h>

namespace Haskell {
namespace Internal {

HaskellPlugin::HaskellPlugin()
{
    // Create your members
}

HaskellPlugin::~HaskellPlugin()
{
    // Unregister objects from the plugin manager's object pool
    // Delete members
}

bool HaskellPlugin::initialize(const QStringList &arguments, QString *errorString)
{
    // Register objects in the plugin manager's object pool
    // Load settings
    // Add actions to menus
    // Connect to other plugins' signals
    // In the initialize function, a plugin can be sure that the plugins it
    // depends on have initialized their members.

    Q_UNUSED(arguments)
    Q_UNUSED(errorString)

    addAutoReleasedObject(new HaskellEditorFactory);
    addAutoReleasedObject(new OptionsPage);
    addAutoReleasedObject(new HaskellBuildConfigurationFactory);
    addAutoReleasedObject(new StackBuildStepFactory);
    addAutoReleasedObject(new HaskellRunConfigurationFactory);
    ProjectExplorer::ProjectManager::registerProjectType<HaskellProject>(
        Constants::C_HASKELL_PROJECT_MIMETYPE);
    TextEditor::SnippetProvider::registerGroup(Constants::C_HASKELLSNIPPETSGROUP_ID,
                                               tr("Haskell", "SnippetProvider"));

    connect(Core::ICore::instance(), &Core::ICore::saveSettingsRequested, this, [] {
        HaskellManager::writeSettings(Core::ICore::settings());
    });
    connect(HaskellManager::instance(), &HaskellManager::stackExecutableChanged,
            &GhcMod::setStackExecutable);

    HaskellManager::readSettings(Core::ICore::settings());
    return true;
}

void HaskellPlugin::extensionsInitialized()
{
    // Retrieve objects from the plugin manager's object pool
    // In the extensionsInitialized function, a plugin can be sure that all
    // plugins that depend on it are completely initialized.
}

ExtensionSystem::IPlugin::ShutdownFlag HaskellPlugin::aboutToShutdown()
{
    // Save settings
    // Disconnect from signals that are not needed during shutdown
    // Hide UI (if you add UI that is not in the main window directly)
    return SynchronousShutdown;
}

} // namespace Internal
} // namespace Haskell
