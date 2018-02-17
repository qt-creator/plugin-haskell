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
#include "haskelleditorwidget.h"
#include "haskellmanager.h"
#include "haskelltokenizer.h"

#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>
#include <utils/qtcassert.h>
#include <utils/runextensions.h>
#include <utils/synchronousprocess.h>
#include <utils/tooltip/tooltip.h>

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

void HaskellHoverHandler::identifyMatch(TextEditor::TextEditorWidget *editorWidget,
                                        int pos,
                                        ReportPriority report)
{
    cancel();
    m_name.clear();
    m_filePath.clear();
    const Utils::optional<Token> token = HaskellEditorWidget::symbolAt(editorWidget->document(),
                                                                       pos, &m_line, &m_col);
    if (token) {
        m_filePath = editorWidget->textDocument()->filePath();
        m_name = token->text.toString();
        setPriority(Priority_Tooltip);
    } else {
        setPriority(-1);
    }
    report(priority());
}

static void showError(const QPointer<TextEditor::TextEditorWidget> &widget,
                      const Error &typeError, const Error &infoError)
{
    if (typeError.type == Error::Type::FailedToStartStack
            || infoError.type == Error::Type::FailedToStartStack) {
        const QString stackExecutable = typeError.type == Error::Type::FailedToStartStack
                                            ? typeError.details
                                            : infoError.details;
        HaskellEditorWidget::showFailedToStartStackError(stackExecutable, widget);
    }
}

static void tryShowToolTip(const QPointer<TextEditor::TextEditorWidget> &widget, const QPoint &point,
                           QFuture<QStringOrError> typeFuture,
                           QFuture<SymbolInfoOrError> symbolFuture)
{
    if (Utils::ToolTip::isVisible() && widget
            && symbolFuture.isResultReadyAt(0) && typeFuture.isResultReadyAt(0)) {
        const QStringOrError typeOrError = typeFuture.result();
        const SymbolInfoOrError infoOrError = symbolFuture.result();
        if (const Error *typeError = Utils::get_if<Error>(&typeOrError))
            if (const Error *infoError = Utils::get_if<Error>(&infoOrError))
                showError(widget, *typeError, *infoError);
        const QString *type = Utils::get_if<QString>(&typeOrError);
        const SymbolInfo *info = Utils::get_if<SymbolInfo>(&infoOrError);
        const QString typeString = !type || type->isEmpty()
                ? QString()
                : toCode(":: " + *type);
        const QString infoString = info ? symbolToHtml(*info) : QString();
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
    Utils::ToolTip::show(point, HaskellManager::trLookingUp(m_name), editorWidget);

    QPointer<TextEditor::TextEditorWidget> widget = editorWidget;
    std::shared_ptr<AsyncGhcMod> ghcmod;
    auto doc = qobject_cast<HaskellDocument *>(editorWidget->textDocument());
    QTC_ASSERT(doc, return);
    ghcmod = doc->ghcMod();
    m_typeFuture = ghcmod->typeInfo(m_filePath, m_line, m_col);
    m_symbolFuture = ghcmod->findSymbol(m_filePath, m_name);
    Utils::onResultReady(m_typeFuture,
                         [typeFuture = m_typeFuture, symbolFuture = m_symbolFuture,
                          ghcmod, widget, point] // hold shared ghcmod pointer
                         (const QStringOrError &) {
                             tryShowToolTip(widget, point, typeFuture, symbolFuture);
                         });
    Utils::onResultReady(m_symbolFuture,
                         [typeFuture = m_typeFuture, symbolFuture = m_symbolFuture,
                          ghcmod, widget, point] // hold shared ghcmod pointer
                         (const SymbolInfoOrError &) {
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
