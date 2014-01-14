#include "UI.h"
#include <iostream>
#include <sstream>

#include "SRInt.h"

UI::UI(SRInt& model) 
	: model_(model) {
}

UI::~UI() {
}

void UI::operator()() {
	std::cout << "Witaj w super SRInt'cie" << std::endl;
	std::cout << "Konfiguracja w pliku config.dat" << std::endl;
	std::string command;
	while (true) {
		std::cout << " > ";
		std::cout.flush();
		std::getline(std::cin, command);
		parseAndRun(command);
	}
}

void UI::parseAndRun(const std::string& cmd) {
	const size_t npos = std::string::npos;
	//create, free, get, set, observe, help
	size_t cmd_len = cmd.find(" ");
	std::string command = cmd_len != npos ? cmd.substr(0, cmd_len) : cmd;
	std::string args = cmd_len != npos ? cmd.substr(cmd_len + 1, npos) : "";
	if (command == "create") {
		model_.dhCreate(args);
	} else if (command == "free") {
		model_.dhFree(args);
	} else if (command == "get") {
		try {
			std::cout << args << " = " << model_.dhGet(args) << std::endl;
		} catch (std::exception& e) {
			std::cout << e.what() << std::endl;
		}
	} else if (command == "set") {
		size_t last_space = args.find_last_of(" ");
		std::string var = last_space != npos ? args.substr(0, last_space) : args;
		std::string val_str = last_space != npos ? args.substr(last_space + 1, npos) : "0";
		std::stringstream ss;
		ss.str(val_str);
		int64 value;
		ss >> value;
		model_.dhSet(var, value);
	} else if (command == "observe") {
		std::cout << "Obserwuje..." << std::endl;  // TODO
	} else if (command == "help") {
		std::cout << "HELP HERE" << std::endl;  // TODO
	} else {
		std::cout << "Nie wiem co z tym zrobic" << std::endl;
	}
}

