#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif
#include "SRInt.h"
#include "DB.h"
#include "zmq.hpp"
#include "zhelpers.hpp"
#include "protobuf.pb.h"

#include <thread>

const int kReceiveTimeout = 100;
const int kTokenNotReceivedTimeoutMs = 5000;
const int kImmidiateSendOption = 1;

SRInt::SRInt(DB& db) 
	: db_(db), context_(1), client_(monitor_events_, context_), server_(context_, ZMQ_PULL) {
	std::stringstream ss;
	ss << "tcp://" << cfg.ip_listen << ":" << cfg.port_listen;
	server_.bind(ss.str().c_str());
	server_.setsockopt(ZMQ_RCVTIMEO, &kReceiveTimeout, sizeof(kReceiveTimeout));
	connected_ = cfg.is_master;
}

SRInt::~SRInt() {
}

void SRInt::operator()() {
	try {
		if (!cfg.is_master)
			SendEntryRequest();

		while (true)
			HandleReceivedMessageByStatus(ReceiveMessage());
	} catch (zmq::error_t& e) {
		std::cout << "Unhandled error_t: " << e.what() << std::endl;
	}
}

void SRInt::dhCreate(const std::string& name) {
	commands_queue_.push(std::bind(&DB::create, &db_, name));
}

void SRInt::dhFree(const std::string& name) {
	commands_queue_.push(std::bind(&DB::free, &db_, name));
}

int64 SRInt::dhGet(const std::string& name) {
	return db_.get(name);
}

void SRInt::dhSet(const std::string& name, int64 value) {
	commands_queue_.push(std::bind(&DB::set, &db_, name, value));
}

void SRInt::dhSetCallback(const std::string& name, NetworkCallback& callback) {
	commands_queue_.push(std::bind(&DB::setCallback, &db_, name, callback));
}

VariablesSnapshot SRInt::dhGetSnapshot() {
	return db_.state_snapshot();
}

void SRInt::addObserver(DBObserver* observer) {
	db_.addObserver(observer);
}

void SRInt::removeObserver(DBObserver* observer) {
	db_.removeObserver(observer);
}

void SRInt::SendState() {
	UpdateConnection();
	if (db_.state()->nodes_size() == 1)
		return;

	Message msg;
	msg.set_type(Message_MessageType_STATE);
	msg.set_allocated_state_content(db_.state());
	s_send(client_.socket(), msg.SerializeAsString());
	msg.release_state_content(); // we are not owners of this state
}

void SRInt::SendEntryRequest() {
	std::stringstream ss;
	ss << "tcp://" << cfg.in_network_ip << ":" << cfg.in_port;
	client_.connect(ss.str().c_str());	

	Message msg;
	msg.set_type(Message_MessageType_ENTRY_REQUEST);
	msg.set_allocated_state_content(db_.state());
	s_send(client_.socket(), msg.SerializeAsString());
	msg.release_state_content();
}

SRInt::ReceiveStatus SRInt::ReceiveMessage() {
	std::string received;
	try {
		received = s_recv(server_);
	} catch (std::exception e) {
		return RECEIVING_ERROR;
	}

	Message msg;
	msg.ParseFromString(received);
	switch (msg.type()) {
		case Message_MessageType_STATE:			
			connected_ = true;
			if (msg.state_content().state_id() < db_.state()->state_id()) // FIXME <=
				return NO_TOKEN_RECEIVED;
			db_.setState(msg.release_state_content());
			return TOKEN_RECEIVED;
		case Message_MessageType_ENTRY_REQUEST: {
			Message_NodeDescription* new_node = msg.mutable_state_content()->mutable_nodes()->ReleaseLast();
			commands_queue_.push(std::bind(&DB::addNode, &db_, new_node));
			return NO_TOKEN_RECEIVED;
		}
		default:
			assert(false);
			return RECEIVING_ERROR;
	}
	assert(false);
}

void SRInt::EngageSingleUserCommand() {
	while (!commands_queue_.empty()) {
		commands_queue_.front()();
		commands_queue_.pop();
	}
}

void SRInt::UpdateConnection() {	
	if (db_.state()->nodes_size() == 1) {
		cfg.is_master = true;
		client_.disconnect();
		return;
	}

	const Message_NodeDescription* next = db_.nextNode();

	std::stringstream ss;
	ss << "tcp://" << next->ip() << ":" << next->port();
	client_.connect(ss.str().c_str());
}

bool once = true;

bool SRInt::HandleMonitorEvents() {
	bool handled = false;
	while (monitor_events_.size() > 0) {		
		switch (monitor_events_.front()) {
			case ZMQ_EVENT_CONNECT_RETRIED:
				handled = true;
				break;
			case ZMQ_EVENT_DISCONNECTED:			
				commands_queue_.push(std::bind(&DB::removeFollower, &db_));
				handled = true;
				break;
			default: assert(false);
		}
		monitor_events_.pop();
	}
	return handled;
}

bool SRInt::NetworkTokenShouldBeInitialized() {
	return (cfg.is_master && db_.state()->nodes_size() == 1);
}

void SRInt::HandleReceivedMessageByStatus(ReceiveStatus status) {
	switch (status) {
	case RECEIVING_ERROR:
		if (HandleMonitorEvents()) {
			if (!connected_) {
				std::cout << "Nie udalo podlaczyc sie do wezla matki." << std::endl;
				Sleep(1000);
				exit(1); //TODO
				return;
			}
			EngageSingleUserCommand();
			SendState();
		}
		break;
	case NO_TOKEN_RECEIVED:
		if (!NetworkTokenShouldBeInitialized()) {
			HandleMonitorEvents();
			break;
		}
		//fallthrough
	case TOKEN_RECEIVED:
		EngageSingleUserCommand();
		SendState();
		break;
	default: assert(false);
	}
}
