#ifndef NETWORK_PLAYERS_H
#define NETWORK_PLAYERS_H

#include "network/Player.h"
#include "network/SuperSafeChanged.h"
#include "reflection/reflection.h"
#include "util/Events.h"
#include "util/Thread.h"
#include "v8tree/Instance.h"
#include "v8tree/Service.h"

#include <list>
#include <memory>
#include <mutex>
#include <queue>
#include <string>

class RakPeerInterface;
struct Packet;

namespace RBX {

class ModelInstance;

namespace Network {

extern char sPlayers[];

struct ChatMessage
{
	const std::string message;
	const std::shared_ptr<Player> source;
	const std::shared_ptr<Player> destination;
};

struct AbuseReport
{
	struct Message
	{
		int userID;
		std::string text;
	};

	int submitterID;
	int allegedAbuserID;
	std::string comment;
	std::list<Message> messages;

	void addMessage(const ChatMessage& cm);
};

class AbuseReporter
{
	struct data
	{
		std::queue<AbuseReport> queue;
		std::mutex requestSync;
	};

	std::shared_ptr<data> _data;
	std::unique_ptr<worker_thread> requestProcessor;

public:
	AbuseReporter(std::string abuseUrl);

	void add(AbuseReport& r, const std::list<ChatMessage>& chatHistory);

private:
	static worker_thread::work_result processRequests(std::shared_ptr<data> _data, std::string abuseUrl);
};

class Players : public DescribedNonCreatable<Players, Instance, sPlayers>,
				public Service,
				public Notifier<Players, ChatMessage>,
				public Notifier<Players, SuperSafeChanged>,
				public Listener<Player, CharacterAdded>
{
	bool characterAutoSpawn;
	std::unique_ptr<AbuseReporter> abuseReporter;
	std::list<ChatMessage> chatHistory;

	class Plugin;

	std::unique_ptr<Plugin> plugin;
	CopyOnWrite<std::vector<std::shared_ptr<Instance>>> players;
	std::shared_ptr<Player> localPlayer;
	RakPeerInterface* peer;
	int maxPlayers;

	void onEvent(const Player* source, CharacterAdded event) override;
	void onChildChanged(Instance* instance, const PropertyChanged& event) override;

public:
	static Reflection::RefPropDescriptor<Players, Player> propLocalPlayer;
	static Reflection::PropDescriptor<Players, bool> propCharacterAutoSpawn;

	Players();
	~Players() override;

	bool superSafeOn() const;
	std::shared_ptr<Instance> createLocalPlayer(int userId);

	Player* getLocalPlayer() const { return localPlayer.get(); }
	int getNumPlayers() const;

	int numPlayers() const { return players->size(); }

	int getMaxPlayers() const { return maxPlayers; }

	void setMaxPlayers(int value);

	std::shared_ptr<const std::vector<std::shared_ptr<Instance>>> getPlayers() { return players.read(); }

	void chat(std::string message);
	void reportAbuse(Player* player, std::string comment);
	std::list<ChatMessage>::const_iterator chatHistory_begin();
	std::list<ChatMessage>::const_iterator chatHistory_end();
	bool canReportAbuse() const;
	void reportAbuse(std::shared_ptr<Instance> player, std::string comment);

	void setAbuseReportUrl(std::string value);
	bool getCharacterAutoSpawnProperty() const { return characterAutoSpawn; }
	void setCharacterAutoSpawnProperty(bool value);
	bool getShouldAutoSpawnCharacter() const;	

	bool OnReceive(RakPeerInterface* peer, Packet* packet);
	void setConnection(RakPeerInterface* peer);

	std::shared_ptr<Instance> playerFromCharacter(std::shared_ptr<Instance> character);
	std::shared_ptr<Instance> getPlayerByID(int userID);

	static Player* getPlayerFromCharacter(Instance* character);

	static RBX::ModelInstance* findLocalCharacter(const Instance* context);
	static Player* findLocalPlayer(const Instance* context);

	static bool clientIsPresent(const Instance* context, bool testInDatamodel);
	static bool serverIsPresent(const Instance*, bool);
	static bool frontendProcessing(const Instance* context, bool testInDatamodel);
	static bool backendProcessing(const Instance* context, bool testInDatamodel);

protected:
	bool askAddChild(const Instance* instance) const override;
	void onChildAdded(Instance* child) override;
	void onChildRemoving(Instance* child) override;

private:
	void addChatMessage(const ChatMessage& message);
};

} // namespace Network
} // namespace RBX

#endif // NETWORK_PLAYERS_H
