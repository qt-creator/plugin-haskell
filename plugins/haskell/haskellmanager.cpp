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

#include "haskellmanager.h"

#include "ghcmod.h"

#include <QDir>
#include <QFileInfo>

#include <unordered_map>

using namespace Utils;

namespace Haskell {
namespace Internal {

class HaskellManagerPrivate
{
public:
    std::unordered_map<FileName, std::weak_ptr<AsyncGhcMod>> ghcModCache;
};

Q_GLOBAL_STATIC(HaskellManagerPrivate, m_d);

FileName HaskellManager::findProjectDirectory(const FileName &filePath)
{
    if (filePath.isEmpty())
        return FileName();

    QDir directory(filePath.toFileInfo().isDir() ? filePath.toString()
                                                 : filePath.parentDir().toString());
    directory.setNameFilters({"stack.yaml", "*.cabal"});
    directory.setFilter(QDir::Files | QDir::Readable);
    do {
        if (!directory.entryList().isEmpty())
            return FileName::fromString(directory.path());
    } while (!directory.isRoot() && directory.cdUp());
    return FileName();
}

std::shared_ptr<AsyncGhcMod> HaskellManager::ghcModForFile(const FileName &filePath)
{
    const FileName projectPath = findProjectDirectory(filePath);
    const auto cacheEntry = m_d->ghcModCache.find(projectPath);
    if (cacheEntry != m_d->ghcModCache.cend()) {
        if (cacheEntry->second.expired())
            m_d->ghcModCache.erase(cacheEntry);
        else
            return cacheEntry->second.lock();
    }
    auto ghcmod = std::make_shared<AsyncGhcMod>(projectPath);
    m_d->ghcModCache.insert(std::make_pair(projectPath, ghcmod));
    return ghcmod;
}

} // namespace Internal
} // namespace Haskell
