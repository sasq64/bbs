
#include "loginmanager.h"

#include <coreutils/log.h>
#include <coreutils/utils.h>
#include <crypto/sha256.h>
#include <time.h>

using namespace std;
using namespace utils;

uint64_t LoginManager::verify_user(const std::string &handle, const std::string &password) {
	auto s = utils::sha256(handle + "\t" + password);
	uint64_t id = NO_USER;
	auto q = db.query<uint64_t>("SELECT ROWID FROM bbsuser WHERE sha=?", s);
	if(q.step())
		id = q.get();
	return id;
}

uint64_t LoginManager::login_user(const std::string &handle, const std::string &password) {
	auto id = verify_user(handle, password);

	lock_guard<mutex> guard(lock);
	if(id > 0)
		logged_in.insert(id);
	return id;
}

void LoginManager::logout_user(uint64_t id) {
	lock_guard<mutex> guard(lock);
	logged_in.erase(id);
}

std::vector<std::string> LoginManager::list_logged_in() {
	lock_guard<mutex> guard(lock);

	vector<string> handles;
	for(auto &id : logged_in) {
		handles.push_back(db.query<string>("SELECT handle FROM bbsuser WHERE ROWID=?", id).get());
	}
	return handles;
}

std::vector<std::string> LoginManager::list_users() {
	vector<string> handles;
	auto q = db.query<string>("SELECT handle FROM bbsuser");
	while(q.step())
		handles.push_back(q.get());
	return handles;
}


uint64_t LoginManager::get_id(const std::string &handle) {
	uint64_t id = NO_USER;
	auto q = db.query<uint64_t>("SELECT ROWID FROM bbsuser WHERE handle=?", handle);
	if(q.step())
		id = q.get();
	return id;
}

std::string LoginManager::get(uint64_t id) {
	std::string handle = "NO USER";
	auto q = db.query<string>("SELECT handle FROM bbsuser WHERE ROWID=?", id);
	if(q.step())
		handle = q.get();
	return handle;
}

uint64_t LoginManager::add_user(const std::string &handle, const std::string &password) {
	auto sha = utils::sha256(handle + "\t" + password);
	db.query("INSERT INTO bbsuser (sha, handle) VALUES (?, ?)", sha, handle).step();
	return db.last_rowid();
}
