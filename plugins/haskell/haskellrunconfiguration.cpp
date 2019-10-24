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

#include "haskellrunconfiguration.h"

#include "haskellmanager.h"
#include "haskellproject.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/localenvironmentaspect.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/runcontrol.h>
#include <projectexplorer/target.h>

using namespace ProjectExplorer;

namespace Haskell {
namespace Internal {

HaskellRunConfigurationFactory::HaskellRunConfigurationFactory()
{
    registerRunConfiguration<HaskellRunConfiguration>(Constants::C_HASKELL_RUNCONFIG_ID);
    addSupportedProjectType(Constants::C_HASKELL_PROJECT_ID);
    addSupportedTargetDeviceType(ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE);
}

HaskellExecutableAspect::HaskellExecutableAspect()
{
    setSettingsKey("Haskell.Executable");
    setLabelText(tr("Executable"));
}

HaskellRunConfiguration::HaskellRunConfiguration(Target *target, Core::Id id)
    : RunConfiguration(target, id)
{
    addAspect<LocalEnvironmentAspect>(target);

    auto executableAspect = addAspect<HaskellExecutableAspect>();
    connect(target, &Target::applicationTargetsChanged, this, [this, target, executableAspect] {
        BuildTargetInfo bti = target->buildTarget(buildKey());
        executableAspect->setValue(bti.targetFilePath.toString());
    });

    addAspect<ArgumentsAspect>();

    auto workingDirAspect = addAspect<WorkingDirectoryAspect>();
    workingDirAspect->setDefaultWorkingDirectory(target->project()->projectDirectory());
    workingDirAspect->setVisible(false);

    addAspect<TerminalAspect>();
}

void HaskellRunConfiguration::doAdditionalSetup(const RunConfigurationCreationInfo &info)
{
    aspect<HaskellExecutableAspect>()->setValue(info.buildKey);
}

Runnable HaskellRunConfiguration::runnable() const
{
    const QString projectDirectory = target()->project()->projectDirectory().toString();
    Runnable r;
    if (BuildConfiguration *buildConfiguration = target()->activeBuildConfiguration())
        r.commandLineArguments += "--work-dir \""
                                  + QDir(projectDirectory)
                                        .relativeFilePath(
                                            buildConfiguration->buildDirectory().toString())
                                  + "\" ";
    const QString executable = aspect<HaskellExecutableAspect>()->value();
    r.commandLineArguments += "exec \"" + executable + "\"";
    const QString arguments = aspect<ArgumentsAspect>()->arguments(macroExpander());
    if (!arguments.isEmpty())
        r.commandLineArguments += " -- " + arguments;
    r.workingDirectory = projectDirectory;
    r.environment = aspect<LocalEnvironmentAspect>()->environment();
    r.executable = r.environment.searchInPath(HaskellManager::stackExecutable().toString());
    return r;
}

} // namespace Internal
} // namespace Haskell
