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

#include "ghcmod.h"

#include <utils/environment.h>

#include <QFileInfo>
#include <QFutureWatcher>
#include <QLoggingCategory>
#include <QMutexLocker>
#include <QProcess>
#include <QRegularExpression>
#include <QTime>
#include <QTimer>

Q_LOGGING_CATEGORY(ghcModLog, "qtc.haskell.ghcmod")
Q_LOGGING_CATEGORY(asyncGhcModLog, "qtc.haskell.ghcmod.async")

// TODO do not hardcode
const char STACK_EXE[] = "/usr/local/bin/stack";
const int kTimeoutMS = 10 * 1000;

using namespace Utils;

namespace Haskell {
namespace Internal {

GhcMod::GhcMod(const Utils::FileName &path)
    : m_path(path)
{
}

GhcMod::~GhcMod()
{
    shutdown();
}

FileName GhcMod::basePath() const
{
    return m_path;
}

static QString toUnicode(QByteArray data)
{
    // clean zero bytes which let QString think that the string ends there
    data.replace('\x0', QByteArray());
    return QString::fromUtf8(data);
}

Utils::optional<SymbolInfo> GhcMod::findSymbol(const FileName &filePath, const QString &symbol)
{
    return parseFindSymbol(runFindSymbol(filePath, symbol));
}

Utils::optional<QString> GhcMod::typeInfo(const FileName &filePath, int line, int col)
{
    return parseTypeInfo(runTypeInfo(filePath, line, col));
}

bool GhcMod::ensureStarted()
{
    if (m_process)
        return true;
    log("starting");
    Environment env = Environment::systemEnvironment();
    env.prependOrSetPath(QFileInfo(STACK_EXE).absolutePath()); // for ghc-mod finding stack back
    m_process.reset(new QProcess);
    m_process->setWorkingDirectory(m_path.toString());
    m_process->setEnvironment(env.toStringList());
    m_process->start(STACK_EXE, {"exec", "ghc-mod", "--", "legacy-interactive"});
    if (!m_process->waitForStarted(kTimeoutMS)) {
        log("failed to start");
        return false;
    }
    log("started");
    m_process->setReadChannel(QProcess::StandardOutput);
    return true;
}

void GhcMod::shutdown()
{
    if (!m_process)
        return;
    log("shutting down");
    m_process->write("\n");
    m_process->closeWriteChannel();
    m_process->waitForFinished(300);
    m_process.reset();
}

void GhcMod::log(const QString &message)
{
    qCDebug(ghcModLog) << "ghcmod for" << m_path.toString() << ":" << qPrintable(message);
}

Utils::optional<QByteArray> GhcMod::runQuery(const QString &query)
{
    if (!ensureStarted())
        return Utils::nullopt;
    log("query \"" + query + "\"");
    m_process->write(query.toUtf8() + "\n");
    bool ok = false;
    QByteArray response;
    QTime readTime;
    readTime.start();
    while (!ok && readTime.elapsed() < kTimeoutMS) {
        m_process->waitForReadyRead(kTimeoutMS - readTime.elapsed() + 10);
        response += m_process->read(2048);
        ok = response.endsWith("OK\n") || response.endsWith("OK\r\n");
    }
    if (ghcModLog().isDebugEnabled())
        qCDebug(ghcModLog) << "response" << QTextCodec::codecForLocale()->toUnicode(response);
    if (!ok) {
        log("failed");
        m_process.reset();
        return Utils::nullopt;
    }
    log("success");
    // convert to unix line endings
    response.replace("\r\n", "\n");
    response.chop(3); // cut off "OK\n"
    return response;
}

Utils::optional<QByteArray> GhcMod::runFindSymbol(const FileName &filePath, const QString &symbol)
{
    return runQuery(QString("info %1 %2").arg(filePath.toString()) // TODO toNative? quoting?
                    .arg(symbol));
}

Utils::optional<QByteArray> GhcMod::runTypeInfo(const FileName &filePath, int line, int col)
{
    return runQuery(QString("type %1 %2 %3").arg(filePath.toString()) // TODO toNative? quoting?
                    .arg(line)
                    .arg(col + 1));
}

Utils::optional<SymbolInfo> GhcMod::parseFindSymbol(const Utils::optional<QByteArray> &response)
{
    QRegularExpression infoRegEx("^\\s*(.*?)\\s+--\\sDefined ((at (.+?)(:(\\d+):(\\d+))?)|(in ‘(.+)’.*))$");
    if (!response)
        return Utils::nullopt;
    SymbolInfo info;
    bool hasFileOrModule = false;
    const QString str = toUnicode(QByteArray(response.value()).replace('\x0', '\n'));
    for (const QString &line : str.split('\n')) {
        if (hasFileOrModule) {
            info.additionalInfo += line;
        } else {
            QRegularExpressionMatch result = infoRegEx.match(line);
            if (result.hasMatch()) {
                hasFileOrModule = true;
                info.definition += result.captured(1);
                if (result.lastCapturedIndex() == 7) { // Defined at <file:line:col>
                    info.file = FileName::fromString(result.captured(4));
                    bool ok;
                    int num = result.captured(6).toInt(&ok);
                    if (ok)
                        info.line = num;
                    num = result.captured(7).toInt(&ok);
                    if (ok)
                        info.col = num;
                } else if (result.lastCapturedIndex() == 9) { // Defined in <module>
                    info.module = result.captured(9);
                }
            } else {
                info.definition += line;
            }
        }
    }
    if (hasFileOrModule)
        return info;
    return Utils::nullopt;
}

Utils::optional<QString> GhcMod::parseTypeInfo(const Utils::optional<QByteArray> &response)
{
    QRegularExpression typeRegEx("^\\d+\\s+\\d+\\s+\\d+\\s+\\d+\\s+\"(.*)\"$",
                                 QRegularExpression::MultilineOption);
    if (!response)
        return Utils::nullopt;
    QRegularExpressionMatch result = typeRegEx.match(toUnicode(response.value()));
    if (result.hasMatch())
        return result.captured(1);
    return Utils::nullopt;
}

AsyncGhcMod::AsyncGhcMod(const FileName &path)
    : m_ghcmod(path)
{
    qCDebug(asyncGhcModLog) << "starting thread for" << m_ghcmod.basePath().toString();
    moveToThread(&m_thread);
    m_thread.start();
}

AsyncGhcMod::~AsyncGhcMod()
{
    qCDebug(asyncGhcModLog) << "stopping thread for" << m_ghcmod.basePath().toString();
    m_mutex.lock();
    for (Operation &op : m_queue)
        op.fi.cancel();
    m_queue.clear();
    m_mutex.unlock();
    m_thread.quit();
    m_thread.wait();
}

FileName AsyncGhcMod::basePath() const
{
    return m_ghcmod.basePath();
}

template <typename Result>
QFuture<Result> createFuture(AsyncGhcMod::Operation op,
                             const std::function<Result(const Utils::optional<QByteArray>&)> &postOp)
{
    auto fi = new QFutureInterface<Result>;
    fi->reportStarted();

    auto opWatcher = new QFutureWatcher<Utils::optional<QByteArray>>();
    QObject::connect(opWatcher, &QFutureWatcherBase::canceled, [fi] { fi->cancel(); });
    QObject::connect(opWatcher, &QFutureWatcherBase::finished, opWatcher, &QObject::deleteLater);
    QObject::connect(opWatcher, &QFutureWatcherBase::finished, [fi] {
        fi->reportFinished();
        delete fi;
    });
    QObject::connect(opWatcher, &QFutureWatcherBase::resultReadyAt,
                     [fi, opWatcher, postOp](int index) {
                         fi->reportResult(postOp(opWatcher->future().resultAt(index)));
                     });
    opWatcher->setFuture(op.fi.future());

    auto fiWatcher = new QFutureWatcher<Result>();
    QObject::connect(fiWatcher, &QFutureWatcherBase::canceled, [op] { op.fi.cancel(); });
    QObject::connect(fiWatcher, &QFutureWatcherBase::finished, fiWatcher, &QObject::deleteLater);
    fiWatcher->setFuture(fi->future());

    return fi->future();
}

QFuture<Utils::optional<SymbolInfo>> AsyncGhcMod::findSymbol(const FileName &filePath,
                                                             const QString &symbol)
{
    QMutexLocker lock(&m_mutex);
    Operation op([this, filePath, symbol] { return m_ghcmod.runFindSymbol(filePath, symbol); });
    m_queue.append(op);
    QTimer::singleShot(0, this, [this] { reduceQueue(); });
    return createFuture<Utils::optional<SymbolInfo>>(op, &GhcMod::parseFindSymbol);
}

QFuture<Utils::optional<QString>> AsyncGhcMod::typeInfo(const FileName &filePath, int line, int col)
{
    QMutexLocker lock(&m_mutex);
    Operation op([this, filePath, line, col] { return m_ghcmod.runTypeInfo(filePath, line, col); });
    m_queue.append(op);
    QTimer::singleShot(0, this, [this] { reduceQueue(); });
    return createFuture<Utils::optional<QString>>(op, &GhcMod::parseTypeInfo);
}

void AsyncGhcMod::reduceQueue()
{
    const auto takeFirst = [this]() {
        Operation op;
        m_mutex.lock();
        if (!m_queue.isEmpty())
            op = m_queue.takeFirst();
        m_mutex.unlock();
        return op;
    };

    Operation op;
    while ((op = takeFirst()).op) {
        if (!op.fi.isCanceled()) {
            Utils::optional<QByteArray> result = op.op();
            op.fi.reportResult(result);
        }
        op.fi.reportFinished();
    }
}

AsyncGhcMod::Operation::Operation(const std::function<Utils::optional<QByteArray>()> &op)
    : op(op)
{
    fi.reportStarted();
}

} // Internal
} // Haskell
