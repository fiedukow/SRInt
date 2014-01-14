#pragma once
#include <string>
#include "DBObserver.h"

class SRInt;

class UI : public DBObserver {
public:
	UI(SRInt& model);
	virtual ~UI();

	void operator()();

	/* Overriten from DBObserver */
	virtual void CallDoneNotify(const std::string& call_message, bool status, DB& db);
	virtual void LocalChangeNotify(const std::string& name, DB& db);
	virtual void GlobalChangeNotify(const std::string& name, int old_value, int new_value, DB& db);

private:
	void parseAndRun(const std::string&);
	void showPrompt();

	SRInt& model_;
};

