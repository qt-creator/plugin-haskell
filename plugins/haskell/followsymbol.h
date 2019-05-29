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

#include "ghcmod.h"

#include <texteditor/codeassist/assistproposalitem.h>
#include <texteditor/codeassist/genericproposal.h>
#include <texteditor/codeassist/genericproposalwidget.h>
#include <texteditor/codeassist/iassistprocessor.h>
#include <texteditor/codeassist/iassistprovider.h>

namespace Haskell {
namespace Internal {

class FollowSymbolAssistProposalItem : public TextEditor::AssistProposalItem
{
public:
    FollowSymbolAssistProposalItem(const Utils::FilePath &basePath,
                                   const SymbolInfoOrError &info,
                                   bool inNextSplit);

    void apply(TextEditor::TextDocumentManipulatorInterface &, int) const override;

private:
    Utils::FilePath m_basePath;
    SymbolInfoOrError m_info;
    bool m_inNextSplit;
};

class InstantActivationProposalWidget : public TextEditor::GenericProposalWidget
{
protected:
    void showProposal(const QString &prefix) override;
};

class InstantProposal : public TextEditor::GenericProposal
{
public:
    InstantProposal(int cursorPos, const QList<TextEditor::AssistProposalItemInterface *> &items);

    TextEditor::IAssistProposalWidget *createWidget() const override;
};

class FollowSymbolAssistProvider : public TextEditor::IAssistProvider
{
public:
    RunType runType() const override;
    TextEditor::IAssistProcessor *createProcessor() const override;
    void setOpenInNextSplit(bool inNextSplit);

private:
    bool m_inNextSplit = false;
};

class FollowSymbolAssistProcessor : public TextEditor::IAssistProcessor
{
public:
    FollowSymbolAssistProcessor(bool inNextSplit);

    TextEditor::IAssistProposal *immediateProposal(const TextEditor::AssistInterface *interface) override;
    TextEditor::IAssistProposal *perform(const TextEditor::AssistInterface *interface) override;

private:
    std::shared_ptr<AsyncGhcMod> m_ghcmod;
    QFuture<SymbolInfoOrError> m_symbolFuture;
    bool m_inNextSplit;
};

} // namespace Internal
} // namespace Haskell
