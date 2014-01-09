#pragma once
#include <string>

class SRInt;

class UI
{
public:
	UI(SRInt& model);
	~UI();

	void operator()();

private:
	void parseAndRun(const std::string&);

	SRInt& model_;
};

