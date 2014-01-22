#pragma once
#include <queue>
#include <functional>
#include "DBObserver.h"
#include "BlockingQueue.hpp"
#include "protobuf.pb.h"
#include "DB.h"
#include "zmq.hpp"
#include "Config.h"
#include <map>
#include <thread>

typedef std::function<void ()> UserCommand;

class Monitor : public zmq::monitor_t {
public:
	Monitor(zmq::socket_t& socket, std::queue<int>& events);
	virtual ~Monitor();
	void operator()();

	virtual void on_event_disconnected(const zmq_event_t &event_, const char* addr_);
	virtual void on_event_connect_retried(const zmq_event_t &event_, const char* addr_);
	virtual void on_event_connected(const zmq_event_t &event_, const char* addr_);

private:
	zmq::socket_t& socket_;
	std::queue<int>& events_;

	int retries_;
	std::string last_connected_addr_; // zmq ftw!!!!!

	static int next_monitor_index;
};

class Client {	
public:
	Client(std::queue<int>& monitor_events, zmq::context_t& context);
	~Client();

	void connect(const std::string& address);
	void disconnect();

	zmq::socket_t& socket();
	bool hasSocket();

private:
	std::string address_;
	std::queue<int>& monitor_events_;
	zmq::context_t& context_;
	bool connected_;

	std::thread* monitor_thread_;
	zmq::socket_t* socket_;
	Monitor* monitor_;
};

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
	enum ReceiveStatus { TOKEN_RECEIVED, NO_TOKEN_RECEIVED, RECEIVING_ERROR };

	void SendState();
	void SendEntryRequest();
	ReceiveStatus ReceiveMessage();	
	void EngageSingleUserCommand();
	void UpdateConnection();
	bool HandleMonitorEvents(); // true if handled something
	bool NetworkTokenShouldBeInitialized();

	DB& db_;
	std::queue<UserCommand> commands_queue_; // FIXME thread save (non-blocking) queue 

	zmq::context_t context_;	
	zmq::socket_t server_;
	Client client_;

	// TODO Move this to another class
	std::queue<int> monitor_events_; // FIXME thread save (non-blocking) queue 	
	bool connected_;

	Config cfg;
};

