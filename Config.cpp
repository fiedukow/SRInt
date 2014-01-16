#include "Config.h"
#include <fstream>

Config::Config()
{
	std::fstream in;
	in.open("config.dat", std::fstream::in);
	if (!in.good())
		throw std::runtime_error("Config file cannot be opened.");
	in >> ip_listen >> port_listen >> in_network_ip >> in_port >> is_master;
	in.close();
}