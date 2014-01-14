#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif
#include "SRInt.h"
#include "DB.h"
#include "zmq.hpp"
#include "zhelpers.hpp"
#include "protobuf.pb.h"

SRInt::SRInt(DB& db) 
	: db_(db) {
}

SRInt::~SRInt() {
}

void SRInt::operator()() {
	UserCommand command;
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

void SRInt::SendState() {
	zmq::context_t context(1);
	zmq::socket_t client = zmq::socket_t(context, ZMQ_PUSH);
	client.connect("tcp://169.254.190.225:5555");

	Message msg;
	msg.set_type(Message_MessageType_STATE);
	msg.set_allocated_state_content(db_.state());
	s_send(client, msg.SerializeAsString());
	msg.release_state_content(); // we are not owners of this state
}

void SRInt::ReceiveState() {
	zmq::context_t context(1);
	zmq::socket_t socket(context, ZMQ_PULL);
	socket.bind("tcp://*:5555");

	std::string received = s_recv(socket);
	Message msg;
	msg.ParseFromString(received);
	db_.setState(msg.release_state_content());	

	//std::cout << "Id " << last_message->state_content().state_id() << std::endl;
	//for (auto i = last_message->state_content().variables().begin();
	//	i != last_message->state_content().variables().end();
	//	++i)
	//	std::cout << i->name() << " = " << i->value() << std::endl;

}

void SRInt::EngageSingleUserCommand() {
	if (commands_queue_.empty())
		return;
	commands_queue_.front()();
	commands_queue_.pop();
}