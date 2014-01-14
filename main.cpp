#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif
#include "zmq.hpp"
#include <string>
#include <iostream>
#include <thread>
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
	base_state->set_state_id(15);

	Message_NodeDescription* owner_node = new Message_NodeDescription;	
	owner_node->set_ip("127.0.0.1");
	owner_node->set_port(5555);
	owner_node->set_node_id(31);	

	base_state->mutable_nodes()->Add()->CopyFrom(*owner_node);

	DB db(base_state, owner_node);
	db.create("dupa");
	db.set("dupa", 72);

	SRInt model(db);
	UI ui(model);
	std::thread model_thread(std::ref(model));
	ui();
    return 0;
}

