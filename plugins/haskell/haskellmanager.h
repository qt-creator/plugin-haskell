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

#include <QObject>

#include <utils/fileutils.h>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace Haskell {
namespace Internal {

class AsyncGhcMod;

class HaskellManager : public QObject
{
    Q_OBJECT

public:
    static HaskellManager *instance();

    static Utils::FileName findProjectDirectory(const Utils::FileName &filePath);
    static std::shared_ptr<AsyncGhcMod> ghcModForFile(const Utils::FileName &filePath);
    static Utils::FileName stackExecutable();
    static void setStackExecutable(const Utils::FileName &filePath);
    static void readSettings(QSettings *settings);
    static void writeSettings(QSettings *settings);

    static QString trLookingUp(const QString &name);

signals:
    void stackExecutableChanged(const Utils::FileName &filePath);
};

} // namespace Internal
} // namespace Haskell
