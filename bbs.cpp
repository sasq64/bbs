#include "bbs.h"

using namespace std;
using namespace utils;

string BBS::init_dbname;
//BBS BBS::NO_BBS;

string BBS::Session::format(string templ) {
	size_t tstart = 0;
	while(tstart != string::npos) {
		tstart = templ.find("%[", tstart);
		if(tstart != string::npos) {
			auto tend = templ.find("]", tstart+2);
			auto var = templ.substr(tstart+2, tend-tstart-2);
			LOGD("Found var %s at %d,%d", var, tstart, tend);
			if(variables.count(var) > 0)
				templ.replace(tstart, tend-tstart+1, variables[var]);
			tstart++;
		}
	}
	return templ;
}
	
BBS::Session BBS::login(const string &handle, const string &password) {
	auto id = loginmanager.verify_user(handle, password);

	lock_guard<mutex> guard(lock);
	if(id > 0)
		logged_in.insert(id);
	return Session(id);
}

void BBS::logout(uint64_t id) {
}

string BBS::get_text(const string &name, const string &what) {
	File f;
	if(name == "ansi") {		
		f = File("text/" + what + ".ans");
	} else if (name == "ansi") {
		f = File("text/" + what + ".pet");
	}
	if(!f.exists())
		f = File("text/" + what + ".txt");

	return f.read();
}

string BBS::get_user_data(uint64_t user, const string &what) {
	auto q = db.query<string>("SELECT data FROM metadata WHERE user=? AND what=?", user, what);
	if(q.step())
		return q.get();
	return "";
}

bool BBS::change_password(const std::string &handle, const std::string &newp, const std::string &oldp) {
	return loginmanager.change_password(handle, newp, oldp);
}

void BBS::set_user_data(uint64_t user, const string &what, const string &data) {
	db.exec("INSERT OR REPLACE INTO metadata(user,what,data) VALUES (?,?,?)", user, what, data);
}

BBS::BBS(const string &dbname) : db { dbname }, loginmanager { db } {

	db.exec("PRAGMA synchronous = OFF");
	db.exec("CREATE TABLE IF NOT EXISTS metadata (user INT, what STRING, data STRING, PRIMARY KEY(user, what))");

	if(loginmanager.get_id("admin") == LoginManager::NO_USER) {
		loginmanager.add_user("admin", "password");
	}


	File f { "text.cfg" };
	auto lines = f.getLines();
	for(const auto &l : lines) {
		if(l[0] != '#' && l[0] != '!') {
			auto parts = split(l, "=");
			if(parts.size() == 2)
				texts[parts[0]] = parts[1];
			else
				LOGD("Illegal line '%s'", l);
		}
	}
}
