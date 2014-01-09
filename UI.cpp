#include "UI.h"
#include <iostream>

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
	std::cout << "Wykonuje: " << cmd << std::endl;
}

