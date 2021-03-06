#pragma once
#include <functional>
#include <map>

#include "DBObserver.h"
#include "BlockingQueue.hpp"
#include "protobuf.pb.h"
#include "DB.h"
#include "zmq.hpp"
#include "Config.h"
#include "Client.h"
#include "SafeQueue.hpp"

typedef std::function<void ()> UserCommand;

class SRInt {
public:
	SRInt(DB& db);
	~SRInt();

	void operator()();

	// REQUESTED API
	void dhCreate(const std::string& name);
	void dhFree(const std::string& name);
	int64 dhGet(const std::string& name);
	void dhSet(const std::string& name, int64 value);
	void dhSetCallback(const std::string& name, NetworkCallback& callback);
	VariablesSnapshot dhGetSnapshot();

	void addObserver(DBObserver* observer);
	void removeObserver(DBObserver* observer);

private:
	enum ReceiveStatus { TOKEN_RECEIVED, ACK_RECEIVED, UPDATE_RECEIVED, NO_TOKEN_RECEIVED, RECEIVING_ERROR };

	void SendState();
	void SendEntryRequest();
	ReceiveStatus ReceiveMessage();	
	void EngageSingleUserCommand();
	void UpdateConnection();
	bool HandleMonitorEvents(); // true if handled something
	bool NetworkTokenShouldBeInitialized();
	void HandleReceivedMessageByStatus(ReceiveStatus status);

	DB& db_;
	SafeQueue<UserCommand> commands_queue_;

	zmq::context_t context_;	
	zmq::socket_t server_;
	Client client_;

	// TODO Move this to another class
	SafeQueue<int> monitor_events_;
	bool was_in_network_;
	int64 last_self_change_id_;
	int64 entry_request_send_time_;

	Config cfg;
};
