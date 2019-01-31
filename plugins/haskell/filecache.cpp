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

#include "filecache.h"

#include <coreplugin/idocument.h>
#include <texteditor/textdocument.h>
#include <utils/algorithm.h>

#include <QFile>
#include <QLoggingCategory>
#include <QTemporaryFile>
#include <QTextDocument>

Q_LOGGING_CATEGORY(cacheLog, "qtc.haskell.filecache", QtWarningMsg)

using namespace Core;
using namespace TextEditor;
using namespace Utils;

namespace Haskell {
namespace Internal {

FileCache::FileCache(const QString &id,
                     const std::function<QList<Core::IDocument *>()> &documentsToUpdate)
    : m_tempDir(id),
      m_documentsToUpdate(documentsToUpdate)
{
    qCDebug(cacheLog) << "creating cache path at" << m_tempDir.path();
}


void FileCache::update()
{
    const QList<IDocument *> documents = m_documentsToUpdate();
    for (IDocument *document : documents) {
        const Utils::FileName filePath = document->filePath();
        if (m_fileMap.contains(filePath)) {
            // update the existing cached file
            // check revision if possible
            if (auto textDocument = qobject_cast<TextDocument *>(document)) {
                if (m_fileRevision.value(filePath) != textDocument->document()->revision())
                    writeFile(document);
            } else {
                writeFile(document);
            }
        } else {
            // save file if it is modified
            if (document->isModified())
                writeFile(document);
        }
    }
    cleanUp(documents);
}

QHash<FileName, FileName> FileCache::fileMap() const
{
    return m_fileMap;
}

void FileCache::writeFile(IDocument *document)
{
    FileName cacheFilePath = m_fileMap.value(document->filePath());
    if (cacheFilePath.isEmpty()) {
        cacheFilePath = createCacheFile(document->filePath());
        m_fileMap.insert(document->filePath(), cacheFilePath);
    }
    qCDebug(cacheLog) << "writing" << cacheFilePath;
    if (auto baseTextDocument = qobject_cast<BaseTextDocument *>(document)) {
        QString errorMessage;
        if (baseTextDocument->write(cacheFilePath.toString(),
                                    QString::fromUtf8(baseTextDocument->contents()),
                                    &errorMessage)) {
            if (auto textDocument = qobject_cast<TextDocument *>(document)) {
                m_fileRevision.insert(document->filePath(), textDocument->document()->revision());
            } else {
                m_fileRevision.insert(document->filePath(), -1);
            }
        } else {
            qCDebug(cacheLog) << "!!! writing file failed:" << errorMessage;
        }

    } else {
        QFile file(cacheFilePath.toString());
        if (file.open(QIODevice::WriteOnly)) {
            file.write(document->contents());
        } else {
            qCDebug(cacheLog) << "!!! opening file for writing failed";
        }
    }
}

void FileCache::cleanUp(const QList<IDocument *> &documents)
{
    const QSet<FileName> files = Utils::transform<QSet>(documents, &IDocument::filePath);
    auto it = QMutableHashIterator<FileName, FileName>(m_fileMap);
    while (it.hasNext()) {
        it.next();
        if (!files.contains(it.key())) {
            qCDebug(cacheLog) << "deleting" << it.value();
            QFile::remove(it.value().toString());
            m_fileRevision.remove(it.key());
            it.remove();
        }
    }
}

FileName FileCache::createCacheFile(const FileName &filePath)
{
    QTemporaryFile tempFile(m_tempDir.path() + "/XXXXXX-" + filePath.fileName());
    tempFile.setAutoRemove(false);
    tempFile.open();
    return FileName::fromString(tempFile.fileName());
}

} // namespace Internal
} // namespace Haskell
