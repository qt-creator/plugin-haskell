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

#include <utils/fileutils.h>
#include <utils/optional.h>
#include <utils/synchronousprocess.h>

#include <QFuture>
#include <QMutex>
#include <QThread>

#include <memory>

QT_BEGIN_NAMESPACE
class QProcess;
QT_END_NAMESPACE

namespace Haskell {
namespace Internal {

class SymbolInfo {
public:
    QStringList definition;
    QStringList additionalInfo;
    Utils::FileName file;
    int line = -1;
    int col = -1;
    QString module;
};

template <typename T> class ghcmod_deleter;
template <> class ghcmod_deleter<QProcess>
{
public:
    void operator()(QProcess *p) { Utils::SynchronousProcess::stopProcess(*p); delete p; }
};
using unique_ghcmod_process = std::unique_ptr<QProcess, ghcmod_deleter<QProcess>>;

class GhcMod
{
public:
    GhcMod(const Utils::FileName &path);
    ~GhcMod();

    Utils::FileName basePath() const;

    Utils::optional<SymbolInfo> findSymbol(const Utils::FileName &filePath, const QString &symbol);
    Utils::optional<QString> typeInfo(const Utils::FileName &filePath, int line, int col);

    Utils::optional<QByteArray> runQuery(const QString &query);

    Utils::optional<QByteArray> runFindSymbol(const Utils::FileName &filePath, const QString &symbol);
    Utils::optional<QByteArray> runTypeInfo(const Utils::FileName &filePath, int line, int col);

    static Utils::optional<SymbolInfo> parseFindSymbol(const Utils::optional<QByteArray> &response);
    static Utils::optional<QString> parseTypeInfo(const Utils::optional<QByteArray> &response);
private:
    bool ensureStarted();
    void shutdown();
    void log(const QString &message);

    Utils::FileName m_path;
    unique_ghcmod_process m_process; // kills process on reset
};

class AsyncGhcMod : public QObject
{
    Q_OBJECT

public:
    struct Operation {
        Operation() = default;
        Operation(const std::function<Utils::optional<QByteArray>()> &op);
        mutable QFutureInterface<Utils::optional<QByteArray>> fi;
        std::function<Utils::optional<QByteArray>()> op;
    };

    AsyncGhcMod(const Utils::FileName &path);
    ~AsyncGhcMod();

    Utils::FileName basePath() const;

    QFuture<Utils::optional<SymbolInfo>> findSymbol(const Utils::FileName &filePath,
                                                    const QString &symbol);
    QFuture<Utils::optional<QString>> typeInfo(const Utils::FileName &filePath, int line, int col);

private:
    void reduceQueue();

    QThread m_thread;
    GhcMod m_ghcmod;
    QVector<Operation> m_queue;
    QMutex m_mutex;
};

} // Internal
} // Haskell
