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

#include "followsymbol.h"

#include "haskelleditorwidget.h"
#include "haskellmanager.h"
#include "haskelltokenizer.h"

#include <coreplugin/editormanager/editormanager.h>
#include <texteditor/codeassist/assistinterface.h>
#include <texteditor/codeassist/assistproposalitem.h>
#include <texteditor/codeassist/genericproposal.h>
#include <texteditor/codeassist/genericproposalmodel.h>

#include <utils/qtcassert.h>

using namespace TextEditor;
using namespace Utils;

namespace Haskell {
namespace Internal {

IAssistProvider::RunType FollowSymbolAssistProvider::runType() const
{
    return AsynchronousWithThread;
}

IAssistProcessor *FollowSymbolAssistProvider::createProcessor() const
{
    return new FollowSymbolAssistProcessor(m_inNextSplit);
}

void FollowSymbolAssistProvider::setOpenInNextSplit(bool inNextSplit)
{
    m_inNextSplit = inNextSplit;
}

FollowSymbolAssistProcessor::FollowSymbolAssistProcessor(bool inNextSplit)
    : m_inNextSplit(inNextSplit)
{
}

IAssistProposal *FollowSymbolAssistProcessor::immediateProposal(const AssistInterface *interface)
{
    int line, column;
    const optional<Token> symbol
            = HaskellEditorWidget::symbolAt(interface->textDocument(), interface->position(),
                                            &line, &column);
    QTC_ASSERT(symbol, return nullptr); // should have been checked before
    const auto filePath = FileName::fromString(interface->fileName());
    m_ghcmod = HaskellManager::ghcModForFile(filePath);
    m_symbolFuture = m_ghcmod->findSymbol(filePath, symbol->text.toString());

    auto item = new AssistProposalItem();
    item->setText(HaskellManager::trLookingUp(symbol->text.toString()));
    item->setData(QString());
    item->setOrder(-1000);

    const QList<TextEditor::AssistProposalItemInterface *> list = {item};
    auto proposal = new GenericProposal(interface->position(), list);
    proposal->setFragile(true);
    return proposal;
}

IAssistProposal *FollowSymbolAssistProcessor::perform(const AssistInterface *interface)
{
    const int position = interface->position();
    delete interface;
    const SymbolInfoOrError info = m_symbolFuture.result();
    auto item = new FollowSymbolAssistProposalItem(m_ghcmod->basePath(), info, m_inNextSplit);
    return new InstantProposal(position, {item});
}

FollowSymbolAssistProposalItem::FollowSymbolAssistProposalItem(const FileName &basePath,
                                                               const SymbolInfoOrError &info,
                                                               bool inNextSplit)
    : m_basePath(basePath),
      m_inNextSplit(inNextSplit)
{
    const SymbolInfo *info_p = Utils::get_if<SymbolInfo>(&info);
    if (info_p && !info_p->file.isEmpty()) {
        m_info = info;
        setText(m_basePath.toString() + '/' + info_p->file.toString());
    }
}

void FollowSymbolAssistProposalItem::apply(TextDocumentManipulatorInterface &, int) const
{
    const SymbolInfo *info_p = Utils::get_if<SymbolInfo>(&m_info);
    if (info_p)
        Core::EditorManager::openEditorAt(m_basePath.toString() + '/' + info_p->file.toString(),
                                          info_p->line, info_p->col - 1, Core::Id(),
                                          m_inNextSplit ? Core::EditorManager::OpenInOtherSplit
                                                        : Core::EditorManager::NoFlags);
}

void InstantActivationProposalWidget::showProposal(const QString &prefix)
{
    if (model() && model()->size() == 1) {
        emit proposalItemActivated(model()->proposalItem(0));
        deleteLater();
        return;
    }
    GenericProposalWidget::showProposal(prefix);
}

InstantProposal::InstantProposal(int cursorPos, const QList<AssistProposalItemInterface *> &items)
    : GenericProposal(cursorPos, items)
{
}

IAssistProposalWidget *InstantProposal::createWidget() const
{
    return new InstantActivationProposalWidget();
}

} // namespace Internal
} // namespace Haskell
