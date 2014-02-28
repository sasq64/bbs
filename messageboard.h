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
	msgboard_exception(const char *ptr = "Messageboard Exception") : msg(ptr) {
	}
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
			: id(id), first_msg(first_msg), group(group), name(n), creator(s), msg_count(0), unread_count(0), byme_count(0) {}
		uint64_t id;
		uint64_t first_msg;
		uint64_t group;
		std::string name;
		uint64_t creator;
		// ex data
		int msg_count;
		int unread_count;
		int byme_count;	
	};

	struct Message {
		Message(uint64_t id = 0, const std::string &t = "", uint64_t topic = 0, uint64_t creator = 0, uint64_t parent = 0, uint64_t ts = 0, bool r = true)
			: id(id), text(t), topic(topic), creator(creator), parent(parent), timestamp(ts), read(r) {
				LOGD("Message(%d,%s)", id, text);
			}
		uint64_t id;
		std::string text;
		uint64_t topic;
		uint64_t creator;
		uint64_t parent;
		uint64_t timestamp;
		bool read;
	};

	uint64_t current_user() { return currentUser; }

	Message get_message(uint64_t msgid);
	Topic get_topic(uint64_t topicid);
	Group get_group(uint64_t groupid);
	Group get_group(const std::string &name);

	void flush_bits();
	void update_bits();

	uint64_t get_first_unread_msg() {
		uint64_t mid = msgbits.lowest_set()+1;
		if(mid >= last_msg())
			return 0;
		return mid;
	}

	uint64_t first_msg() {
		return 1;
	}

	uint64_t last_msg() {
		uint64_t maxm = -1;
		auto q = db.query<uint64_t>("SELECT MAX(ROWID) FROM message");
		try {
			if(q.step())
				maxm = q.get();
		} catch(sqlite3db::db_exception &e) {
			LOGD(e.what());
		}
		LOGD("LAST MSG %d", maxm);
		return maxm+1;
	}

	bool is_read(int msgid) {
		return !msgbits.get(msgid-1);
	}
	void mark_read(int msgid) {
		if(msgid < 1)
			throw msgboard_exception("Illegal msgid");
		LOGD("Msg %d read", msgid);
		msgbits.set(msgid-1, false);
	}

	Group current_group() {
		return currentGroup;
	}

	std::vector<Message> get_replies(uint64_t id);

	void join_group(uint64_t group);
	Group join_group(const std::string &group);

	Topic next_unread_topic(uint64_t group);
	Group next_unread_group();



	uint64_t create_group(const std::string &name);
	const std::vector<Group> list_groups();
	Group enter_group(const std::string &group_name);
	Group enter_group(uint64_t groupid);
	const std::vector<Topic> list_topics(uint64_t group);
	const std::vector<Message> list_messages(uint64_t topic_id);
	uint64_t post(const std::string &topic_name, const std::string &text);
	uint64_t reply(uint64_t msgid, const std::string &text);
private:
	sqlite3db::Database &db;
	uint64_t currentUser;
	Group currentGroup;

	// Bits mark if message is UNREAD.
	BitField msgbits;
	uint64_t lastEnd;
};

#endif // MESSAGEBOARD_H