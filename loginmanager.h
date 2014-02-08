#ifndef LOGINMANAGER_H
#define LOGINMANAGER_H

#include <cstdint>
#include <string>
#include <sqlite3/database.h>

class LoginManager {
public:
	struct User {
		User(uint64_t id = 0, const std::string &handle = "") : id(id), handle(handle) {}
		const uint64_t id;
		const std::string handle;
	};

	static const uint64_t NO_USER = 0;
	LoginManager(sqlite3db::Database &db) : db(db) {}
	uint64_t verify_user(const std::string &handle, const std::string &password);
	uint64_t get_user(const std::string &handle);
	uint64_t add_user(const std::string &handle, const std::string &password);
private:
	sqlite3db::Database &db;
};

#endif // LOGINMANAGER_H