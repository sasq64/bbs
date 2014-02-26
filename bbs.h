#ifndef BBS_H
#define BBS_H

#include "loginmanager.h"

#include <coreutils/log.h>
#include <coreutils/utils.h>
#include <coreutils/file.h>
#include <sqlite3/database.h>

#include <unordered_map>
#include <string>
#include <mutex>
#include <memory>

#ifdef USE_T_DEFINE
#define _T(x) BBS::instance().translate(x)
#endif

class BBS {
public:

	class Session {

		struct Logout {
			Logout(uint64_t id) : id(id) {}
			~Logout() {
				if(id > 0)
					BBS::instance().logout(id);
			}
			uint64_t id;
		};

	public:

		Session() : id(0) {}
		Session(uint64_t id) : id(id), logout(std::shared_ptr<Logout>(new Logout(id))) {}

		std::string format(std::string templ);
		std::unordered_map<std::string, std::string>& vars() { return variables; }
		uint64_t user_id() { return id; }
	private:
		uint64_t id;
		std::unordered_map<std::string, std::string> variables;
		std::shared_ptr<Logout> logout;
	};

	static BBS& instance() {
		static BBS bbs { init_dbname };
		return bbs;
	}

	static BBS NO_BBS;

	static void init(const std::string &dbname) {
		init_dbname = dbname;
	}

	std::string get_user_data(uint64_t user, const std::string &what);
	void set_user_data(uint64_t user, const std::string &what, const std::string &data);

	Session login(const std::string &handle, const std::string &password);
	void logout(uint64_t id);
	bool verify_user(const std::string &handle, const std::string &password) {
		return loginmanager.verify_user(handle, password);
	}
	bool change_password(const std::string &handle, const std::string &oldp, const std::string &newp = "");

	std::vector<std::string> list_users() {
		return loginmanager.list_users();
	}
	std::vector<std::string> list_logged_in() {
		return loginmanager.list_logged_in();
	}
	uint64_t get_user_id(const std::string &handle) {
		return loginmanager.get_id(handle);
	}
	std::string get_user(uint64_t id) {
		return loginmanager.get(id);
	}
	uint64_t create_user(const std::string &handle, const std::string &password) {
		return loginmanager.add_user(handle, password);
	}

	std::string get_text(const std::string &name, const std::string &what);
	std::unordered_map<std::string, std::string>& text() { return texts; }
	//LoginManager& getlm() { return loginmanager; }
	sqlite3db::Database& getdb() { return db; }

private:

	static std::string init_dbname;

	BBS(const std::string &dbname);

	sqlite3db::Database db;
	LoginManager loginmanager;

	std::unordered_map<std::string, std::string> texts;
	std::unordered_set<uint64_t> logged_in;
	std::mutex lock;
};

#endif // BBS_H
