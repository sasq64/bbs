#ifndef MESSAGEBOARD_H
#define MESSAGEBOARD_H

#include <cstdint>
#include <string>
#include <vector>
#include <sqlite3/database.h>
#include <coreutils/bitfield.h>
#include <coreutils/log.h>

class msgboard_exception : public std::exception {
public:
	msgboard_exception(const char *ptr = "Messageboard Exception") : msg(ptr) {}
	virtual const char *what() const throw() { return msg; }
private:
	const char *msg;
};

class MessageBoard {
public:

	MessageBoard(sqlite3db::Database &db, uint64_t userId);

	struct Group {
		Group(uint64_t id = 0, const std::string &n = "", uint64_t c = 0)
			: id(id), name(n), creator(c) {}
		uint64_t id;
		std::string name;
		uint64_t creator;
	};

	struct Topic {
		Topic(uint64_t id = 0, uint64_t first_msg = 0, uint64_t group = 0, const std::string &n = "", uint64_t s = 0)
			: id(id), first_msg(first_msg), group(group), name(n), creator(s) {}
		uint64_t id;
		uint64_t first_msg;
		uint64_t group;
		std::string name;
		uint64_t creator;
	};

	struct Message {
		Message(uint64_t id = 0, const std::string &t = "", uint64_t topic = 0, uint64_t creator = 0, uint64_t parent = 0, bool r = true)
			: id(id), text(t), topic(topic), creator(creator), parent(parent), read(r) {}
		uint64_t id;
		std::string text;
		uint64_t topic;
		uint64_t creator;
		uint64_t parent;
		uint64_t timestamp;
		bool read;
	};

	Message get_message(uint64_t msgid);
	Topic get_topic(uint64_t topicid);
	Group get_group(uint64_t groupid);

	uint64_t get_first_unread_msg() {
		return msgbits.lowest_unset();
	}

	bool is_read(int pos) {
		return msgbits.get(pos);
	}
	void mark_read(int pos) {
		LOGD("Msg %d read", pos);
		msgbits.set(pos, true);
	}

	uint64_t create_group(const std::string &name);
	const std::vector<Group> list_groups();
	uint64_t enter_group(const std::string &group_name);
	uint64_t enter_group(uint64_t groupid);
	const std::vector<Topic> list_topics();
	const std::vector<Message> list_messages(uint64_t topic_id);
	uint64_t post(const std::string &topic_name, const std::string &text);
	uint64_t reply(uint64_t msgid, const std::string &text);
private:
	sqlite3db::Database &db;
	uint64_t currentUser;
	uint64_t currentGroup;
	BitField msgbits;
};

#endif // MESSAGEBOARD_H