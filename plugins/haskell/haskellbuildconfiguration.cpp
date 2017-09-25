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

#include "haskellbuildconfiguration.h"

#include "haskellconstants.h"
#include "haskellproject.h"
#include "stackbuildstep.h"

#include <projectexplorer/buildinfo.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <utils/algorithm.h>
#include <utils/detailswidget.h>
#include <utils/mimetypes/mimedatabase.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>

#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

using namespace ProjectExplorer;

const char C_HASKELL_BUILDCONFIGURATION_ID[] = "Haskell.BuildConfiguration";

namespace Haskell {
namespace Internal {

HaskellBuildConfigurationFactory::HaskellBuildConfigurationFactory() {}

int HaskellBuildConfigurationFactory::priority(const Target *parent) const
{
    return HaskellProject::isHaskellProject(parent->project()) ? 0 : -1;
}

static QList<BuildInfo *> createInfos(const HaskellBuildConfigurationFactory *factory,
                                      const Kit *k,
                                      const Utils::FileName &projectFilePath)
{
    auto info = new BuildInfo(factory);
    info->typeName = HaskellBuildConfigurationFactory::tr("Release");
    info->displayName = info->typeName;
    info->buildDirectory = projectFilePath.parentDir().appendPath(".stack-work");
    info->kitId = k->id();
    info->buildType = BuildConfiguration::BuildType::Release;
    return {info};
}

QList<BuildInfo *> HaskellBuildConfigurationFactory::availableBuilds(const Target *parent) const
{
    // Entries that are available in add build configuration dropdown
    QTC_ASSERT(priority(parent) > -1, return {});
    return Utils::transform(createInfos(this, parent->kit(), parent->project()->projectFilePath()),
                            [](BuildInfo *info) {
                                info->displayName.clear();
                                return info;
                            });
}

int HaskellBuildConfigurationFactory::priority(const Kit *k, const QString &projectPath) const
{
    if (k && Utils::mimeTypeForFile(projectPath).matchesName(Constants::C_HASKELL_PROJECT_MIMETYPE))
        return 0;
    return -1;
}

QList<BuildInfo *> HaskellBuildConfigurationFactory::availableSetups(
    const Kit *k, const QString &projectPath) const
{
    QTC_ASSERT(priority(k, projectPath) > -1, return {});
    return createInfos(this, k, Utils::FileName::fromString(projectPath));
}

BuildConfiguration *HaskellBuildConfigurationFactory::create(Target *parent,
                                                             const BuildInfo *info) const
{
    QTC_ASSERT(HaskellProject::isHaskellProject(parent->project()), return nullptr);
    auto config = new HaskellBuildConfiguration(parent);
    config->setBuildDirectory(info->buildDirectory);
    config->setBuildType(info->buildType);
    config->setDisplayName(info->displayName);

    BuildStepList *buildSteps = config->stepList(Core::Id(ProjectExplorer::Constants::BUILDSTEPS_BUILD));
    auto stackBuildStep = new StackBuildStep(buildSteps);
    buildSteps->appendStep(stackBuildStep);

    return config;
}

bool HaskellBuildConfigurationFactory::canRestore(const Target *parent, const QVariantMap &map) const
{
    Q_UNUSED(map)
    return HaskellProject::isHaskellProject(parent->project());
}

BuildConfiguration *HaskellBuildConfigurationFactory::restore(Target *parent, const QVariantMap &map)
{
    QTC_ASSERT(canRestore(parent, map), return nullptr);
    auto config = new HaskellBuildConfiguration(parent);
    config->fromMap(map);
    return config;
}

bool HaskellBuildConfigurationFactory::canClone(const Target *parent,
                                                BuildConfiguration *product) const
{
    Q_UNUSED(parent)
    return product->id() == C_HASKELL_BUILDCONFIGURATION_ID;
}

BuildConfiguration *HaskellBuildConfigurationFactory::clone(Target *parent,
                                                            BuildConfiguration *product)
{
    QTC_ASSERT(canClone(parent, product), return nullptr);
    auto config = new HaskellBuildConfiguration(parent);
    config->fromMap(product->toMap());
    return config;
}

HaskellBuildConfiguration::HaskellBuildConfiguration(Target *target)
    : BuildConfiguration(target, C_HASKELL_BUILDCONFIGURATION_ID)
{}

NamedWidget *HaskellBuildConfiguration::createConfigWidget()
{
    return new HaskellBuildConfigurationWidget(this);
}

BuildConfiguration::BuildType HaskellBuildConfiguration::buildType() const
{
    return m_buildType;
}

void HaskellBuildConfiguration::setBuildType(BuildConfiguration::BuildType type)
{
    m_buildType = type;
}

HaskellBuildConfigurationWidget::HaskellBuildConfigurationWidget(HaskellBuildConfiguration *bc)
    : NamedWidget()
    , m_buildConfiguration(bc)
{
    setDisplayName(tr("General"));
    setLayout(new QVBoxLayout);
    layout()->setMargin(0);
    auto box = new Utils::DetailsWidget;
    box->setState(Utils::DetailsWidget::NoSummary);
    layout()->addWidget(box);
    auto details = new QWidget;
    box->setWidget(details);
    details->setLayout(new QHBoxLayout);
    details->layout()->setMargin(0);
    details->layout()->addWidget(new QLabel(tr("Build directory:")));

    auto buildDirectoryInput = new Utils::PathChooser;
    buildDirectoryInput->setExpectedKind(Utils::PathChooser::Directory);
    buildDirectoryInput->setFileName(m_buildConfiguration->buildDirectory());
    details->layout()->addWidget(buildDirectoryInput);

    connect(m_buildConfiguration,
            &BuildConfiguration::buildDirectoryChanged,
            buildDirectoryInput,
            [this, buildDirectoryInput] {
                buildDirectoryInput->setFileName(m_buildConfiguration->buildDirectory());
            });
    connect(buildDirectoryInput,
            &Utils::PathChooser::pathChanged,
            m_buildConfiguration,
            [this, buildDirectoryInput](const QString &) {
                m_buildConfiguration->setBuildDirectory(buildDirectoryInput->rawFileName());
            });
}

} // namespace Internal
} // namespace Haskell
