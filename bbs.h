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

class BBS {
public:

	class Session {
	public:

		Session(uint64_t id) : id(id) {}

		std::string format(std::string templ);
		std::unordered_map<std::string, std::string>& vars() { return variables; }
		uint64_t user_id() { return id; }
	private:
		uint64_t id;
		std::unordered_map<std::string, std::string> variables;
	};

	static std::string init_dbname;

	static BBS& instance() {
		static BBS bbs { init_dbname };
		return bbs;
	}

	static void init(const std::string &dbname) {
		init_dbname = dbname;
	}

	std::string get_user_data(uint64_t user, const std::string &what);
	void set_user_data(uint64_t user, const std::string &what, const std::string &data);

	Session login(const std::string &handle, const std::string &password);
	std::string get_text(const std::string &name, const std::string &what);
	std::unordered_map<std::string, std::string>& text() { return texts; }
	LoginManager& getlm() { return loginmanager; }
	sqlite3db::Database& getdb() { return db; }

private:

	BBS(const std::string &dbname);

	sqlite3db::Database db;
	LoginManager loginmanager;

	std::unordered_map<std::string, std::string> texts;
	std::unordered_set<uint64_t> logged_in;
	std::mutex lock;
};

#endif // BBS_H
