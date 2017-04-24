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

#include "haskellhoverhandler.h"

#include "haskelldocument.h"
#include "haskelltokenizer.h"

#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>
#include <utils/qtcassert.h>
#include <utils/runextensions.h>
#include <utils/synchronousprocess.h>
#include <utils/tooltip/tooltip.h>

#include <QTextBlock>
#include <QTextDocument>

#include <functional>

using namespace Utils;

static QString toCode(const QString &s)
{
    if (s.isEmpty())
        return s;
    return "<pre>" + s.toHtmlEscaped() + "</pre>";
}

namespace Haskell {
namespace Internal {

QString symbolToHtml(const SymbolInfo &info)
{
    if (info.definition.isEmpty())
        return QString();
    QString result = "<pre>" + info.definition.join("\n") + "</pre>";
    if (!info.file.isEmpty()) {
        result += "<div align=\"right\"><i>-- " + info.file.toString();
        if (info.line >= 0) {
            result += ":" + QString::number(info.line);
            if (info.col >= 0)
                result += ":" + QString::number(info.col);
        }
        result += "</i></div>";
    } else if (!info.module.isEmpty()) {
        result += "<div align=\"right\"><i>-- Module \"" + info.module + "\"</i></div>";
    }
    if (!info.additionalInfo.isEmpty())
        result += "<pre>" + info.additionalInfo.join("\n") + "</pre>";
    return result;
}

void HaskellHoverHandler::identifyMatch(TextEditor::TextEditorWidget *editorWidget, int pos)
{
    cancel();
    m_name.clear();
    editorWidget->convertPosition(pos, &m_line, &m_col);
    if (m_line < 0 || m_col < 0)
        return;
    QTextBlock block = editorWidget->document()->findBlockByNumber(m_line - 1);
    if (block.text().isEmpty())
        return;
    m_filePath = editorWidget->textDocument()->filePath();
    const int startState = block.previous().isValid() ? block.previous().userState() : -1;
    const Tokens tokens = HaskellTokenizer::tokenize(block.text(), startState);
    const Token token = tokens.tokenAtColumn(m_col);
    if (token.isValid()
            && (token.type == TokenType::Variable
                || token.type == TokenType::Operator
                || token.type == TokenType::Constructor
                || token.type == TokenType::OperatorConstructor)) {
       m_name = token.text.toString();
    }
    if (m_name.isEmpty())
        setPriority(-1);
    else
        setPriority(Priority_Tooltip);
}

static void tryShowToolTip(const QPointer<QWidget> &widget, const QPoint &point,
                           QFuture<Utils::optional<QString>> typeFuture,
                           QFuture<Utils::optional<SymbolInfo>> symbolFuture)
{
    if (Utils::ToolTip::isVisible() && widget
            && symbolFuture.isResultReadyAt(0) && typeFuture.isResultReadyAt(0)) {
        const Utils::optional<QString> type = typeFuture.result();
        const Utils::optional<SymbolInfo> info = symbolFuture.result();
        const QString typeString = !type || type.value().isEmpty()
                ? QString()
                : toCode(":: " + type.value());
        const QString infoString = info ? symbolToHtml(info.value()) : QString();
        const QString tip = typeString + infoString;
        Utils::ToolTip::show(point, tip, widget);
    }
}

void HaskellHoverHandler::operateTooltip(TextEditor::TextEditorWidget *editorWidget,
                                         const QPoint &point)
{
    cancel();
    if (m_name.isEmpty()) {
        Utils::ToolTip::hide();
        return;
    }
    Utils::ToolTip::show(point, tr("Looking up \"%1\"").arg(m_name), editorWidget);

    QPointer<QWidget> widget = editorWidget;
    std::shared_ptr<AsyncGhcMod> ghcmod;
    auto doc = qobject_cast<HaskellDocument *>(editorWidget->textDocument());
    QTC_ASSERT(doc, return);
    ghcmod = doc->ghcMod();
    m_typeFuture = ghcmod->typeInfo(m_filePath, m_line, m_col);
    m_symbolFuture = ghcmod->findSymbol(m_filePath, m_name);
    Utils::onResultReady(m_typeFuture,
                         [typeFuture = m_typeFuture, symbolFuture = m_symbolFuture,
                          ghcmod, widget, point] // hold shared ghcmod pointer
                         (const Utils::optional<QString> &) {
                             tryShowToolTip(widget, point, typeFuture, symbolFuture);
                         });
    Utils::onResultReady(m_symbolFuture,
                         [typeFuture = m_typeFuture, symbolFuture = m_symbolFuture,
                          ghcmod, widget, point] // hold shared ghcmod pointer
                         (const Utils::optional<SymbolInfo> &) {
                             tryShowToolTip(widget, point, typeFuture, symbolFuture);
                         });
}

void HaskellHoverHandler::cancel()
{
    if (m_symbolFuture.isRunning())
        m_symbolFuture.cancel();
    if (m_typeFuture.isRunning())
        m_typeFuture.cancel();
}

} // namespace Internal
} // namespace Haskell
