#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif
#include "SRInt.h"
#include "DB.h"
#include "zmq.hpp"
#include "zhelpers.hpp"
#include "protobuf.pb.h"

SRInt::SRInt(DB& db) 
	: db_(db), context_(1), client_(context_, ZMQ_PUSH), server_(context_, ZMQ_PULL) {
	std::stringstream ss;
	ss << "tcp://" << cfg.ip_listen << ":" << cfg.port_listen;
	server_.bind(ss.str().c_str());
}

SRInt::~SRInt() {
}

void SRInt::operator()() {
	UserCommand command;
	if (!cfg.is_master)
		SendEntryRequest();

	while (true) {
		if (!ReceiveMessage() && !NetworkTokenShouldBeInitialized())
			continue;
		EngageSingleUserCommand();
		SendState();
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

	Message msg;
	msg.set_type(Message_MessageType_STATE);
	msg.set_allocated_state_content(db_.state());
	s_send(client_, msg.SerializeAsString());
	msg.release_state_content(); // we are not owners of this state
}

void SRInt::SendEntryRequest() {
	std::stringstream ss;
	ss << "tcp://" << cfg.in_network_ip << ":" << cfg.in_port;
	client_.connect(ss.str().c_str());

	Message msg;
	msg.set_type(Message_MessageType_ENTRY_REQUEST);
	msg.set_allocated_state_content(db_.state());
	s_send(client_, msg.SerializeAsString());
	msg.release_state_content();
}

bool SRInt::ReceiveMessage() {
	std::string received = s_recv(server_);
	Message msg;
	msg.ParseFromString(received);
	switch (msg.type()) {
		case Message_MessageType_STATE:			
			db_.setState(msg.release_state_content());
			return true;
		case Message_MessageType_ENTRY_REQUEST: {
			Message_NodeDescription* new_node = msg.mutable_state_content()->mutable_nodes()->ReleaseLast();
			commands_queue_.push(std::bind(&DB::addNode, &db_, new_node));
			return false;
		}
		default:
			assert(false);
			return false;
	}
	assert(false);
}

void SRInt::EngageSingleUserCommand() {
	if (commands_queue_.empty())
		return;
	commands_queue_.front()();
	commands_queue_.pop();
}

void SRInt::UpdateConnection() {	
	const Message_NodeDescription* next = db_.nextNode();
	if (last_connected_ip == next->ip() && last_connected_port == next->port())
		return;

	if (last_connected_ip != "") {
		std::stringstream ss;
		ss << "tcp://" << last_connected_ip << ":" << last_connected_port;
		client_.disconnect(ss.str().c_str());
	}

	std::stringstream ss;
	ss << "tcp://" << next->ip() << ":" << next->port();
	client_.connect(ss.str().c_str());
	last_connected_ip = next->ip();
	last_connected_port = next->port();
}

bool SRInt::NetworkTokenShouldBeInitialized() {
	return (cfg.is_master && db_.state()->nodes_size() == 1);
}