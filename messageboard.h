#ifndef MESSAGEBOARD_H
#define MESSAGEBOARD_H

#include <cstdint>
#include <string>
#include <vector>
#include <sqlite3/database.h>

class MessageBoard {
public:

	MessageBoard(sqlite3db::Database &db) : db(db) {}

	struct Group {
		Group(uint64_t id, const std::string &n, uint64_t lp = 0,  std::string c = "", uint64_t lr = 0) : id(id), name(n), last_post(lp), creator(c), last_read(lr) {}
		const uint64_t id;
		const std::string name;
		const uint64_t last_post;
		const  std::string creator;
		const uint64_t last_read;
	};

	struct Topic {
		Topic(uint64_t id, const std::string &n, const std::string &s = "", const std::string &l = "") : id(id), name(n), starter(s), lastPoster(l) {}
		const uint64_t id;
		const std::string name;
		const std::string starter;
		const std::string lastPoster;
	};

	struct Message {
		Message(uint64_t id, const std::string &t, const std::string &p = "") : id(id), text(t), poster(p) {}
		const uint64_t id;
		const std::string text;
		const std::string poster;
	};

	void login(uint64_t userid);
	uint64_t create_group(const std::string &name);
	const std::vector<Group> list_groups();
	uint64_t enter_group(const std::string &group_name);
	const std::vector<Topic> list_topics();
	const std::vector<Message> list_messages(uint64_t topic_id);
	uint64_t post(const std::string &topic_name, const std::string &text);
	uint64_t reply(uint64_t msgid, const std::string &text);
private:
	uint64_t currentUser;
	uint64_t currentGroup;
	sqlite3db::Database &db;
};

#endif // MESSAGEBOARD_H