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

#include <coreplugin/editormanager/documentmodel.h>
#include <coreplugin/idocument.h>
#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/qtcassert.h>

#include <QFileInfo>
#include <QFutureWatcher>
#include <QLoggingCategory>
#include <QMetaObject>
#include <QMutexLocker>
#include <QProcess>
#include <QRegularExpression>
#include <QTime>
#include <QTimer>

#include <functional>

Q_LOGGING_CATEGORY(ghcModLog, "qtc.haskell.ghcmod", QtWarningMsg)
Q_LOGGING_CATEGORY(asyncGhcModLog, "qtc.haskell.ghcmod.async", QtWarningMsg)

// TODO do not hardcode
const int kTimeoutMS = 10 * 1000;

using namespace Utils;

namespace Haskell {
namespace Internal {

FileName GhcMod::m_stackExecutable = Utils::FileName::fromString("stack");
QMutex GhcMod::m_mutex;

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

void GhcMod::setFileMap(const QHash<FileName, FileName> &fileMap)
{
    if (fileMap != m_fileMap) {
        log("setting new file map");
        m_fileMap = fileMap;
        shutdown();
    }
}

static QString toUnicode(QByteArray data)
{
    // clean zero bytes which let QString think that the string ends there
    data.replace('\x0', QByteArray());
    return QString::fromUtf8(data);
}

SymbolInfoOrError GhcMod::findSymbol(const FileName &filePath, const QString &symbol)
{
    return parseFindSymbol(runFindSymbol(filePath, symbol));
}

QStringOrError GhcMod::typeInfo(const FileName &filePath, int line, int col)
{
    return parseTypeInfo(runTypeInfo(filePath, line, col));
}

static QStringList fileMapArgs(const QHash<FileName, FileName> &map)
{
    QStringList result;
    const auto end = map.cend();
    for (auto it = map.cbegin(); it != end; ++it)
        result << "--map-file" << it.key().toString() + "=" + it.value().toString();
    return result;
}

Utils::optional<Error> GhcMod::ensureStarted()
{
    m_mutex.lock();
    const FileName plainStackExecutable = m_stackExecutable;
    m_mutex.unlock();
    Environment env = Environment::systemEnvironment();
    const FileName stackExecutable = env.searchInPath(plainStackExecutable.toString());
    if (m_process) {
        if (m_process->state() == QProcess::NotRunning) {
            log("is no longer running");
            m_process.reset();
        } else if (FileName::fromString(m_process->program()) != stackExecutable) {
            log("stack settings changed");
            shutdown();
        }
    }
    if (m_process)
        return Utils::nullopt;
    log("starting");
    // for ghc-mod finding stack back:
    env.prependOrSetPath(stackExecutable.toFileInfo().absolutePath());
    m_process.reset(new QProcess);
    m_process->setWorkingDirectory(m_path.toString());
    m_process->setEnvironment(env.toStringList());
    m_process->start(stackExecutable.toString(),
                     QStringList{"exec", "ghc-mod", "--"} + fileMapArgs(m_fileMap)
                     << "legacy-interactive");
    if (!m_process->waitForStarted(kTimeoutMS)) {
        log("failed to start");
        m_process.reset();
        return Error({Error::Type::FailedToStartStack, plainStackExecutable.toUserOutput()});
    }
    log("started");
    m_process->setReadChannel(QProcess::StandardOutput);
    return Utils::nullopt;
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

QByteArrayOrError GhcMod::runQuery(const QString &query)
{
    const Utils::optional<Error> error = ensureStarted();
    if (error)
        return error.value();
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
        shutdown();
        return Error({Error::Type::Other, QString()});
    }
    log("success");
    // convert to unix line endings
    response.replace("\r\n", "\n");
    response.chop(3); // cut off "OK\n"
    return response;
}

QByteArrayOrError GhcMod::runFindSymbol(const FileName &filePath, const QString &symbol)
{
    return runQuery(QString("info %1 %2").arg(filePath.toString()) // TODO toNative? quoting?
                    .arg(symbol));
}

QByteArrayOrError GhcMod::runTypeInfo(const FileName &filePath, int line, int col)
{
    return runQuery(QString("type %1 %2 %3").arg(filePath.toString()) // TODO toNative? quoting?
                    .arg(line)
                    .arg(col + 1));
}

SymbolInfoOrError GhcMod::parseFindSymbol(const QByteArrayOrError &response)
{
    QRegularExpression infoRegEx("^\\s*(.*?)\\s+--\\sDefined ((at (.+?)(:(\\d+):(\\d+))?)|(in ‘(.+)’.*))$");
    const QByteArray *bytes = Utils::get_if<QByteArray>(&response);
    if (!bytes)
        return Utils::get<Error>(response);
    SymbolInfo info;
    bool hasFileOrModule = false;
    const QString str = toUnicode(QByteArray(*bytes).replace('\x0', '\n'));
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
    return Error({Error::Type::Other, QString()});
}

QStringOrError GhcMod::parseTypeInfo(const QByteArrayOrError &response)
{
    QRegularExpression typeRegEx("^\\d+\\s+\\d+\\s+\\d+\\s+\\d+\\s+\"(.*)\"$",
                                 QRegularExpression::MultilineOption);
    const QByteArray *bytes = Utils::get_if<QByteArray>(&response);
    if (!bytes)
        return Utils::get<Error>(response);
    QRegularExpressionMatch result = typeRegEx.match(toUnicode(*bytes));
    if (result.hasMatch())
        return result.captured(1);
    return Error({Error::Type::Other, QString()});
}

void GhcMod::setStackExecutable(const FileName &filePath)
{
    QMutexLocker lock(&m_mutex);
    m_stackExecutable = filePath;
}

static QList<Core::IDocument *> getOpenDocuments(const FileName &path)
{
    return Utils::filtered(Core::DocumentModel::openedDocuments(), [path] (Core::IDocument *doc) {
        return path.isEmpty() || doc->filePath().isChildOf(path);
    });
}

AsyncGhcMod::AsyncGhcMod(const FileName &path)
    : m_ghcmod(path),
      m_fileCache("haskell", std::bind(getOpenDocuments, path))
{
    qCDebug(asyncGhcModLog) << "starting thread for" << m_ghcmod.basePath().toString();
    m_thread.start();
    m_threadTarget.moveToThread(&m_thread);
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
                             const std::function<Result(const QByteArrayOrError &)> &postOp)
{
    auto fi = new QFutureInterface<Result>;
    fi->reportStarted();

    // propagate inner events to outside future
    auto opWatcher = new QFutureWatcher<QByteArrayOrError>();
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

    // propagate cancel from outer future to inner future
    auto fiWatcher = new QFutureWatcher<Result>();
    QObject::connect(fiWatcher, &QFutureWatcherBase::canceled, [op] { op.fi.cancel(); });
    QObject::connect(fiWatcher, &QFutureWatcherBase::finished, fiWatcher, &QObject::deleteLater);
    fiWatcher->setFuture(fi->future());

    return fi->future();
}

/*!
    Asynchronously looks up the \a symbol in the context of \a filePath.

    Returns a QFuture handle for the asynchronous operation. You may not block the main event loop
    while waiting for it to finish - doing so will result in a deadlock.
*/
QFuture<SymbolInfoOrError> AsyncGhcMod::findSymbol(const FileName &filePath,
                                                             const QString &symbol)
{
    QMutexLocker lock(&m_mutex);
    Operation op([this, filePath, symbol] { return m_ghcmod.runFindSymbol(filePath, symbol); });
    m_queue.append(op);
    QTimer::singleShot(0, &m_threadTarget, [this] { reduceQueue(); });
    return createFuture<SymbolInfoOrError>(op, &GhcMod::parseFindSymbol);
}

/*!
    Asynchronously looks up the type at \a line and \a col in \a filePath.

    Returns a QFuture handle for the asynchronous operation. You may not block the main event loop
    while waiting for it to finish - doing so will result in a deadlock.
*/
QFuture<QStringOrError> AsyncGhcMod::typeInfo(const FileName &filePath, int line, int col)
{
    QMutexLocker lock(&m_mutex);
    Operation op([this, filePath, line, col] { return m_ghcmod.runTypeInfo(filePath, line, col); });
    m_queue.append(op);
    QTimer::singleShot(0, &m_threadTarget, [this] { reduceQueue(); });
    return createFuture<QStringOrError>(op, &GhcMod::parseTypeInfo);
}

/*!
    Synchronously runs an update of the cache of modified files.
    This must be run on the main thread.

    \internal
*/
void AsyncGhcMod::updateCache()
{
    m_fileCache.update();
}

/*!
    Takes operations from the queue and executes them, until the queue is empty.
    This must be run within the internal thread whenever an operation is added to the queue.
    Canceled operations are not executed, but removed from the queue.
    Before each operation, the cache of modified files is updated on the main thread.

    \internal
*/
void AsyncGhcMod::reduceQueue()
{
    QTC_ASSERT(QThread::currentThread() != thread(), return);
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
            QMetaObject::invokeMethod(this, "updateCache", Qt::BlockingQueuedConnection);
            m_ghcmod.setFileMap(m_fileCache.fileMap());
            QByteArrayOrError result = op.op();
            op.fi.reportResult(result);
        }
        op.fi.reportFinished();
    }
}

AsyncGhcMod::Operation::Operation(const std::function<QByteArrayOrError()> &op)
    : op(op)
{
    fi.reportStarted();
}

} // Internal
} // Haskell
