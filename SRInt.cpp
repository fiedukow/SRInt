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

Monitor::Monitor(zmq::socket_t& socket, std::queue<int>& events)
	: socket_(socket), events_(events) {
}

Monitor::~Monitor() {
}

void Monitor::operator()() {
	monitor(socket_, "inproc://monitor.req", ZMQ_EVENT_ALL);
}

void Monitor::on_event_disconnected(const zmq_event_t &event_, const char* addr_) {
	std::cout << "DISCONNECT" << std::endl;
	events_.push(ZMQ_EVENT_DISCONNECTED);
}

void Monitor::on_event_connect_delayed(const zmq_event_t &event_, const char* addr_) {
	std::cout << "UNABLE TO CONNECT" << std::endl;
	events_.push(ZMQ_EVENT_CONNECT_DELAYED);
}

void Monitor::on_event_connected(const zmq_event_t &event_, const char* addr_) {
	std::cout << "CONNECT" << std::endl;
	std::queue<int> empty;
	std::swap(events_, empty);
}

SRInt::SRInt(DB& db) 
	: db_(db), context_(1), client_(context_, ZMQ_PUSH), server_(context_, ZMQ_PULL), monitor_(client_, monitor_events_) {
	std::stringstream ss;
	ss << "tcp://" << cfg.ip_listen << ":" << cfg.port_listen;
	server_.bind(ss.str().c_str());
	server_.setsockopt(ZMQ_RCVTIMEO, &kReceiveTimeout, sizeof(kReceiveTimeout));
}

SRInt::~SRInt() {
}

void SRInt::operator()() {
	std::thread client_monitor_thread_(std::ref(monitor_));
	UserCommand command;
	if (!cfg.is_master)
		SendEntryRequest();

	while (true) {
		ReceiveStatus status = ReceiveMessage();
		switch (status) {
			case RECEIVING_ERROR:
				HandleMonitorEvents();
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
			default: assert(false);
		}
	}

	monitor_.abort();
	client_monitor_thread_.join();
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
			if (msg.state_content().state_id() < db_.state()->state_id())
				return NO_TOKEN_RECEIVED;
			db_.setState(msg.release_state_content());
			return TOKEN_RECEIVED;
		case Message_MessageType_ENTRY_REQUEST: {
			Message_NodeDescription* new_node = msg.mutable_state_content()->mutable_nodes()->ReleaseLast();
			commands_queue_.push(std::bind(&DB::addNode, &db_, new_node));
			std::cout << "Entry request!" << std::endl;
			return NO_TOKEN_RECEIVED;
		}
		default:
			assert(false);
			return RECEIVING_ERROR;
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
	if (db_.state()->nodes_size() == 1) {
		std::cout << "Going master." << std::endl;
		cfg.is_master = true;
		return;
	}

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

void SRInt::HandleMonitorEvents() {
	while (monitor_events_.size() > 0) {		
		switch (monitor_events_.front()) {
			case ZMQ_EVENT_DISCONNECTED:
			case ZMQ_EVENT_CONNECT_DELAYED:				
				commands_queue_.push(std::bind(&DB::removeFollower, &db_));
				break;
			default: assert(false);
		}
		monitor_events_.pop();
	}
}

bool SRInt::NetworkTokenShouldBeInitialized() {
	return (cfg.is_master && db_.state()->nodes_size() == 1);
}