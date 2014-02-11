
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
	db.execf("SELECT ROWID FROM bbsuser WHERE sha=%Q;", [&](int i, const vector<string> &result) {
		id = std::stol(result[0]);
	}, s);
	return id;
}

uint64_t LoginManager::get_id(const std::string &handle) {
	uint64_t id = NO_USER;
	db.execf("SELECT ROWID FROM bbsuser WHERE handle=%Q;", [&](int i, const vector<string> &result) {
		id = std::stol(result[0]);
	}, handle);
	return id;
}

std::string LoginManager::get(uint64_t id) {
	std::string handle = "NO USER";
	db.execf("SELECT handle FROM bbsuser WHERE ROWID=%d;", [&](int i, const vector<string> &result) {
		handle = result[0];
	}, id);
	return handle;
}

uint64_t LoginManager::add_user(const std::string &handle, const std::string &password) {
	auto sha = utils::sha256(handle + "\t" + password);
	db.exec("INSERT INTO bbsuser (sha, handle) VALUES (%Q, %Q)", sha, handle);
	return db.last_rowid();
}
