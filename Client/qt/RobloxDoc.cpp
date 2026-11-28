#include "RobloxDoc.h"

#include "ActiveDocView.h"
#include "ClipboardVerbs.h"
#include "CommandIds.h"
#include "External.h"
#include "FileDialog.h"
#include "HtmlDialog.h"
#include "InstanceExplorer.h"
#include "LDrawParser.h"
#include "MainFrm.h"
#include "Resources.h"
#include "Roblox.h"
#include "SystemMessageBox.h"
#include "network/Players.h"
#include "script/LuaMemory.h"
#include "script/ScriptContext.h"
#include "util/ContentProvider.h"
#include "util/FileSystem.h"
#include "util/Log.h"
#include "util/RunStateOwner.h"
#include "util/Sound.h"
#include "util/Utilities.h"
#include "util/standardout.h"
#include "v8datamodel/Selection.h"
#include "v8datamodel/TimeState.h"
#include "v8datamodel/Visit.h"
#include "v8datamodel/Workspace.h"
#include "v8xml/SerializerV2.h"
#include "v8xml/XmlSerializer.h"

#include <QApplication>
#include <QComboBox>
#include <QDir>
#include <QFile>
#include <QLineEdit>
#include <QProcess>
#include <QStandardPaths>

#ifdef _WIN32
#include <shlobj.h>
#include <windows.h>
#endif

#include <cstdio>
#include <sstream>

static const unsigned reportAbuseTemplateId = 123;
static const unsigned undoStringId = 130;
static const unsigned redoStringId = 131;
static const unsigned ldrawFilterStringId = 141;
static const unsigned modelFilterStringId = 184;
static const unsigned scriptFilterStringId = 208;

static bool canSave(RBX::DataModel* dataModel)
{
	return !RBX::Network::Players::clientIsPresent(dataModel, true) ||
		   RBX::Network::Players::serverIsPresent(dataModel, true);
}

bool ReportAbuseVerb::isEnabled() const
{
	RBX::DataModel::Lock lock(m_document->dataModel());
	return m_document->dataModel()->create<RBX::Network::Players>()->canReportAbuse();
}

void ReportAbuseVerb::doIt(RBX::IDataState* dataState)
{
	theApp.mainFrame()->postCommand(ID_TOOLS_REPORT_ABUSE);
}

void ExitCommand::doIt(RBX::IDataState* dataState)
{
	CMainFrame* mainFrame = theApp.mainFrame();

	if (mainFrame->isGameLayout()) {
		QMetaObject::invokeMethod(mainFrame, [mainFrame]() { mainFrame->close(); }, Qt::QueuedConnection);
		return;
	}

	if (!m_document->views().empty()) {
		View* view = m_document->views().front();
		QMetaObject::invokeMethod(
			view, [view]() { routeCommand({view, view->document()}, ID_FILE_CLOSE); }, Qt::QueuedConnection
		);
	}
}

void CRobloxDoc::saveSelection()
{
	CFileDialog dialog(
		false,
		QStringLiteral("rbxm"),
		QString(),
		CFileDialog::HideReadOnly | CFileDialog::OverwritePrompt,
		Resources::string(modelFilterStringId)
	);
	if (dialog.DoModal() == IDOK) {
		m_workspace->SaveSelectionToFile(dialog.GetPathName().toStdString());
	}
}

bool CRobloxDoc::save(const QString& pathName, bool replace)
{
	if (!canSave(m_workspace->dataModel().get())) {
		return false;
	}

	return Document::save(pathName, replace);
}

bool CRobloxDoc::saveDocument(const QString& pathName)
{
	if (!canSave(m_workspace->dataModel().get())) {
		return false;
	}

	if (!m_workspace->SaveToFile(QDir::toNativeSeparators(pathName).toStdString())) {
		return false;
	}

	setModified(false);
	return true;
}

void CRobloxDoc::setModified(bool modified)
{
	Document::setModified(modified);
	if (m_workspace) {
		m_workspace->dataModel()->setDirty(modified);
	}
}

void CRobloxDoc::setTitle(const QString& title)
{
	Document::setTitle(title);
	if (m_workspace) {
		m_workspace->dataModel()->setName(title.toStdString());
	}
}

QString CRobloxDoc::saveTestCopy(CRobloxDoc* doc, const char* name)
{
	QString path = QString::fromStdString(RBX::FileSystem::getUserAppDataDirectory(true, "Test"));
#ifdef _WIN32
	char shortPath[MAX_PATH];
	if (GetShortPathNameA(path.toLocal8Bit().constData(), shortPath, MAX_PATH)) {
		path = QString::fromLocal8Bit(shortPath);
	}
#endif
	path += name;
	path += ".rbxl";

	if (!doc->m_workspace->SaveToFile(path.toStdString())) {
		throw std::runtime_error("Failed to copy document");
	}

	return path;
}

static void launchClient(const QString& file, const QString& arguments)
{
	QString folder;
#ifdef _WIN32
	char path[MAX_PATH];
	if (SHGetFolderPathA(nullptr, CSIDL_COMMON_DOCUMENTS, nullptr, SHGFP_TYPE_CURRENT, path)) {
		throw std::runtime_error("Unable to get common document folder");
	}
	folder = QString::fromLocal8Bit(path);
#else
	folder = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
#endif

	QStringList parameters;
	if (!file.isEmpty()) {
		parameters.append(file);
	}
	parameters.append(QProcess::splitCommand(arguments));

	if (!QProcess::startDetached(QCoreApplication::applicationFilePath(), parameters, folder)) {
		throw std::runtime_error("ShellExecute failed");
	}
}

void CRobloxDoc::testSolo()
{
	try {
		launchClient(saveTestCopy(this, "Visit"), " -script " + CRobloxApp::baseUrl() + "/game/visit.ashx");
	}
	catch (std::exception& e) {
		SystemMessageBox::warning(theApp.mainFrame(), Resources::string(appTitleStringId), e.what());
	}
}

void CRobloxDoc::testOnePlayer()
{
	try {
		launchClient(saveTestCopy(this, "Server"), " -script " + CRobloxApp::baseUrl() + "/game/gameserver.ashx");
		launchClient(QString(), " -script " + CRobloxApp::baseUrl() + "/game/join.ashx");
	}
	catch (std::exception& e) {
		SystemMessageBox::warning(theApp.mainFrame(), Resources::string(appTitleStringId), e.what());
	}
}

void CRobloxDoc::testTwoPlayers()
{
	try {
		launchClient(saveTestCopy(this, "Server"), " -script " + CRobloxApp::baseUrl() + "/game/gameserver.ashx");
		launchClient(QString(), " -script " + CRobloxApp::baseUrl() + "/game/join.ashx");
		launchClient(QString(), " -script " + CRobloxApp::baseUrl() + "/game/join.ashx?UserID=0&port=4344");
	}
	catch (std::exception& e) {
		SystemMessageBox::warning(theApp.mainFrame(), Resources::string(appTitleStringId), e.what());
	}
}

RBX::VerbContainer* CRobloxDoc::verbContainer()
{
	return dataModel()->getWorkspace();
}

void CRobloxDoc::undo()
{
	RBX::DataModel::Lock lock(m_workspace->dataModel());
	m_commands->m_undo.doIt(m_workspace->dataModel().get());
}

void CRobloxDoc::updateUndo(CommandState& state)
{
	std::shared_ptr<RBX::DataModel> dataModel = m_workspace->dataModel();
	std::string name;
	{
		RBX::DataModel::Lock lock(dataModel);
		state.enabled = dataModel->canUndo(name);
	}

	state.text = Resources::string(undoStringId).replace("%s", QString::fromStdString(name));
}

void CRobloxDoc::redo()
{
	RBX::DataModel::Lock lock(m_workspace->dataModel());
	m_commands->m_redo.doIt(m_workspace->dataModel().get());
}

void CRobloxDoc::updateRedo(CommandState& state)
{
	std::shared_ptr<RBX::DataModel> dataModel = m_workspace->dataModel();
	std::string name;
	{
		RBX::DataModel::Lock lock(dataModel);
		state.enabled = dataModel->canRedo(name);
	}

	state.text = Resources::string(redoStringId).replace("%s", QString::fromStdString(name));
}

bool CRobloxDoc::isModified() const
{
	if (dataModel()) {
		RBX::DataModel::Lock lock(dataModel());
		return dataModel()->isDirty();
	}

	return false;
}

void CRobloxDoc::insertFromFile()
{
	CFileDialog dialog(
		true,
		QStringLiteral("rbxm"),
		QString(),
		CFileDialog::HideReadOnly | CFileDialog::OverwritePrompt,
		Resources::string(modelFilterStringId)
	);
	if (dialog.DoModal() == IDOK) {
		RBX::DataModel::Lock lock(m_workspace->dataModel());
		m_workspace->insertContent(RBX::ContentId::fromFile(dialog.GetPathName().toStdString().c_str()), true);
	}
}

void CRobloxDoc::deleteContents()
{
	{
		RBX::DataModel::Lock lock(dataModel());
		for (RBX::Verb* verb : m_ownedVerbs) {
			delete verb;
		}
		m_ownedVerbs.clear();
		m_commands.reset();
	}

	if (m_workspace) {
		m_workspace->closeDataModel();
		m_workspace.reset();
	}

	if (ActiveDocView::activeDocument() == this) {
		ActiveDocView::activate(true, this);
	}

	Document::deleteContents();
}

CRobloxDoc::~CRobloxDoc() = default;

void CRobloxDoc::updateTimeIndicator(CommandState& state)
{
	if (dataModel()) {
		char text[32];
		{
			RBX::DataModel::Lock lock(m_workspace->dataModel());
			std::snprintf(text, sizeof(text), "t %.4g", dataModel()->getSimTime());
		}
		state.text = text;
	}
}

void CRobloxDoc::bindOwnedVerb(unsigned id, RBX::Verb* verb)
{
	if (verb) {
		m_ownedVerbs.push_back(verb);
	}

	m_verbs.bind(id, verb);
}

CRobloxDoc::CRobloxDoc() : m_verbs([this]() { return dataModel(); }, [this]() { return verbContainer(); })
{
	addCommand(ID_FILE_PUBLISH, [this]() { publish(); });
	addCommand(ID_FILE_PUBLISH_SELECTION, [this]() { publishSelection(); });
	addCommand(ID_TOOLS_REPORT_ABUSE, [this]() { reportAbuse(); });
	addCommand(ID_COMMAND_LINE, [this]() { executeCommandLine(); });
	addCommand(ID_TOOLS_EXECUTE_SCRIPT, [this]() { promptExecuteScript(); });
	addCommand(ID_INSERT_LDRAW, [this]() { insertLDraw(); });
	addCommand(ID_TEST_SOLO, [this]() { testSolo(); });
	addCommand(ID_TEST_1_PLAYER, [this]() { testOnePlayer(); });
	addCommand(ID_TEST_2_PLAYERS, [this]() { testTwoPlayers(); });
	addCommand(ID_SELECTION_INSERT_FROM_FILE, [this]() { insertFromFile(); });
	addCommand(ID_FILE_SAVE_SELECTION, [this]() { saveSelection(); });
	addCommand(ID_EDIT_UNDO, [this]() { undo(); });
	addCommand(ID_EDIT_REDO, [this]() { redo(); });
	addCommand(ID_RUN_PAUSE, [this]() { pause(); });
	addCommand(ID_RUN_RESET, [this]() { reset(); });
	addCommand(ID_RUN_RUN, [this]() { run(); });	
	addCommand(ID_RUN_SOLO, [this]() { runsolo(); });
	addCommand(ID_TOOLS_GC, [this]() { collectGarbage(); });

	for (unsigned id :
		 {ID_COMMAND_LINE,
		  ID_TOOLS_EXECUTE_SCRIPT,
		  ID_INSERT_LDRAW,
		  ID_TEST_SOLO,
		  ID_TEST_1_PLAYER,
		  ID_TEST_2_PLAYERS}) {
		addCommandUpdate(id, [](CommandState& state) { state.enabled = true; });
	}
	addCommandUpdate(ID_FILE_PUBLISH, [](CommandState& state) { state.enabled = true; });
	addCommandUpdate(ID_FILE_PUBLISH_SELECTION, [this](CommandState& state) { updateHasSelection(state); });
	addCommandUpdate(ID_INDICATOR_TIME, [this](CommandState& state) { updateTimeIndicator(state); });
	addCommandUpdate(ID_INDICATOR_FPS, [this](CommandState& state) { updateFpsIndicator(state); });
	addCommandUpdate(ID_TOOLS_REPORT_ABUSE, [this](CommandState& state) { updateReportAbuse(state); });
	addCommandUpdate(ID_FILE_SAVE_SELECTION, [this](CommandState& state) { updateHasSelection(state); });
	addCommandUpdate(ID_EDIT_UNDO, [this](CommandState& state) { updateUndo(state); });
	addCommandUpdate(ID_EDIT_REDO, [this](CommandState& state) { updateRedo(state); });
	addCommandUpdate(ID_RUN_PAUSE, [this](CommandState& state) { updatePause(state); });
	addCommandUpdate(ID_RUN_RESET, [this](CommandState& state) { updateReset(state); });
	addCommandUpdate(ID_RUN_RUN, [this](CommandState& state) { updateRun(state); });
	addCommandUpdate(ID_RUN_SOLO, [this](CommandState& state) { updateRunSolo(state); });	
	addCommandUpdate(ID_TOOLS_GC, [this](CommandState& state) { updateCollectGarbage(state); });
	addCommandUpdate(ID_COMMAND_THREADS, [this](CommandState& state) { updateThreadCount(state); });
	addCommandUpdate(ID_COMMAND_HEAP, [this](CommandState& state) { updateHeapSize(state); });
}

bool CRobloxDoc::hasCommand(unsigned id) const
{
	return m_verbs.has(id) || Document::hasCommand(id);
}

bool CRobloxDoc::executeCommand(unsigned id)
{
	return m_verbs.execute(id) || Document::executeCommand(id);
}

bool CRobloxDoc::updateCommand(unsigned id, CommandState& state)
{
	return m_verbs.update(id, state) || Document::updateCommand(id, state);
}

void CRobloxDoc::createWorkspace()
{
	m_workspace = std::make_shared<CWorkspace>();

	std::shared_ptr<RBX::DataModel> dataModel = m_workspace->dataModel();
	RBX::DataModel::Lock lock(dataModel);

	dataModel->setName(title().toStdString());

	m_commands.reset(new RBX::DocumentVerbs(dataModel.get()));

	m_ownedVerbs.push_back(new ReportAbuseVerb(dataModel.get(), this));
	m_ownedVerbs.push_back(new ExitCommand(dataModel.get(), this));
	m_ownedVerbs.push_back(new RBX::FixedCameraCommand(m_workspace->workspace()));
	m_ownedVerbs.push_back(new RBX::CameraPanLeftCommand(m_workspace->workspace()));
	m_ownedVerbs.push_back(new RBX::CameraPanRightCommand(m_workspace->workspace()));
	m_ownedVerbs.push_back(new RBX::CameraTiltUpCommand(m_workspace->workspace()));
	m_ownedVerbs.push_back(new RBX::CameraTiltDownCommand(m_workspace->workspace()));
	m_ownedVerbs.push_back(new RBX::CameraZoomInCommand(m_workspace->workspace()));
	m_ownedVerbs.push_back(new RBX::CameraZoomOutCommand(m_workspace->workspace()));
	m_ownedVerbs.push_back(new RBX::CameraCenterCommand(m_workspace->workspace()));
	m_ownedVerbs.push_back(new RBX::CameraZoomExtentsCommand(m_workspace->workspace()));
	m_ownedVerbs.push_back(new RBX::CameraModelViewCommand(m_workspace->workspace()));

	m_verbs.bind(ID_EDIT_CLEAR, &m_commands->m_deleteSelection);
	m_verbs.bind(ID_EDIT_SELECT_ALL, &m_commands->m_selectAll);
	m_verbs.bind(ID_EDIT_DUPLICATE, &m_commands->m_duplicateSelection);
	m_verbs.bind(ID_EDIT_UNLOCK_ALL, &m_commands->m_allCanSelect);
	m_verbs.bind(ID_EDIT_LOCK, &m_commands->m_canNotSelect);
	m_verbs.bind(ID_TOOLS_JOIN_BRICKS, &m_commands->m_join);
	m_verbs.bind(ID_TOOLS_FORCE_ASSERTION, &m_commands->m_forceAssertion);
	m_verbs.bind(ID_TOOLS_ENGINE_STATS, &m_commands->m_engineStats);
	m_verbs.bind(ID_FORMAT_GROUP, RBX::Name::lookup("Group"));
	m_verbs.bind(ID_FORMAT_UNGROUP, RBX::Name::lookup("UnGroup"));
	m_verbs.bind(ID_FORMAT_GLUE_TO_WELDS, RBX::Name::lookup("WeldSelection"));
	m_verbs.bind(ID_FORMAT_WELDS_TO_SNAPS, RBX::Name::lookup("SnapSelection"));
	m_verbs.bind(ID_FORMAT_ROTATE, RBX::Name::lookup("SelectionRotate"));
	m_verbs.bind(ID_FORMAT_TILT, RBX::Name::lookup("SelectionTilt"));
	m_verbs.bind(ID_SELECTION_UP_BRICK, RBX::Name::lookup("SelectionUpBrick"));
	m_verbs.bind(ID_SELECTION_UP_PLATE, RBX::Name::lookup("SelectionUpPlate"));
	m_verbs.bind(ID_SELECTION_DOWN, RBX::Name::lookup("SelectionDown"));
	m_verbs.bind(ID_FORMAT_SET_MODEL_FRONT, &m_commands->m_modelSetFrontTool);
	m_verbs.bind(ID_FORMAT_SET_PRIMARY_PART, &m_commands->m_modelSetPrimaryPartTool);
	bindOwnedVerb(ID_EDIT_CUT, new CutVerb(dataModel.get()));
	bindOwnedVerb(ID_EDIT_COPY, new CopyVerb(dataModel.get()));
	bindOwnedVerb(ID_EDIT_PASTE, new PasteVerb(dataModel.get(), false));
	bindOwnedVerb(ID_SELECTION_PASTE_INTO, new PasteVerb(dataModel.get(), true));
	bindOwnedVerb(ID_INSERT_DECAL, new InsertDecal(dataModel.get(), "InsertDecal"));
	bindOwnedVerb(ID_INSERT_TEXTURE, new InsertTexture(dataModel.get()));
	bindOwnedVerb(ID_INSERT_OBJECT, new InsertObject(dataModel.get()));
	m_verbs.bind(ID_INSERT_CLEAR_STARTERPACK_TOOLS, &m_commands->m_clearStarterpack);
	m_verbs.bind(ID_INSERT_CLEAR_BACKPACK_TOOLS, &m_commands->m_clearBackpack);
	m_verbs.bind(ID_TOOL_FILL, RBX::Name::lookup("FillTool"));
	m_verbs.bind(ID_TOOL_DROPPER, RBX::Name::lookup("DropperTool"));
	m_verbs.bind(ID_TOOL_DRAG, &m_commands->m_arrowTool);
	m_verbs.bind(ID_TOOL_RESIZE, &m_commands->m_resizeTool);
	m_verbs.bind(ID_TOOL_MOVE_AXIS, &m_commands->m_axisMoveTool);
	m_verbs.bind(ID_TOOL_LOCK, &m_commands->m_lockTool);
	m_verbs.bind(ID_TOOL_ANCHOR, &m_commands->m_anchorTool);
	m_verbs.bind(ID_TOOL_GLUE, &m_commands->m_glueTool);
	m_verbs.bind(ID_TOOL_SMOOTH, &m_commands->m_flatTool);
	m_verbs.bind(ID_TOOL_WELD, &m_commands->m_weldTool);
	m_verbs.bind(ID_TOOL_STUDS, &m_commands->m_studsTool);
	m_verbs.bind(ID_TOOL_INLET, &m_commands->m_inletTool);
	m_verbs.bind(ID_TOOL_MOTOR, &m_commands->m_rightMotorTool);
	m_verbs.bind(ID_TOOL_HINGE, &m_commands->m_hingeTool);
}

bool CRobloxDoc::openDocument(const QString& pathName)
{
#ifdef _WIN32
	char cachePath[MAX_PATH];
	if (SHGetFolderPathA(nullptr, CSIDL_INTERNET_CACHE, nullptr, 0, cachePath) == S_OK) {
		QString relativePath = QDir(QString::fromLocal8Bit(cachePath)).relativeFilePath(pathName);
		if (!relativePath.startsWith("..") && !QDir::isAbsolutePath(relativePath)) {
			return false;
		}
	}
#endif

	deleteContents();
	createWorkspace();
	setModified(true);

	try {
		std::unique_ptr<std::istream> stream = RBX::ContentProvider::singleton().getContent(
			RBX::ContentId::fromFile(QDir::toNativeSeparators(pathName).toStdString().c_str())
		);
		m_workspace->load(*stream);

		setModified(false);
	}
	catch (std::exception& e) {
		RBX::StandardOut::singleton()->print(RBX::MESSAGE_ERROR, e);
		SystemMessageBox::warning(
			theApp.mainFrame(),
			Resources::string(appTitleStringId),
			QStringLiteral("Unable to open document %1.\n%2").arg(pathName, e.what())
		);
		return false;
	}

	return true;
}

bool CRobloxDoc::newDocument()
{
	if (!Document::newDocument()) {
		return false;
	}

	try {
		createWorkspace();
	}
	catch (std::exception& e) {
		RBX::StandardOut::singleton()->print(RBX::MESSAGE_ERROR, e);
		SystemMessageBox::warning(
			theApp.mainFrame(), Resources::string(appTitleStringId), "An error occured while creating the document"
		);
		return false;
	}

	return true;
}

void CRobloxDoc::updateHasSelection(CommandState& state)
{
	RBX::DataModel::Lock lock(m_workspace->dataModel());
	RBX::Selection* selection = RBX::ServiceProvider::create<RBX::Selection>(m_workspace->dataModel().get());
	state.enabled = selection->size() > 0;
}

void CRobloxDoc::publish()
{
	CPublishAsPlaceDialog dialog(m_workspace, nullptr);
	dialog.DoModal();
}

void CRobloxDoc::publishSelection()
{
	CSaveModelToRobloxDialog dialog(m_workspace, nullptr);
	dialog.DoModal();
}

void CRobloxDoc::reportAbuse()
{
	CRobloxDHtmlDialog dialog(reportAbuseTemplateId, CRobloxApp::baseUrl() + "/AbuseReport/InGameChat.aspx", nullptr);
	dialog.setExternal(std::make_shared<CWorkspaceExternal>(m_workspace));
	dialog.DoModal();
}

void CRobloxDoc::executeCommandLine()
{
	QComboBox* commandLine = theApp.mainFrame()->commandLine();
	std::string text = commandLine->currentText().toStdString();

	if (commandLine->findText(commandLine->currentText()) == -1) {
		commandLine->insertItem(0, commandLine->currentText());
		if (commandLine->count() > 10) {
			commandLine->removeItem(commandLine->count() - 1);
		}
	}

	commandLine->setEnabled(false);
	RBX::StandardOut::singleton()->print(RBX::MESSAGE_OUTPUT, "> %s", text.c_str());

	try {
		if (RBX::ContentProvider::isHttpUrl(text)) {
			std::unique_ptr<std::istream> stream = RBX::ContentProvider::singleton().getContent(RBX::ContentId(text));
			text = RBX::readStream(*stream);
		}

		RBX::DataModel::Lock lock(dataModel());
		std::vector<RBX::Reflection::Value> arguments;
		m_workspace->dataModel()->create<RBX::ScriptContext>()->executeInNewThread(
			RBX::Security::CmdLine, text.c_str(), "Cmd", arguments
		);
	}
	catch (std::exception&) {
	}

	commandLine->setEnabled(true);
	commandLine->lineEdit()->selectAll();
	commandLine->lineEdit()->setFocus();
}

void CRobloxDoc::updatePause(CommandState& state)
{
	RBX::DataModel::Lock lock(m_workspace->dataModel());
	state.enabled = dataModel()->create<RBX::RunService>()->getRunState() == RBX::RS_RUNNING;
}

void CRobloxDoc::updateReset(CommandState& state)
{
	RBX::DataModel::Lock lock(m_workspace->dataModel());
	RBX::RunState runState = dataModel()->create<RBX::RunService>()->getRunState();
	// Disable reset only when acting as a real network client (joined a server).
	// Solo play and plain run() both run entirely in-editor, so reset is always
	// available there regardless of whether a local player exists.
	bool isNetworkClient = !m_isSoloPlay &&
	                       RBX::Network::Players::clientIsPresent(dataModel().get(), true);
	state.enabled = runState != RBX::RS_NORMAL && !isNetworkClient;
}

void CRobloxDoc::updateRun(CommandState& state)
{
	RBX::DataModel::Lock lock(m_workspace->dataModel());
	RBX::RunService* runService = dataModel()->create<RBX::RunService>();
	state.enabled = !runService->runDisabled && !runService->isRunState();
}

void CRobloxDoc::updateRunSolo(CommandState& state)
{
	RBX::DataModel::Lock lock(m_workspace->dataModel());
	RBX::RunService* runService = dataModel()->create<RBX::RunService>();
	// Solo play is only available when the simulation is fully stopped (RS_NORMAL)
	// and no network session is active.
	bool networkClientPresent = RBX::Network::Players::clientIsPresent(dataModel().get(), true);
	state.enabled = !runService->runDisabled && !runService->isRunState() && !networkClientPresent;
}


void CRobloxDoc::updateFpsIndicator(CommandState& state)
{
	if (dataModel()) {
		RBX::DataModel::Lock lock(m_workspace->dataModel());
		RBX::RunState runState = dataModel()->create<RBX::RunService>()->getRunState();
		if (runState != RBX::RS_RUNNING) {
			state.text = "";
		}
		else {
			char text[32];
			std::snprintf(text, sizeof(text), "%.3gfps", dataModel()->timeState->fpsStopwatch.smoothFPS());
			state.text = text;
		}
	}
}

void CRobloxDoc::updateCollectGarbage(CommandState& state)
{
	RBX::DataModel::Lock lock(m_workspace->dataModel());
	RBX::ScriptContext* scriptContext = RBX::ServiceProvider::find<RBX::ScriptContext>(dataModel().get());
	state.enabled = scriptContext && scriptContext->hasState();
}

void CRobloxDoc::collectGarbage()
{
	RBX::DataModel::Lock lock(m_workspace->dataModel());
	RBX::ScriptContext* scriptContext = RBX::ServiceProvider::find<RBX::ScriptContext>(dataModel().get());
	if (scriptContext && scriptContext->allocator) {
		scriptContext->gc();

		unsigned int heapSize;
		unsigned int heapCount;
		scriptContext->allocator->getHeapStats(heapSize, heapCount);

		std::string heap = RBX::Log::formatMem(heapSize);
		unsigned int threadCount = scriptContext->getThreadCount();
		RBX::StandardOut::singleton()->print(
			RBX::MESSAGE_INFO, "Script heap %s (%d blocks), %d threads", heap.c_str(), heapCount, threadCount
		);
	}
}

void CRobloxDoc::updateThreadCount(CommandState& state)
{
	RBX::DataModel::Lock lock(m_workspace->dataModel());
	RBX::ScriptContext* scriptContext = RBX::ServiceProvider::find<RBX::ScriptContext>(dataModel().get());
	if (scriptContext && scriptContext->allocator) {
		char text[32];
		std::snprintf(text, sizeof(text), "%d threads", scriptContext->getThreadCount());
		state.text = text;
		state.enabled = true;
	}
	else {
		state.text = "";
		state.enabled = false;
	}
}

void CRobloxDoc::updateHeapSize(CommandState& state)
{
	RBX::DataModel::Lock lock(m_workspace->dataModel());
	RBX::ScriptContext* scriptContext = RBX::ServiceProvider::find<RBX::ScriptContext>(dataModel().get());
	if (scriptContext && scriptContext->allocator) {
		unsigned int heapSize;
		unsigned int heapCount;
		scriptContext->allocator->getHeapStats(heapSize, heapCount);

		std::string heap = RBX::Log::formatMem(heapSize);
		state.text = heap.c_str();
		state.enabled = true;
	}
	else {
		state.text = "";
		state.enabled = false;
	}
}

bool CRobloxDoc::executeScriptFile(std::string fileName)
{
	if (RBX::ContentProvider::isHttpUrl(fileName.c_str())) {
		fileName = RBX::ContentProvider::singleton().getFile(RBX::ContentId(fileName.c_str()));
	}

	QFile file(QString::fromStdString(fileName));
	if (!file.open(QIODevice::ReadOnly)) {
		throw std::runtime_error("Unable to open " + fileName);
	}
	QByteArray script = file.readAll();

	RBX::DataModel::Lock lock(dataModel());
	std::vector<RBX::Reflection::Value> arguments;
	try {
		dataModel()->create<RBX::ScriptContext>()->executeInNewThread(
			RBX::Security::LocalGUI, script.constData(), fileName.c_str(), arguments
		);
		return true;
	}
	catch (std::exception&) {
		return false;
	}
}

void CRobloxDoc::promptExecuteScript()
{
	CFileDialog dialog(
		true,
		QStringLiteral("lua"),
		QString(),
		CFileDialog::HideReadOnly | CFileDialog::OverwritePrompt,
		Resources::string(scriptFilterStringId)
	);
	if (dialog.DoModal() == IDOK) {
		executeScriptFile(dialog.GetPathName().toStdString());
	}
}

void CRobloxDoc::updateReportAbuse(CommandState& state)
{
	RBX::DataModel::Lock lock(m_workspace->dataModel());
	state.enabled = dataModel()->create<RBX::Network::Players>()->canReportAbuse();
}

bool CRobloxDoc::saveModified()
{
	if (!m_workspace || !m_workspace->dataModel()) {
		return true;
	}

	if (!isModified()) {
		return true;
	}

	RBX::DataModel::Lock lock(dataModel());
	bool wasRunning = RBX::ServiceProvider::create<RBX::RunService>(dataModel().get())->isRunState();
	RBX::ServiceProvider::create<RBX::RunService>(dataModel().get())->pause();
	RBX::Visit* visit = RBX::ServiceProvider::find<RBX::Visit>(dataModel().get());

	if (visit == nullptr || visit->getUploadUrl() == "NoVisit") {
		theApp.mainFrame()->activateWindow();
		return Document::saveModified();
	}

	if (visit->getUploadUrl().empty()) {
		return true;
	}

	QApplication::alert(theApp.mainFrame());

	CUploadPlaceDialog dialog(m_workspace, nullptr);
	bool result = dialog.DoModal() == IDOK;
	if (!result && wasRunning) {
		RBX::ServiceProvider::create<RBX::RunService>(m_workspace->dataModel().get())->run();
	}
	return result;
}

void CRobloxDoc::pause()
{
	RBX::DataModel::Lock lock(m_workspace->dataModel());
	RBX::ServiceProvider::create<RBX::RunService>(dataModel().get())->pause();
}

void CRobloxDoc::reset()
{
	RBX::DataModel::Lock lock(m_workspace->dataModel());
	if (dataModel()->create<RBX::RunService>()->getRunState() != RBX::RS_NORMAL) {
		RBX::ServiceProvider::create<RBX::RunService>(dataModel().get())->reset();

		// Clear the solo flag before restoring state.
		// Do NOT manually setParent(nullptr) on the local player here — clearContents()
		// below will destroy the entire service tree cleanly, including Players and its
		// local player. Calling setParent() first leaves Players with a dangling internal
		// localPlayer pointer and causes a crash on the next runsolo() call.
		m_isSoloPlay = false;

		dataModel()->clearContents();
		SerializerV2::load(m_resetState.get(), dataModel().get());
		dataModel()->getWorkspace()->joinAllHack();
		m_resetState.reset();
	}
}

void CRobloxDoc::run()
{
	RBX::DataModel::Lock lock(m_workspace->dataModel());
	if (dataModel()->create<RBX::RunService>()->getRunState() == RBX::RS_NORMAL) {
		m_resetState.reset(SerializerV2::newRootElement());
		dataModel()->writeChildren(m_resetState.get());
	}

	RBX::ServiceProvider::create<RBX::RunService>(dataModel().get())->run();
}

void CRobloxDoc::runsolo()
{
	RBX::DataModel::Lock lock(m_workspace->dataModel());

	// Save state for reset, same as run()
	if (dataModel()->create<RBX::RunService>()->getRunState() == RBX::RS_NORMAL) {
		m_resetState.reset(SerializerV2::newRootElement());
		dataModel()->writeChildren(m_resetState.get());
	}

	// Ensure the Visit service is present (equivalent to game:GetService("Visit"))
	RBX::ServiceProvider::create<RBX::Visit>(dataModel().get());

	// Create the local player (equivalent to Players:CreateLocalPlayer(0))
	RBX::Network::Players* players =
	    RBX::ServiceProvider::create<RBX::Network::Players>(dataModel().get());
	std::shared_ptr<RBX::Instance> playerInstance = players->createLocalPlayer(0);

	// Start the simulation (equivalent to RunService:run())
	RBX::ServiceProvider::create<RBX::RunService>(dataModel().get())->run();

	// Load the character after the simulation is running
	// (equivalent to player:LoadCharacter())
	if (RBX::Network::Player* player =
	        dynamic_cast<RBX::Network::Player*>(playerInstance.get())) {
		player->loadCharacter();
	}

	m_isSoloPlay = true;
}

void CRobloxDoc::insertLDraw()
{
	CFileDialog dialog(
		true,
		QStringLiteral("ldr"),
		QString(),
		CFileDialog::HideReadOnly | CFileDialog::OverwritePrompt,
		Resources::string(ldrawFilterStringId)
	);
	if (dialog.DoModal() != IDOK) {
		return;
	}

	RBX::LDraw2Lua::LDrawParser parser(dialog.GetPathName().toStdString().c_str());
	std::ostringstream stream;
	parser.writeLua(stream);
	stream.flush();

	try {
		RBX::DataModel::Lock lock(m_workspace->dataModel());
		std::vector<RBX::Reflection::Value> arguments;
		std::string script = stream.str();
		dataModel()->create<RBX::ScriptContext>()->executeInNewThread(
			RBX::Security::LocalGUI, script.c_str(), "LDraw", arguments
		);
	}
	catch (std::exception& e) {
		SystemMessageBox::critical(theApp.mainFrame(), Resources::string(appTitleStringId), e.what());
	}
}
