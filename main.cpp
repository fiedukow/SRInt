#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif
#include "zmq.hpp"
#include <string>
#include <iostream>
#include "zhelpers.hpp"
#include "protobuf.pb.h"
#ifndef _WIN32
#include <unistd.h>
#else
#include <windows.h>
#endif

#include "DB.h"
#include "UI.h"
#include "SRInt.h"

int main () {
	Message_State* base_state = new Message_State;	
	base_state->set_state_id(0);

	Message_NodeDescription* owner_node = new Message_NodeDescription;
	owner_node->set_ip("169.254.26.129");
	owner_node->set_port(5555);
	owner_node->set_node_id(-1);	

	DB db(base_state, owner_node);
	db.create("x");
	db.set("x", 17);
	db.create("dupa.");
	db.set("dupa.", 8);
	db.create("maciek");
	db.set("maciek", -9);
	Message_State* state = db.state();
	
	Message* msg = new Message;
	msg->set_type(Message_MessageType_STATE);
	msg->set_allocated_state_content(base_state);

	//  Prepare our context and socket
    zmq::context_t context (1);
    /*zmq::socket_t socket (context, ZMQ_PULL);
    socket.bind ("tcp://*:5555");*/
	zmq::socket_t * client = new zmq::socket_t (context, ZMQ_PUSH);
    client->connect ("tcp://169.254.139.252:5555");

    std::cout << "Server is up and running!" << std::endl;
    if (true) {
		//std::string received = s_recv(socket);
  //      std::cout << "Received " << received << std::endl;   
		//Message m;
		//m.ParseFromString(received);
		//std::cout << "Id " << m.state_content().state_id()  << std::endl;
		//for(auto i = m.state_content().variables().begin();
		//	i != m.state_content().variables().end();
		//	++i)
		//	std::cout << i->name() << " = " << i->value() << std::endl;
  //      std::stringstream ss;
  //      ss << "World " << answer_number++;
        //  Send reply back to client
		std::cout << "Sending: \"" << state->SerializeAsString() << "\""<< std::endl;
        s_send(*client, state->SerializeAsString());
    }

	SRInt model(db);
	UI ui(model);
	ui();
    return 0;
}

