#pragma once
#include <string>

struct Config {	
	std::string ip_listen;
	int port_listen;
	std::string in_network_ip;
	int in_port;
	bool is_master;

	Config();
};
