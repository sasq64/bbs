
#include "messageboard.h"

#include <coreutils/log.h>
#include <coreutils/file.h>

#include <chrono>

using namespace std;
using namespace utils;
using std::chrono::system_clock;

uint64_t get_timestamp() {
	auto tp = system_clock::now();
	auto ts = system_clock::to_time_t(tp);
	return ts;
}

MessageBoard::MessageBoard(sqlite3db::Database &db, uint64_t userId) : db(db), currentUser(userId) {

	db.exec("CREATE TABLE IF NOT EXISTS msggroup (name TEXT, creatorid INT)");
	db.exec("CREATE TABLE IF NOT EXISTS msgtopic (name TEXT, creatorid INT, groupid INT, firstmsg INT, FOREIGN KEY(groupid) REFERENCES msggroup(ROWID), FOREIGN KEY(firstmsg) REFERENCES message(ROWID))");
	db.exec("CREATE TABLE IF NOT EXISTS message (contents TEXT, creatorid INT, parentid INT, topicid INT, timestamp INT, FOREIGN KEY(parentid) REFERENCES message(ROWID), FOREIGN KEY(topicid) REFERENCES msgtopic(ROWID))");
	db.exec("CREATE TABLE IF NOT EXISTS msgbits (user INT, bits BLOB, PRIMARY KEY(user))");

	LOGD("User %d on board", userId);

	//auto lm = last_msg();
	//msgbits.grow(lm);

	auto query = db.query<BitField::storage_type>("SELECT bits FROM msgbits WHERE user=?", userId);
	msgbits = BitField(query.get());
}

void MessageBoard::flush_bits() {
	msgbits.grow(1);
	db.exec("INSERT OR REPLACE INTO msgbits(user,bits) VALUES (?,?)", currentUser, msgbits.get_vector());
}

uint64_t MessageBoard::create_group(const std::string &name) {
	db.exec("INSERT INTO msggroup (name, creatorid) VALUES (?, ?)", name, currentUser);
	return db.last_rowid();
}

const std::vector<MessageBoard::Group> MessageBoard::list_groups() {
	vector<Group> groups;
	auto q = db.query<uint64_t, string, uint64_t>("SELECT ROWID,name,creatorid FROM msggroup");
	while(q.step()) {
		groups.push_back(q.get<Group>());
	}
	return groups; // NOTE: std::move ?
}

MessageBoard::Group MessageBoard::enter_group(uint64_t id) {
	currentGroup = get_group(id);
	return currentGroup;
}

MessageBoard::Group MessageBoard::enter_group(const std::string &group_name) {
	currentGroup = get_group(group_name);
	return currentGroup;
}

const std::vector<MessageBoard::Topic> MessageBoard::list_topics() {
	vector<Topic> topics;
	auto q = db.query<uint64_t, uint64_t, uint64_t, string, uint64_t>("SELECT ROWID,firstmsg,groupid,name,creatorid FROM msgtopic WHERE groupid=?", currentGroup.id);
	while(q.step()) {
		topics.push_back(q.get<Topic>());
	}
	return topics; // NOTE: std::move ?
}

const std::vector<MessageBoard::Message> MessageBoard::list_messages(uint64_t topic_id) {
	vector<Message> messages;	
	auto q = db.query<uint64_t, string, uint64_t, uint64_t, uint64_t, uint64_t>("SELECT ROWID,contents,topicid,creatorid,parentid,timestamp FROM message WHERE topicid=?", topic_id);
	while(q.step()) {
		messages.push_back(q.get<Message>());
	}
	return messages; // NOTE: std::move ?		
}

vector<MessageBoard::Message> MessageBoard::get_replies(uint64_t id) {
	vector<Message> messages;
	auto q = db.query<uint64_t, string, uint64_t, uint64_t, uint64_t, uint64_t>("SELECT ROWID,contents,topicid,creatorid,parentid,timestamp FROM message WHERE parentid=?", id);
	while(q.step()) {
		messages.push_back(q.get<Message>());
	}
	return messages; // NOTE: std::move ?		
}

uint64_t MessageBoard::post(const std::string &topic_name, const std::string &text) {
	auto ta = db.transaction();

	auto ts = get_timestamp();
	if(currentGroup.id < 1)
		throw msgboard_exception("No current group");

	db.exec("INSERT INTO msgtopic (name,creatorid,groupid) VALUES (?, ?, ?)", topic_name, currentUser, currentGroup.id);
	auto topicid = db.last_rowid();
	db.exec("INSERT INTO message (contents, creatorid, parentid, topicid, timestamp) VALUES (?, ?, 0, ?, ?)", text, currentUser, topicid, ts);
	auto msgid = db.last_rowid();
	db.exec("UPDATE msgtopic SET firstmsg=? WHERE ROWID=?", msgid, topicid);
	mark_read(msgid);

	ta.commit();
	return msgid;
}

uint64_t MessageBoard::reply(uint64_t msgid, const std::string &text) {

	uint64_t topicid = db.query<uint64_t>("SELECT topicid FROM message WHERE ROWID=?", msgid).get();
	if(topicid == 0) throw msgboard_exception("Repy failed, no such topic");
	auto ts = get_timestamp();
	db.exec("INSERT INTO message (contents, creatorid, parentid, topicid, timestamp) VALUES (?, ?, ?, ?, ?)", text, currentUser, msgid, topicid, ts);

	msgid = db.last_rowid();
	mark_read(msgid);

	return msgid;		
}

MessageBoard::Message MessageBoard::get_message(uint64_t msgid) {
	auto q = db.query<uint64_t, string, uint64_t, uint64_t, uint64_t, uint64_t>("SELECT ROWID,contents,topicid,creatorid,parentid,timestamp FROM message WHERE ROWID=?", msgid);
	if(q.step())
		return q.get<Message>();
	else
		throw msgboard_exception("No such message");
};

MessageBoard::Topic MessageBoard::get_topic(uint64_t id) {
	Topic topic;
	auto q = db.query<uint64_t, uint64_t, uint64_t, string, uint64_t>("SELECT ROWID,firstmsg,groupid,name,creatorid FROM msgtopic WHERE ROWID=?", id);
	if(q.step())
		return q.get<Topic>();
	else
		throw msgboard_exception("No such topic");
};

MessageBoard::Group MessageBoard::get_group(uint64_t id) {
	auto q = db.query<uint64_t, string, uint64_t>("SELECT ROWID,name,creatorid FROM msggroup WHERE ROWID=?", id);
	if(q.step())
		return q.get<Group>();
	else
		throw msgboard_exception("No such group");
};

MessageBoard::Group MessageBoard::get_group(const std::string &name) {
	auto q = db.query<uint64_t, string, uint64_t>("SELECT ROWID,name,creatorid FROM msggroup WHERE name=?", name);
	if(q.step())
		return q.get<Group>();
	else
		throw msgboard_exception("No such group");
};

#ifdef MY_UNIT_TEST

#include "catch.hpp"

TEST_CASE("msgboard", "Messageboard test") {

	using namespace utils;
	using namespace std;

	try {
		File::remove("test.db");
	} catch(io_exception &e) {}	

	sqlite3db::Database db { "test.db" };
	MessageBoard mb { db, 0 };

	auto mid = mb.create_group("misc");
	auto cid = mb.create_group("coding");

	REQUIRE(mid > 0);
	REQUIRE(cid > 0);

	mb.enter_group(mid);

	uint64_t id;

	id = mb.post("First post", "In the misc group");
	REQUIRE(id > 0);
	id = mb.post("Second post", "Also in the misc group");
	REQUIRE(id > 0);
	id = mb.reply(id, "This is a reply");
	REQUIRE(id > 0);

	MessageBoard mb2 { db, 1 };

	mb2.enter_group("misc");
	mb2.post("Third post", "Hello again");

	auto msgid = mb2.get_first_unread_msg();
	LOGD("ID %d", msgid);
	auto msg = mb2.get_message(msgid);
	auto topic = mb2.get_topic(msg.topic);
	REQUIRE(msgid == 1);
	REQUIRE(topic.name == "First post");
	mb2.mark_read(1);
	REQUIRE(mb2.get_first_unread_msg() == 2);
	mb2.mark_read(2);
	REQUIRE(mb2.get_first_unread_msg() == 3);

	msgid = mb.get_first_unread_msg();
	REQUIRE(msgid == 4);
	mb.reply(msgid, "Hello to you too!");

	mb2.mark_read(3);
	REQUIRE(mb2.get_first_unread_msg() == 5);

}

#endif