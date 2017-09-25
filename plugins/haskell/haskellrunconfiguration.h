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

#pragma once

#include <projectexplorer/runconfiguration.h>

namespace Haskell {
namespace Internal {

class HaskellRunConfigurationFactory : public ProjectExplorer::IRunConfigurationFactory
{
    Q_OBJECT

public:
    HaskellRunConfigurationFactory();

    QList<Core::Id> availableCreationIds(ProjectExplorer::Target *parent,
                                         CreationMode mode) const override;
    QString displayNameForId(Core::Id id) const override;
    bool canCreate(ProjectExplorer::Target *parent, Core::Id id) const override;
    bool canRestore(ProjectExplorer::Target *parent, const QVariantMap &map) const override;
    bool canClone(ProjectExplorer::Target *parent,
                  ProjectExplorer::RunConfiguration *product) const override;
    ProjectExplorer::RunConfiguration *clone(ProjectExplorer::Target *parent,
                                             ProjectExplorer::RunConfiguration *product) override;

private:
    ProjectExplorer::RunConfiguration *doCreate(ProjectExplorer::Target *parent,
                                                Core::Id id) override;
    ProjectExplorer::RunConfiguration *doRestore(ProjectExplorer::Target *parent,
                                                 const QVariantMap &map) override;
};

class HaskellRunConfiguration : public ProjectExplorer::RunConfiguration
{
    Q_OBJECT

public:
    HaskellRunConfiguration(ProjectExplorer::Target *parent);

    QWidget *createConfigurationWidget() override;
    ProjectExplorer::Runnable runnable() const override;
    void initialize(Core::Id id);

private:
    QString m_executable;
};

} // namespace Internal
} // namespace Haskell
