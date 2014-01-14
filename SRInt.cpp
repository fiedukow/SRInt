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
	server_.bind("tcp://*:5555");
}

SRInt::~SRInt() {
}

void SRInt::operator()() {
	UserCommand command;
	client_.connect("tcp://169.254.190.225:5555");
	while (true) {
		SendState();
		ReceiveState();
		EngageSingleUserCommand();		
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
	Message msg;
	msg.set_type(Message_MessageType_STATE);
	msg.set_allocated_state_content(db_.state());
	s_send(client_, msg.SerializeAsString());
	msg.release_state_content(); // we are not owners of this state
}

void SRInt::ReceiveState() {
	std::string received = s_recv(server_);
	Message msg;
	msg.ParseFromString(received);
	db_.setState(msg.release_state_content());	
}

void SRInt::EngageSingleUserCommand() {
	if (commands_queue_.empty())
		return;
	commands_queue_.front()();
	commands_queue_.pop();
}