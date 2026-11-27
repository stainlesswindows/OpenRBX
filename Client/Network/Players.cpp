#include "network/Players.h"

#include "PacketIds.h"
#include "network/Client.h"
#include "network/Server.h"
#include "network/Streaming.h"
#include "reflection/property.h"
#include "util/Http.h"
#include "util/standardout.h"
#include "v8datamodel/GameSettings.h"
#include "v8datamodel/ModelInstance.h"
#include "v8datamodel/Teams.h"
#include "v8xml/XmlElement.h"
#include "v8xml/XmlSerializer.h"

#include <PluginInterface.h>
#include <RakPeerInterface.h>
#include <algorithm>
#include <sstream>
#include <stdexcept>

template <class T>
class PluginInterfaceAdapter : public PluginInterface
{
	T* c;

protected:
	PluginInterfaceAdapter(T* c) : c(c) {}

public:
	PluginReceiveResult OnReceive(RakPeerInterface* peer, Packet* packet) override;
};

namespace RBX::Network {

char sPlayers[] = "Players";

static Reflection::PropDescriptor<Players, int> propPlayerCount(
	"NumPlayers", "Data", &Players::numPlayers, 0, Reflection::PropertyDescriptor::UI
);

static Reflection::PropDescriptor<Players, int> propPlayerMaxCount(
	"MaxPlayers", "Data", &Players::getMaxPlayers, &Players::setMaxPlayers
);

class Players::Plugin : public PluginInterfaceAdapter<Players>
{
public:
	Plugin(Players* players) : PluginInterfaceAdapter<Players>(players) {}
};

Reflection::RefPropDescriptor<Players, Player> Players::propLocalPlayer(
	"LocalPlayer", "Data", &Players::getLocalPlayer, 0
);

static Reflection::BoundFuncDesc<Players, std::shared_ptr<Instance>(int), 1> func_GetPlayerByID(
	&Players::getPlayerByID, "GetPlayerByID", "userID", Reflection::FunctionDescriptor::NeedTrustedCaller
);

Reflection::PropDescriptor<Players, bool> Players::propCharacterAutoSpawn(
	"CharacterAutoLoads", "Behavior", &Players::getCharacterAutoSpawnProperty, &Players::setCharacterAutoSpawnProperty, Reflection::PropertyDescriptor::STANDARD
);

static Reflection::BoundFuncDesc<Players, void(std::string), 1> funcChat(
	&Players::chat, "Chat", "message", Reflection::FunctionDescriptor::NeedTrustedCaller
);

static Reflection::BoundFuncDesc<Players, void(std::shared_ptr<Instance>, std::string), 2> funcReportAbuse(
	&Players::reportAbuse, "ReportAbuse", "player", "message", Reflection::FunctionDescriptor::NeedTrustedCaller
);

static Reflection::BoundFuncDesc<Players, std::shared_ptr<const std::vector<std::shared_ptr<Instance>>>(), 0>
	func_players(&Players::getPlayers, "players", Reflection::FunctionDescriptor::AnyCaller);
static Reflection::BoundFuncDesc<Players, std::shared_ptr<const std::vector<std::shared_ptr<Instance>>>(), 0>
	func_GetPlayers(&Players::getPlayers, "GetPlayers", Reflection::FunctionDescriptor::AnyCaller);
static Reflection::BoundFuncDesc<Players, std::shared_ptr<Instance>(int), 1> func_createLocalPlayer(
	&Players::createLocalPlayer, "CreateLocalPlayer", "userId", Reflection::FunctionDescriptor::NeedTrustedCaller
);

static Reflection::BoundFuncDesc<Players, void(std::string), 1> func_setAbuseReportUrl(
	&Players::setAbuseReportUrl, "SetAbuseReportUrl", "url", Reflection::FunctionDescriptor::NeedTrustedCaller
);

static Reflection::BoundFuncDesc<Players, std::shared_ptr<Instance>(std::shared_ptr<Instance>), 1>
	func_playerFromCharacterOld(
		&Players::playerFromCharacter, "playerFromCharacter", "character", Reflection::FunctionDescriptor::AnyCaller
	);
static Reflection::BoundFuncDesc<Players, std::shared_ptr<Instance>(std::shared_ptr<Instance>), 1>
	func_playerFromCharacter(
		&Players::playerFromCharacter, "GetPlayerFromCharacter", "character", Reflection::FunctionDescriptor::AnyCaller
	);
static Reflection::SignalDesc<Players, void(std::shared_ptr<Instance>)> event_PlayerAdded("PlayerAdded", "player");
static Reflection::SignalDesc<Players, void(std::shared_ptr<Instance>)> event_PlayerRemoving(
	"PlayerRemoving", "player"
);

bool Players::clientIsPresent(const Instance* context, bool testInDatamodel)
{
	return Client::clientIsPresent(context, testInDatamodel);
}

bool Players::serverIsPresent(const Instance* context, bool testInDatamodel)
{
	return Server::serverIsPresent(context, testInDatamodel);
}

bool Players::frontendProcessing(const Instance* context, bool testInDatamodel)
{
	const ServiceProvider* serviceProvider = ServiceProvider::findServiceProvider(context);

	return serviceProvider != nullptr && !Server::serverIsPresent(context, testInDatamodel);
}

bool Players::backendProcessing(const Instance* context, bool testInDatamodel)
{
	const ServiceProvider* serviceProvider = ServiceProvider::findServiceProvider(context);

	return serviceProvider != nullptr && !Client::clientIsPresent(context, testInDatamodel);
}

bool Players::askAddChild(const Instance* instance) const
{
	return dynamic_cast<const Player*>(instance) != nullptr;
}

bool Players::canReportAbuse() const
{
	return abuseReporter || clientIsPresent(this, true);
}

void Players::setConnection(RakPeerInterface* peer)
{
	if (this->peer) {
		this->peer->DetachPlugin(plugin.get());
	}

	this->peer = peer;

	if (peer) {
		peer->AttachPlugin(plugin.get());
	}
}

void Players::onEvent(const Player* source, CharacterAdded event)
{
	const_cast<Player*>(source)->rebuildBackpack();
}

bool Players::superSafeOn() const
{
	return localPlayer ? Player::prop_SuperSafeChat.getValue(localPlayer.get()) : false;
}

} // namespace RBX::Network

static void writeMessage(const RBX::Network::AbuseReport::Message& msg, XmlElement* messages)
{
	XmlElement* message = messages->addChild(RBX::Name::declare("message", -1));
	message->setValue(msg.text);
	message->addAttribute(RBX::Name::declare("userID", -1), msg.userID);
}

namespace RBX::Network {

std::shared_ptr<Instance> Players::getPlayerByID(int userID)
{
	std::vector<std::shared_ptr<Instance>>::const_iterator iter = players->begin();
	std::vector<std::shared_ptr<Instance>>::const_iterator end = players->end();

	for (; iter != end; ++iter) {
		if (static_cast<Player*>(iter->get())->getUserID() == userID) {
			return *iter;
		}
	}

	return std::shared_ptr<Instance>();
}

std::shared_ptr<Instance> Players::playerFromCharacter(std::shared_ptr<Instance> character)
{
	std::shared_ptr<const std::vector<std::shared_ptr<Instance>>> myPlayers = getPlayers();

	std::vector<std::shared_ptr<Instance>>::const_iterator iter = myPlayers->begin();
	std::vector<std::shared_ptr<Instance>>::const_iterator end = myPlayers->end();

	for (; iter != end; ++iter) {
		if (static_cast<Player*>(iter->get())->getCharacter() == character.get()) {
			return *iter;
		}
	}

	return std::shared_ptr<Instance>();
}

void Players::onChildChanged(Instance* instance, const PropertyChanged& event)
{
	if (instance == localPlayer.get() && &event.getProperty().getDescriptor() == &Player::prop_SuperSafeChat) {
		Notifier<Players, SuperSafeChanged>::raise(SuperSafeChanged(superSafeOn()));
	}
}

void AbuseReport::addMessage(const ChatMessage& cm)
{
	int userID = cm.source ? Player::prop_userId.getValue(cm.source.get()) : 0;
	Message m = {userID, cm.message};
	messages.push_back(m);
}

Players::~Players()
{
	setConnection(nullptr);
}

Player* Players::findLocalPlayer(const Instance* context)
{
	const Players* players = ServiceProvider::find<Players>(context);

	return players ? players->getLocalPlayer() : nullptr;
}

RBX::ModelInstance* Players::findLocalCharacter(const Instance* context)
{
	if (Player* player = findLocalPlayer(context)) {
		return player->getCharacter();
	}

	return nullptr;
}

Player* Players::getPlayerFromCharacter(Instance* character)
{
	if (Players* players = ServiceProvider::find<Players>(character)) {
		std::shared_ptr<Instance> player = players->playerFromCharacter(shared_from(character));
		return static_cast<Player*>(player.get());
	}

	return nullptr;
}

Players::Players() : players(std::vector<std::shared_ptr<Instance>>()), peer(nullptr), maxPlayers(12), characterAutoSpawn(true)
{
	setName("Players");
	plugin.reset(new Plugin(this));
}

void AbuseReporter::add(AbuseReport& r, const std::list<ChatMessage>& chatHistory)
{
	std::for_each(chatHistory.begin(), chatHistory.end(), [&r](const ChatMessage& cm) { r.addMessage(cm); });

	{
		std::scoped_lock lock(_data->requestSync);
		_data->queue.push(r);
	}

	requestProcessor->wake();
}

worker_thread::work_result AbuseReporter::processRequests(std::shared_ptr<data> _data, std::string abuseUrl)
{
	try {
		AbuseReport r;

		{
			std::scoped_lock lock(_data->requestSync);
			if (_data->queue.empty()) {
				return worker_thread::done;
			}
			r = _data->queue.front();
		}

		std::stringstream body;

		{
			TextXmlWriter writer(body);
			XmlElement root(Name::declare("report", -1));
			root.addAttribute(Name::declare("userID", -1), r.submitterID);

			root.addChild(new XmlElement(Name::declare("comment", -1), r.comment));
			XmlElement* messages = root.addChild(new XmlElement(Name::declare("messages", -1)));
			std::for_each(r.messages.begin(), r.messages.end(), [messages](const AbuseReport::Message& msg) {
				writeMessage(msg, messages);
			});
			writer.serialize(&root);

			body.flush();
		}

		std::string response;

		Http(abuseUrl).post(body, false, response);
		StandardOut::singleton()->print(MESSAGE_INFO, "Posted abuse report to %s", abuseUrl.c_str());
	}
	catch (std::exception& e) {
		StandardOut::singleton()->print(
			MESSAGE_WARNING, "Unable to post abuse report to %s. %s", abuseUrl.c_str(), e.what()
		);
	}

	{
		std::scoped_lock lock(_data->requestSync);
		_data->queue.pop();
	}

	return worker_thread::more;
}

void Players::reportAbuse(Player* player, std::string comment)
{
	if (abuseReporter) {
		StandardOut::singleton()->print(
			MESSAGE_INFO, "Submitting abuse report from %s", player ? player->getName().c_str() : "[[[anonymous]]]"
		);

		AbuseReport r;
		r.submitterID = localPlayer ? localPlayer->getUserID() : 0;
		r.allegedAbuserID = player ? player->getUserID() : 0;
		r.comment = comment;
		abuseReporter->add(r, chatHistory);
	}
	else {
		StandardOut::singleton()->print(MESSAGE_INFO, "Sending abuse report to game server via Raknet");

		if (!peer) {
			throw std::runtime_error("Can't report abuse: Not in a networked game");
		}

		RakNet::BitStream bitStream;
		bitStream << (unsigned char) ID_REPORT_ABUSE;
		bitStream << (localPlayer ? localPlayer->getUserID() : 0);
		bitStream << (player ? player->getUserID() : 0);
		bitStream << comment;

		peer->Send(&bitStream, HIGH_PRIORITY, RELIABLE, 2, UNASSIGNED_SYSTEM_ADDRESS, true);
	}
}

void Players::reportAbuse(std::shared_ptr<Instance> player, std::string comment)
{
	reportAbuse(dynamic_cast<Player*>(player.get()), comment);
}

void Players::chat(std::string message)
{
	if (!localPlayer) {
		throw std::runtime_error("No local Player to chat from");
	}

	RakNet::BitStream bitStream;
	bitStream << (unsigned char) ID_CHAT_ALL;

	Guid::Data id;
	localPlayer->getGuid().extract(id);

	bitStream << id.scope->toString();
	bitStream << id.index;
	bitStream << message;

	peer->Send(&bitStream, MEDIUM_PRIORITY, RELIABLE, 2, UNASSIGNED_SYSTEM_ADDRESS, true);

	ChatMessage event = {message, localPlayer};
	Notifier<Players, ChatMessage>::raise(event);
}

void Players::setMaxPlayers(int value)
{
	if (value != maxPlayers) {
		maxPlayers = value;
		raisePropertyChanged(propPlayerMaxCount);
	}
}

std::shared_ptr<Instance> Players::createLocalPlayer(int userId)
{
	if (localPlayer) {
		throw std::runtime_error("Local player already exists");
	}

	localPlayer = Creatable<Instance>::create<Player>();
	Player::prop_userId.setValue(localPlayer.get(), userId);
	localPlayer->setParent(this);
	raisePropertyChanged(propLocalPlayer);

	Notifier<Players, SuperSafeChanged>::raise(SuperSafeChanged(superSafeOn()));

	return localPlayer;
}

void Players::addChatMessage(const ChatMessage& message)
{
	chatHistory.push_front(message);

	while (chatHistory.size() > GameSettings::singleton().chatHistory) {
		chatHistory.pop_back();
	}

	Notifier<Players, ChatMessage>::raise(message);
	Player::event_Chatted.fire(message.source.get(), message.message, message.destination);
}

void Players::setCharacterAutoSpawnProperty(bool value)
{
	if(value != characterAutoSpawn)
	{
		characterAutoSpawn = value;
		raisePropertyChanged(propCharacterAutoSpawn);
	}
}

bool Players::getShouldAutoSpawnCharacter() const
{
	return characterAutoSpawn;
}

bool Players::OnReceive(RakPeerInterface* peer, Packet* packet)
{
	switch (packet->data[0]) {
	case ID_CHAT_ALL:
		try {
			RakNet::BitStream inBitstream(packet->data, packet->length, false);
			inBitstream.IgnoreBits(8);

			std::string foo;
			inBitstream >> foo;

			Guid::Data id;
			id.scope = &Name::lookup(foo);
			inBitstream >> id.index;

			Instance* player = nullptr;
			Instance::lookupByGuid(id, player);

			std::string message;
			inBitstream >> message;

			if (!player) {
				StandardOut::singleton()->print(
					MESSAGE_WARNING,
					"Chat message from unknown Player %s %s",
					packet->systemAddress.ToString(true),
					message.c_str()
				);
			}
			else if (player->getParent() == this) {
				ChatMessage event = {message, shared_from_polymorphic_downcast<Player>(player)};
				addChatMessage(event);

				peer->Send(
					reinterpret_cast<const char*>(packet->data),
					packet->length,
					MEDIUM_PRIORITY,
					RELIABLE_ORDERED,
					2,
					packet->systemAddress,
					true
				);
			}
		}
		catch (std::exception& e) {
			StandardOut::singleton()->print(
				MESSAGE_ERROR, "Players::OnReceive packet %d: %s", packet->data[0], e.what()
			);
		}

		return false;

	case ID_REPORT_ABUSE:
		try {
			if (abuseReporter) {
				RakNet::BitStream inBitstream(packet->data, packet->length, false);
				inBitstream.IgnoreBits(8);

				AbuseReport r;

				inBitstream >> r.submitterID;
				inBitstream >> r.allegedAbuserID;
				inBitstream >> r.comment;

				abuseReporter->add(r, chatHistory);
			}
		}
		catch (std::exception& e) {
			StandardOut::singleton()->print(
				MESSAGE_ERROR, "Players::OnReceive packet %d: %s", packet->data[0], e.what()
			);
		}

		return false;

	default:
		return true;
	}
}

void Players::onChildRemoving(Instance* child)
{
	if (Player* player = dynamic_cast<Player*>(child)) {
		std::shared_ptr<std::vector<std::shared_ptr<Instance>>>& list = players.write();
		list->erase(std::find(list->begin(), list->end(), shared_from(child)));

		if (backendProcessing(this, false)) {
			event_PlayerRemoving.fire(this, shared_from(player));
		}

		stopListening(player, this);
	}
}

void Players::onChildAdded(Instance* child)
{
	if (Player* player = dynamic_cast<Player*>(child)) {
		if (player->getUserID() > 0) {
			std::shared_ptr<Instance> otherPlayer = getPlayerByID(player->getUserID());

			if (otherPlayer) {
				otherPlayer->setParent(nullptr);
			}
		}

		std::shared_ptr<std::vector<std::shared_ptr<Instance>>>& list = players.write();
		list->push_back(shared_from(child));

		if (backendProcessing(this, true)) {
			startListening(player, this);
			player->rebuildBackpack();

			if (Teams* teams = ServiceProvider::find<Teams>(this)) {
				teams->assignNewPlayerToTeam(player);
			}

			event_PlayerAdded.fire(this, shared_from(player));
		}
	}
}

} // namespace RBX::Network

template <class T>
PluginReceiveResult PluginInterfaceAdapter<T>::OnReceive(RakPeerInterface* peer, Packet* packet)
{
	return c->OnReceive(peer, packet) ? RR_CONTINUE_PROCESSING : RR_STOP_PROCESSING_AND_DEALLOCATE;
}

namespace RBX::Network {

AbuseReporter::AbuseReporter(std::string abuseUrl) : _data(new data())
{
	requestProcessor.reset(
		new worker_thread([_data = _data, abuseUrl] { return processRequests(_data, abuseUrl); }, "rbx_abusereporter")
	);
}

void Players::setAbuseReportUrl(std::string value)
{
	if (value.empty()) {
		abuseReporter.reset();
	}
	else {
		abuseReporter.reset(new AbuseReporter(value));
	}
}

} // namespace RBX::Network
