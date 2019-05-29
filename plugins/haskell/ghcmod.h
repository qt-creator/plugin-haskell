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

#include "filecache.h"

#include <utils/fileutils.h>
#include <utils/optional.h>
#include <utils/synchronousprocess.h>
#include <utils/variant.h>

#include <QFuture>
#include <QMutex>
#include <QThread>

#include <memory>

QT_BEGIN_NAMESPACE
class QProcess;
QT_END_NAMESPACE

namespace Haskell {
namespace Internal {

class Error {
public:
    enum class Type {
        FailedToStartStack,
        Other // TODO get rid of it
    };
    Type type;
    QString details;
};

class SymbolInfo {
public:
    QStringList definition;
    QStringList additionalInfo;
    Utils::FilePath file;
    int line = -1;
    int col = -1;
    QString module;
};

using QByteArrayOrError = Utils::variant<QByteArray, Error>;
using QStringOrError = Utils::variant<QString, Error>;
using SymbolInfoOrError = Utils::variant<SymbolInfo, Error>;

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
    GhcMod(const Utils::FilePath &path);
    ~GhcMod();

    Utils::FilePath basePath() const;
    void setFileMap(const QHash<Utils::FilePath, Utils::FilePath> &fileMap);

    SymbolInfoOrError findSymbol(const Utils::FilePath &filePath, const QString &symbol);
    QStringOrError typeInfo(const Utils::FilePath &filePath, int line, int col);

    QByteArrayOrError runQuery(const QString &query);

    QByteArrayOrError runFindSymbol(const Utils::FilePath &filePath, const QString &symbol);
    QByteArrayOrError runTypeInfo(const Utils::FilePath &filePath, int line, int col);

    static SymbolInfoOrError parseFindSymbol(const QByteArrayOrError &response);
    static QStringOrError parseTypeInfo(const QByteArrayOrError &response);

   static void setStackExecutable(const Utils::FilePath &filePath);

private:
    Utils::optional<Error> ensureStarted();
    void shutdown();
    void log(const QString &message);

    static Utils::FilePath m_stackExecutable;
    static QMutex m_mutex;

    Utils::FilePath m_path;
    unique_ghcmod_process m_process; // kills process on reset
    QHash<Utils::FilePath, Utils::FilePath> m_fileMap;
};

class AsyncGhcMod : public QObject
{
    Q_OBJECT

public:
    struct Operation {
        Operation() = default;
        Operation(const std::function<QByteArrayOrError()> &op);
        mutable QFutureInterface<QByteArrayOrError> fi;
        std::function<QByteArrayOrError()> op;
    };

    AsyncGhcMod(const Utils::FilePath &path);
    ~AsyncGhcMod();

    Utils::FilePath basePath() const;

    QFuture<SymbolInfoOrError> findSymbol(const Utils::FilePath &filePath, const QString &symbol);
    QFuture<QStringOrError> typeInfo(const Utils::FilePath &filePath, int line, int col);

private slots:
    void updateCache(); // called through QMetaObject::invokeMethod

private:
    void reduceQueue();

    QThread m_thread;
    QObject m_threadTarget; // used to run methods in m_thread
    GhcMod m_ghcmod; // only use in m_thread
    FileCache m_fileCache; // only update through reduceQueue
    QVector<Operation> m_queue;
    QMutex m_mutex;
};

} // Internal
} // Haskell
