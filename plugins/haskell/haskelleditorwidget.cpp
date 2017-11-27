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

#include "haskelleditorwidget.h"

#include "haskelltokenizer.h"

#include <utils/textutils.h>

#include <QTextBlock>

using namespace TextEditor;

namespace Haskell {
namespace Internal {

HaskellEditorWidget::HaskellEditorWidget(QWidget *parent)
    : TextEditorWidget(parent)
{
}

Utils::optional<Token> HaskellEditorWidget::symbolAt(QTextDocument *doc, int position,
                                                     int *line, int *column)
{
    Utils::Text::convertPosition(doc, position, line, column);
    if (*line < 0 || *column < 0)
        return Utils::nullopt;
    const QTextBlock block = doc->findBlockByNumber(*line - 1);
    if (block.text().isEmpty())
        return Utils::nullopt;
    const int startState = block.previous().isValid() ? block.previous().userState() : -1;
    const Tokens tokens = HaskellTokenizer::tokenize(block.text(), startState);
    const Token token = tokens.tokenAtColumn(*column);
    if (token.isValid()
            && (token.type == TokenType::Variable
                || token.type == TokenType::Operator
                || token.type == TokenType::Constructor
                || token.type == TokenType::OperatorConstructor)) {
        return token;
    }
    return Utils::nullopt;
}

Utils::Link HaskellEditorWidget::findLinkAt(const QTextCursor &cursor,
                                            bool resolveTarget, bool inNextSplit)
{
    int line, column;
    const Utils::optional<Token> symbol = symbolAt(document(), cursor.position(), &line, &column);
    if (symbol) {
        const QTextBlock block = document()->findBlockByNumber(line - 1);
        Utils::Link link;
        link.linkTextStart = block.position() + symbol->startCol;
        link.linkTextEnd = link.linkTextStart + symbol->length;
        if (resolveTarget) {
            m_followSymbolAssistProvider.setOpenInNextSplit(inNextSplit);
            invokeAssist(FollowSymbol, &m_followSymbolAssistProvider);
        }
        return link;
    }
    return Utils::Link();
}

} // namespace Internal
} // namespace Haskell
