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

#include "haskellproject.h"

#include "haskellconstants.h"

#include <coreplugin/iversioncontrol.h>
#include <coreplugin/vcsmanager.h>
#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/runextensions.h>

#include <QFile>
#include <QTextStream>

using namespace ProjectExplorer;
using namespace Utils;

const char C_HASKELL_PROJECT_ID[] = "Haskell.Project";

namespace Haskell {
namespace Internal {

static QVector<QString> parseExecutableNames(const FileName &projectFilePath)
{
    static const QString EXECUTABLE = "executable";
    static const int EXECUTABLE_LEN = EXECUTABLE.length();
    QVector<QString> result;
    QFile file(projectFilePath.toString());
    if (file.open(QFile::ReadOnly)) {
        QTextStream stream(&file);
        while (!stream.atEnd()) {
            const QString line = stream.readLine().trimmed();
            if (line.length() > EXECUTABLE_LEN && line.startsWith(EXECUTABLE)
                    && line.at(EXECUTABLE_LEN).isSpace())
                result.append(line.mid(EXECUTABLE_LEN + 1).trimmed());
        }
    }
    return result;
}

HaskellProjectNode::HaskellProjectNode(const FileName &projectFilePath, Core::Id id)
    : ProjectNode(projectFilePath, id.toString().toLatin1())
{}

HaskellProject::HaskellProject(const Utils::FileName &fileName)
    : Project(Constants::C_HASKELL_PROJECT_MIMETYPE, fileName)
{
    setId(C_HASKELL_PROJECT_ID);
    setDisplayName(fileName.toFileInfo().completeBaseName());
    updateFiles();
}

bool HaskellProject::isHaskellProject(Project *project)
{
    return project && project->id() == C_HASKELL_PROJECT_ID;
}

HaskellProject *HaskellProject::toHaskellProject(Project *project)
{
    if (project && project->id() == C_HASKELL_PROJECT_ID)
        return static_cast<HaskellProject *>(project);
    return nullptr;
}

QList<QString> HaskellProject::availableExecutables() const
{
    return parseExecutableNames(projectFilePath()).toList();
}

void HaskellProject::updateFiles()
{
    emitParsingStarted();
    FileName projectDir = projectDirectory();
    const QList<Core::IVersionControl *> versionControls = Core::VcsManager::versionControls();
    QFuture<QList<FileNode *>> future = Utils::runAsync([this, projectDir, versionControls] {
        return FileNode::scanForFilesWithVersionControls(
            projectDir,
            [this](const FileName &fn) -> FileNode * {
                if (fn != FileName::fromString(projectFilePath().toString() + ".user"))
                    return new FileNode(fn, FileType::Source, false);
                else
                    return nullptr;
            },
            versionControls);
    });
    Utils::onResultReady(future, this, [this](const QList<FileNode *> &nodes) {
        auto root = new HaskellProjectNode(projectDirectory(), id());
        root->setDisplayName(displayName());
        root->addNestedNodes(nodes);
        setRootProjectNode(root);
        emitParsingFinished(true);
    });
}

} // namespace Internal
} // namespace Haskell
