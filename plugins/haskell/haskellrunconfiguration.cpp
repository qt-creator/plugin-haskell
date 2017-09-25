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

#include "haskellconstants.h"
#include "haskellmanager.h"
#include "haskellproject.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/localenvironmentaspect.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/runnables.h>
#include <projectexplorer/target.h>
#include <utils/detailswidget.h>

#include <QFormLayout>

using namespace ProjectExplorer;

namespace Haskell {
namespace Internal {

HaskellRunConfigurationFactory::HaskellRunConfigurationFactory() {}

QList<Core::Id> HaskellRunConfigurationFactory::availableCreationIds(
    Target *parent,
    IRunConfigurationFactory::CreationMode mode) const
{
    Q_UNUSED(mode)
    const auto project = HaskellProject::toHaskellProject(parent->project());
    if (project)
        return project->availableRunConfigurationIds();
    return {};
}

QString HaskellRunConfigurationFactory::displayNameForId(Core::Id id) const
{
    return id.suffixAfter(Constants::C_HASKELL_RUNCONFIG_ID_PREFIX);
}

bool HaskellRunConfigurationFactory::canCreate(Target *parent, Core::Id id) const
{
    return id.name().startsWith(Constants::C_HASKELL_RUNCONFIG_ID_PREFIX)
           && HaskellProject::isHaskellProject(parent->project());
}

bool HaskellRunConfigurationFactory::canRestore(Target *parent, const QVariantMap &map) const
{
    return canCreate(parent, idFromMap(map));
}

bool HaskellRunConfigurationFactory::canClone(Target *parent, RunConfiguration *product) const
{
    return canCreate(parent, product->id());
}

RunConfiguration *HaskellRunConfigurationFactory::clone(Target *parent, RunConfiguration *product)
{
    return cloneHelper<HaskellRunConfiguration>(parent, product);
}

RunConfiguration *HaskellRunConfigurationFactory::doCreate(Target *parent, Core::Id id)
{
    QTC_ASSERT(canCreate(parent, id), return nullptr);
    return createHelper<HaskellRunConfiguration>(parent, id);
}

RunConfiguration *HaskellRunConfigurationFactory::doRestore(Target *parent, const QVariantMap &map)
{
    QTC_ASSERT(canRestore(parent, map), return nullptr);
    auto config = doCreate(parent, idFromMap(map));
    config->fromMap(map);
    return config;
}

HaskellRunConfiguration::HaskellRunConfiguration(Target *parent)
    : RunConfiguration(parent)
{
    auto argumentAspect = new ArgumentsAspect(this, "Haskell.RunAspect.Arguments");
    auto workingDirAspect = new WorkingDirectoryAspect(this, "Haskell.RunAspect.WorkingDirectory");
    workingDirAspect->setDefaultWorkingDirectory(parent->project()->projectDirectory());
    auto terminalAspect = new TerminalAspect(this, "Haskell.RunAspect.Terminal");
    auto environmentAspect
        = new LocalEnvironmentAspect(this, LocalEnvironmentAspect::BaseEnvironmentModifier());
    addExtraAspect(argumentAspect);
    addExtraAspect(terminalAspect);
    addExtraAspect(environmentAspect);
}

QWidget *HaskellRunConfiguration::createConfigurationWidget()
{
    auto details = new Utils::DetailsWidget;
    details->setState(Utils::DetailsWidget::NoSummary);
    auto widget = new QWidget;
    details->setWidget(widget);
    auto layout = new QFormLayout(widget);
    layout->setMargin(0);
    layout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    extraAspect<ArgumentsAspect>()->addToMainConfigurationWidget(widget, layout);
    extraAspect<TerminalAspect>()->addToMainConfigurationWidget(widget, layout);
    return details;
}

Runnable HaskellRunConfiguration::runnable() const
{
    const QString projectDirectory = target()->project()->projectDirectory().toString();
    StandardRunnable r;
    if (BuildConfiguration *buildConfiguration = target()->activeBuildConfiguration())
        r.commandLineArguments += "--work-dir \""
                                  + QDir(projectDirectory)
                                        .relativeFilePath(
                                            buildConfiguration->buildDirectory().toString())
                                  + "\" ";
    r.commandLineArguments += "exec \"" + m_executable + "\"";
    auto argumentsAspect = extraAspect<ArgumentsAspect>();
    if (!argumentsAspect->arguments().isEmpty())
        r.commandLineArguments += " -- " + argumentsAspect->arguments();
    r.workingDirectory = projectDirectory;
    r.runMode = extraAspect<TerminalAspect>()->runMode();
    r.environment = extraAspect<LocalEnvironmentAspect>()->environment();
    r.executable = r.environment.searchInPath(HaskellManager::stackExecutable().toString()).toString();
    return r;
}

void HaskellRunConfiguration::initialize(Core::Id id)
{
    RunConfiguration::initialize(id);
    m_executable = id.suffixAfter(Constants::C_HASKELL_RUNCONFIG_ID_PREFIX);
    setDefaultDisplayName(m_executable);
}

} // namespace Internal
} // namespace Haskell
