#ifndef ROBLOXDOC_H
#define ROBLOXDOC_H

#include "DocView.h"
#include "VerbBinder.h"
#include "Workspace.h"
#include "v8datamodel/DocumentVerbs.h"

#include <memory>
#include <string>
#include <vector>

class CRobloxDoc;
class XmlElement;

class ReportAbuseVerb : public RBX::Verb
{
public:
	ReportAbuseVerb(RBX::VerbContainer* container, CRobloxDoc* document)
		: RBX::Verb(container, "ReportAbuse"), m_document(document)
	{
	}

	bool isEnabled() const override;
	void doIt(RBX::IDataState* dataState) override;

private:
	CRobloxDoc* m_document;
};

class ExitCommand : public RBX::Verb
{
public:
	ExitCommand(RBX::VerbContainer* container, CRobloxDoc* document)
		: RBX::Verb(container, "Exit"), m_document(document)
	{
	}

	void doIt(RBX::IDataState* dataState) override;

private:
	CRobloxDoc* m_document;
};

class CRobloxDoc : public Document
{
public:
	CRobloxDoc();
	~CRobloxDoc() override;

	std::shared_ptr<RBX::DataModel> dataModel() const
	{
		if (m_workspace) {
			return m_workspace->dataModel();
		}

		return std::shared_ptr<RBX::DataModel>();
	}

	const std::shared_ptr<CWorkspace>& workspace() const { return m_workspace; }
	RBX::VerbContainer* verbContainer();

	bool hasCommand(unsigned id) const override;
	bool executeCommand(unsigned id) override;
	bool updateCommand(unsigned id, CommandState& state) override;

	void setTitle(const QString& title) override;
	bool isModified() const override;
	void setModified(bool modified) override;
	void deleteContents() override;
	bool newDocument() override;
	bool openDocument(const QString& pathName) override;
	bool saveDocument(const QString& pathName) override;
	bool saveModified() override;
	bool save(const QString& pathName, bool replace = true) override;

	bool executeScriptFile(std::string fileName);
	void promptExecuteScript();

private:
	void createWorkspace();
	void bindOwnedVerb(unsigned id, RBX::Verb* verb);

	void executeCommandLine();
	void publish();
	void publishSelection();
	void reportAbuse();
	void insertLDraw();
	void saveSelection();
	void insertFromFile();
	void testSolo();
	void testOnePlayer();
	void testTwoPlayers();
	void undo();
	void redo();
	void pause();
	void reset();
	void run();
	void runsolo();
	void collectGarbage();

	void updateUndo(CommandState& state);
	void updateRedo(CommandState& state);
	void updateTimeIndicator(CommandState& state);
	void updateFpsIndicator(CommandState& state);
	void updateHasSelection(CommandState& state);
	void updatePause(CommandState& state);
	void updateReset(CommandState& state);
	void updateRun(CommandState& state);
	void updateRunSolo(CommandState& state);
	void updateCollectGarbage(CommandState& state);
	void updateThreadCount(CommandState& state);
	void updateHeapSize(CommandState& state);
	void updateReportAbuse(CommandState& state);

	static QString saveTestCopy(CRobloxDoc* document, const char* name);

	VerbBinder m_verbs;
	std::unique_ptr<XmlElement> m_resetState;
	std::vector<RBX::Verb*> m_ownedVerbs;
	std::unique_ptr<RBX::DocumentVerbs> m_commands;
	std::shared_ptr<CWorkspace> m_workspace;
	bool m_isSoloPlay = false;
};

#endif // ROBLOXDOC_H
