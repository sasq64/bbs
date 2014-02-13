
#include "messageboard.h"

#include <coreutils/log.h>
#include <coreutils/file.h>

using namespace std;
using namespace utils;

MessageBoard::MessageBoard(sqlite3db::Database &db, uint64_t userId) : db(db), currentUser(userId) {

	db.exec("CREATE TABLE IF NOT EXISTS msggroup (name TEXT, creatorid INT)");
	db.exec("CREATE TABLE IF NOT EXISTS msgtopic (name TEXT, creatorid INT, groupid INT, firstmsg INT, FOREIGN KEY(groupid) REFERENCES msggroup(ROWID), FOREIGN KEY(firstmsg) REFERENCES message(ROWID))");
	db.exec("CREATE TABLE IF NOT EXISTS message (contents TEXT, creatorid INT, parentid INT, topicid INT, timestamp INT, FOREIGN KEY(parentid) REFERENCES message(ROWID), FOREIGN KEY(topicid) REFERENCES msgtopic(ROWID))");
	db.exec("CREATE TABLE IF NOT EXISTS msgbits (user INT, bits BLOB, PRIMARY KEY(user))");

	LOGD("User %d on board", userId);

	auto lm = last_msg();
	msgbits.grow(lm);
	db.get_blob(format("SELECT bits FROM msgbits WHERE user=%d", userId), 0, msgbits.get_vector());

	//File mfile { format("msgbits.%d", userId) };
	//if(mfile.exists()) {
	//	msgbits.set(mfile.getPtr(), mfile.getSize()*8);
	//}
}

void MessageBoard::flush_bits() {
	//File mfile { format("msgbits.%d", currentUser) };
	//mfile.write((const uint8_t*)msgbits.get(), msgbits.size()*8);
	//mfile.close();
	msgbits.grow(1);
	db.put_blob(format("INSERT OR REPLACE INTO msgbits(user,bits) VALUES (%d,?)", currentUser), 1, msgbits.get_vector());
}

uint64_t MessageBoard::create_group(const std::string &name) {
	db.exec("INSERT INTO msggroup (name, creatorid) VALUES (%Q, %d)", name, currentUser);
	return db.last_rowid();
}

const std::vector<MessageBoard::Group> MessageBoard::list_groups() {
	vector<MessageBoard::Group> groups;
	db.execf("SELECT ROWID,name,creatorid FROM msggroup", [&](int i, const vector<string> &result) {
		groups.emplace_back(std::stol(result[0]), result[1], std::stol(result[2]));
	});
	return groups; // NOTE: std::move ?
}

uint64_t MessageBoard::enter_group(uint64_t id) {
	currentGroup = id;
	return id;
}

uint64_t MessageBoard::enter_group(const std::string &group_name) {
	uint64_t id = 0;
	//uint64_t lastpost = 0;
	db.execf("SELECT ROWID FROM msggroup WHERE name=%Q", [&](int i, const vector<string> &result) {
		id = std::stol(result[0]);
		//lastpost = std::stol(result[1]);
	}, group_name);
	//auto id = db.select_first<pair<uint64_t, uint64_t>>("SELECT ROWID,lastpost FROM msggroup WHERE name=%Q", group_name);
	//LOGD("Last post %d", lastpost);
	if(id != 0) {
		currentGroup = id;
	}
	return id;
}

const std::vector<MessageBoard::Topic> MessageBoard::list_topics() {
	vector<MessageBoard::Topic> topics;
	LOGD("currentGroup %d", currentGroup);
	db.execf("SELECT ROWID,firstmsg,groupid,name,creatorid FROM msgtopic WHERE groupid=%d", [&](int i, const vector<string> &result) {
		LOGD(result[1]);
		topics.emplace_back(std::stol(result[0]), std::stol(result[1]), std::stol(result[2]), result[3], std::stol(result[4]));
	}, currentGroup);
	return topics; // NOTE: std::move ?
}

const std::vector<MessageBoard::Message> MessageBoard::list_messages(uint64_t topic_id) {
	vector<MessageBoard::Message> messages;
	db.execf("SELECT ROWID,contents,topicid,creatorid,parentid,timestamp FROM message WHERE topicid=%d", [&](int i, const vector<string> &result) {
		LOGD(result[1]);
		//messages.emplace_back(std::stol(result[0]), result[1]);
		messages.emplace_back(std::stol(result[0]), result[1], std::stol(result[2]), std::stol(result[3]), std::stol(result[4]), std::stol(result[5]));
	}, topic_id);
	return messages; // NOTE: std::move ?		
}

vector<MessageBoard::Message> MessageBoard::get_replies(uint64_t id) {
	vector<MessageBoard::Message> messages;
	db.execf("SELECT ROWID,contents,topicid,creatorid,parentid,timestamp FROM message WHERE parentid=%d", [&](int i, const vector<string> &result) {
		messages.emplace_back(std::stol(result[0]), result[1], std::stol(result[2]), std::stol(result[3]), std::stol(result[4]), std::stol(result[5]));
	}, id);
	return messages; // NOTE: std::move ?		
}

uint64_t MessageBoard::post(const std::string &topic_name, const std::string &text) {
	auto ta = db.transaction();
	uint64_t ts = 12345;

	if(currentGroup < 1)
		throw msgboard_exception("No current group");

	db.exec("INSERT INTO msgtopic (name,creatorid,groupid) VALUES (%Q, %d, %d)", topic_name, currentUser, currentGroup);
	auto topicid = db.last_rowid();
	db.exec("INSERT INTO message (contents, creatorid, parentid, topicid, timestamp) VALUES (%Q, %d, 0, %d, %d)", text, currentUser, topicid, ts);
	auto msgid = db.last_rowid();
	db.exec("UPDATE msgtopic SET firstmsg=%d WHERE ROWID=%d", msgid, topicid);

	mark_read(msgid);

	//time_t t;
	//time(&t);
	//long d = 12345;
	//db.exec("UPDATE msggroup SET lastpost=%d,lastby=%d WHERE ROWID=%d", t, currentUser,currentGroup);
	//db.exec("UPDATE msgtopic SET lastpost=%d,lastby=%d WHERE ROWID=%d", t, currentUser, topicid);
	//db.exec("INSERT INTO readmsg (userid, msgid) VALUES (%d,%d)", currentUser, msgid);
	//db.exec("INSERT INTO groupstate (userid, groupid) VALUES (%d, %d)", currentUser, currentGroup);
	//db.exec("INSERT INTO topicstate (userid, topicid) VALUES (%d, %d)", currentUser, currentGroup);
	ta.commit();
	return msgid;
}

uint64_t MessageBoard::reply(uint64_t msgid, const std::string &text) {

	uint64_t topicid = 0;
	db.execf("SELECT topicid FROM message WHERE ROWID=%d", [&](int i, const vector<string> &result) {
		topicid = std::stol(result[0]);
	}, msgid);
	if(topicid == 0) throw msgboard_exception("Repy failed, no such topic");
	uint64_t ts = 12345;
	db.exec("INSERT INTO message (contents, creatorid, parentid, topicid, timestamp) VALUES (%Q, %d, %d, %d, %d)", text, currentUser, msgid, topicid, ts);

	msgid = db.last_rowid();
	mark_read(msgid);

	return msgid;		
}

MessageBoard::Message MessageBoard::get_message(uint64_t msgid) {
	Message msg;
	db.execf("SELECT contents,topicid,creatorid,parentid,timestamp FROM message WHERE ROWID=%d", [&](int i, const vector<string> &result) {
		msg = Message(msgid, result[0], std::stol(result[1]), std::stol(result[2]), std::stol(result[3]), std::stol(result[4]));
	}, msgid);
	if(msg.id == 0) throw msgboard_exception("No such message");
	return msg;
};

MessageBoard::Topic MessageBoard::get_topic(uint64_t id) {
	Topic topic;
	db.execf("SELECT msgtopic.ROWID,firstmsg,groupid,name,creatorid FROM msgtopic WHERE msgtopic.ROWID=%d", [&](int i, const vector<string> &result) {
		topic = Topic(std::stol(result[0]), std::stol(result[1]), std::stol(result[2]), result[3], std::stol(result[4]));
	}, id);
	if(topic.id == 0) throw msgboard_exception("No such topic");
	return topic;
};

MessageBoard::Group MessageBoard::get_group(uint64_t id) {
	Group group;
	db.execf("SELECT ROWID,name FROM msggroup WHERE ROWID=%d", [&](int i, const vector<string> &result) {
		group = Group(std::stol(result[0]), result[1]);
	}, id);
	if(group.id == 0) throw msgboard_exception("No such group");
	return group;
};

MessageBoard::Group MessageBoard::get_group(const std::string &name) {
	Group group;
	db.execf("SELECT ROWID,name FROM msggroup WHERE name=%Q", [&](int i, const vector<string> &result) {
		group = Group(std::stol(result[0]), result[1]);
	}, name);
	if(group.id == 0) throw msgboard_exception("No such group");
	return group;
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