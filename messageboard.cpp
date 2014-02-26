
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
	db.exec("CREATE TABLE IF NOT EXISTS joinedgroups (user INT, groupid INT, FOREIGN KEY(groupid) REFERENCES msggroup(ROWID))");
	db.exec("CREATE TABLE IF NOT EXISTS msgbits (user INT, highmsg INT, bits BLOB, PRIMARY KEY(user))");


	LOGD("User %d on board", userId);

	//auto lm = last_msg();
	//msgbits.grow(lm);
	lastEnd = 0;

	auto query = db.query<BitField::storage_type>("SELECT bits FROM msgbits WHERE user=?", userId);
	if(query.step())
		msgbits = BitField(query.get());

	auto query2 = db.query<uint64_t>("SELECT highmsg FROM msgbits WHERE user=?", userId);
	if(query2.step())
		lastEnd = query2.get();

	update_bits();
}

void MessageBoard::flush_bits() {
	msgbits.grow(1);

	update_bits();
	db.exec("INSERT OR REPLACE INTO msgbits(user,highmsg,bits) VALUES (?,?,?)", currentUser, lastEnd, msgbits.get_vector());
}

void MessageBoard::update_bits() {
	// Every new message added since last time in joined groups should get bits set.
	LOGD("Updating %d to %d", lastEnd, last_msg());
	auto q = db.query<uint64_t>("SELECT message.ROWID FROM message,msgtopic,joinedgroups WHERE message.ROWID>? AND message.topicid=msgtopic.ROWID AND joinedgroups.groupid=msgtopic.groupid AND joinedgroups.user=? AND message.creatorid!=?", lastEnd, currentUser, currentUser);
	while(q.step()) {
		LOGD("Setting msg %d as unread", q.get());
		msgbits.set(q.get()-1, true);
	}
	lastEnd = last_msg();
}

void MessageBoard::join_group(uint64_t group) {
	auto exists = db.query<uint64_t>("SELECT EXISTS(SELECT 1 FROM joinedgroups WHERE user=? AND groupid=?)", currentUser, group).get();
	if(!exists) {
		LOGD("Joining group %d", group);
		db.exec("INSERT OR REPLACE INTO joinedgroups(user,groupid) VALUES (?,?)", currentUser, group);
		auto q = db.query<uint64_t>("SELECT message.ROWID FROM message,msgtopic WHERE msgtopic.groupid=? AND message.topicid=msgtopic.ROWID", group);
		while(q.step()) {
			LOGD("NEW GROUP MSGID %d MARK AS UNREAD", q.get());
			msgbits.set(q.get()-1, true);
		}
	}
}

MessageBoard::Group MessageBoard::join_group(const std::string &group) {
	auto g = get_group(group);
	join_group(g.id);
	return g;
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

MessageBoard::Group MessageBoard::next_unread_group() {
	auto msgid = get_first_unread_msg();
	if(msgid < 1) {
		return 0;
	}
	LOGD("unread %d", msgid);
	auto msg = get_message(msgid);
	auto topic = get_topic(msg.topic);
	auto group = get_group(topic.group);
	return group;
}

MessageBoard::Topic MessageBoard::next_unread_topic(uint64_t group) {
	// TODO: Keep track of lowest unread message and start from there
	auto q = db.query<uint64_t, uint64_t>("SELECT message.ROWID,topicid FROM message,msgtopic WHERE topicid=msgtopic.ROWID AND msgtopic.groupid=?", group);
	while(q.step()) {
		auto t = q.get_tuple();
		LOGD("Found msg %d in topic %d", get<0>(t), get<1>(t));
		if(msgbits.get(get<0>(t)-1)) {
			LOGD("UNREAD");
			return get_topic(get<1>(t));
		} 
	}
	return Topic(0);
}


const std::vector<MessageBoard::Topic> MessageBoard::list_topics(uint64_t group) {
	unordered_map<uint64_t, Topic*> topic_map;
	vector<Topic> topics;

	auto q = db.query<uint64_t, uint64_t, uint64_t, uint64_t>("SELECT message.ROWID,topicid,message.creatorid,timestamp FROM message,msgtopic WHERE topicid=msgtopic.ROWID AND msgtopic.groupid=?", group);
	while(q.step()) {
		auto t = q.get_tuple();
		auto mid = get<0>(t);
		auto topicid = get<1>(t);
		auto creator = get<2>(t);

		if(topic_map.count(topicid) == 0) {
			topics.push_back(get_topic(topicid));
			topic_map[topicid] = &(topics.back());
		}
		Topic *ft = topic_map[topicid];

		if(creator == currentUser) {
			ft->byme_count++;
		}
		if(msgbits.get(mid-1)) {
			ft->unread_count++;
		}
		ft->msg_count++;
	}
	return topics;
	
}

/*
const std::vector<MessageBoard::Topic> MessageBoard::list_topics() {
	vector<Topic> topics;
	auto q = db.query<uint64_t, uint64_t, uint64_t, string, uint64_t>("SELECT ROWID,firstmsg,groupid,name,creatorid FROM msgtopic WHERE groupid=?", currentGroup.id);
	while(q.step()) {
		topics.push_back(q.get<Topic>());
	}
	return topics; // NOTE: std::move ?
}
*/

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
	LOGD("Query msg");
	auto q = db.query<uint64_t, string, uint64_t, uint64_t, uint64_t, uint64_t>("SELECT ROWID,contents,topicid,creatorid,parentid,timestamp FROM message WHERE ROWID=?", msgid);
	LOGD("Step msg");
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

	uint64_t id, tid;
	MessageBoard::Message msg;
	MessageBoard::Group group;
	MessageBoard::Topic topic;


	try {
		File::remove("test.db");
	} catch(io_exception &e) {}	

	sqlite3db::Database db { "test.db" };
	MessageBoard mb { db, 0 };

	auto mid = mb.create_group("misc");
	auto cid = mb.create_group("coding");

	REQUIRE(mid > 0);
	REQUIRE(cid > 0);

	// Post Msg 1 to 3 in group "misc"
	mb.join_group("misc");
	group = mb.enter_group("misc");
	id = mb.post("First post", "In the misc group");
	REQUIRE(id == 1);
	id = mb.post("Second post", "Also in the misc group");
	REQUIRE(id == 2);
	id = mb.reply(id, "This is a reply");
	REQUIRE(id == 3);

	topic = mb.next_unread_topic(group.id);
	REQUIRE(topic.id == 0);

	mb.enter_group("coding");
	id = mb.post("C++", "In the coding group");
	REQUIRE(id == 4);


	// Second user

	MessageBoard mb2 { db, 1 };

	id = mb2.get_first_unread_msg();
	REQUIRE(id == 0);

	group = mb2.next_unread_group();
	REQUIRE(group.id == 0);


	mb2.join_group("misc");
	group = mb2.next_unread_group();
	REQUIRE(group.id == mid);

	mb2.enter_group("misc");
	topic = mb2.next_unread_topic(group.id);
	REQUIRE(topic.id == 1);

	REQUIRE(mb2.get_first_unread_msg() == 1);
	mb2.mark_read(1);
	REQUIRE(mb2.get_first_unread_msg() == 2);
	mb2.mark_read(2);
	mb2.mark_read(3);

	topic = mb2.next_unread_topic(group.id);
	REQUIRE(topic.id == 0);

	topic = mb2.get_topic(1);

	id = mb2.reply(topic.first_msg, "This is a reply");
	msg = mb2.get_message(id);
	REQUIRE(msg.topic == topic.id);

	mb2.post("Third post", "Hello again");

	topic = mb2.next_unread_topic(group.id);
	REQUIRE(topic.id == 0);

	// First user again
	//mb.update_bits();
	{
	MessageBoard mb { db, 0 };


	topic = mb.next_unread_topic(group.id);
	REQUIRE(topic.id > 0);
	id = mb.get_first_unread_msg();
	REQUIRE(id == 5);
	mb.mark_read(id);
	id = mb.get_first_unread_msg();
	REQUIRE(id == 6);
	mb.mark_read(id);

	id = mb.get_first_unread_msg();
	REQUIRE(id == 0);

	mb2.join_group("coding");
	group = mb2.next_unread_group();
	REQUIRE(group.id == 2);
	topic = mb2.next_unread_topic(group.id);
	REQUIRE(topic.id == 3);

	id = mb2.get_first_unread_msg();
	REQUIRE(id == 4);
	mb2.mark_read(id);

	id = mb2.get_first_unread_msg();
	REQUIRE(id == 0);
}
}

#endif