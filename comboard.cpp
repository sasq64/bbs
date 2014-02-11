
#include "comboard.h"

#include <bbsutils/ansiconsole.h>
#include <bbsutils/editor.h>

using namespace bbs;
using namespace utils;
using namespace std;

ComBoard::ComBoard(LoginManager &lm, MessageBoard &board, Console &console) : users(lm), board(board), console(console), 
	commands { 			
		{ "list groups", [&](const vector<string> &args) { 
			LOGD("List groups");
			for(const auto &g : board.list_groups()) {
				//time_t tt = (time_t)g.last_post;
				//const char *t = ctime(&tt);
				console.write(format("%s [Created by %s] LAST POST: %d\n", g.name, users.get(g.creator), 0));
			}
		} },

		{ "list topics", [&](const vector<string> &args) { 
			LOGD("List topics");
			auto group = board.current_group();
			console.write("Topics in group '%s'", group.name);
			for(const auto &t : board.list_topics()) {
				console.write(format("%s (%d)\n", t.name, users.get(t.creator)));
			}
		} },

		{ "list unread", [&](const vector<string> &args) { 
			auto start = board.first_msg();
			auto end = board.last_msg();
			for(auto i = start; i < end; i++) {
				if(!board.is_read(i)) {
					auto msg = board.get_message(i);
					auto topic = board.get_topic(msg.topic);
					auto group = board.get_group(topic.group);
					console.write(format("#%d %s [%s] (%s)\n", i, topic.name, group.name, users.get(msg.creator)));
				}
			}
		} },

		{ "list all", [&](const vector<string> &args) { 
			auto start = board.first_msg();
			auto end = board.last_msg();
			for(auto i = start; i < end; i++) {
				auto msg = board.get_message(i);
				auto topic = board.get_topic(msg.topic);
				auto group = board.get_group(topic.group);
				console.write(format("#%d %s [%s] (%s)\n", i, topic.name, group.name, users.get(msg.creator)));
			}
		} },

		{ "next topic", [&](const vector<string> &args) {
			auto msgid = board.get_first_unread_msg();
			if(msgid < 1) {
				console.write("No more topics\n");
				return;
			}
			LOGD("unread %d", msgid);
			auto msg = board.get_message(msgid);
			auto topic = board.get_topic(msg.topic);
			auto group = board.get_group(topic.group);
			console.write("Group '%s' topic '%s'\n", group.name, topic.name);
			board.enter_group(group.id);
			select_topic(topic.id);
			//currentMsgThread = board.get_thread(topic.first_msgid);
			//auto msg_id = topic.first_msgid;
			msgid = find_first_unread(topic.first_msg);
			currentMsg = board.get_message(msgid);
			lastShown.id = 0;
		} },

		{ "read next", [&](const vector<string> &args) {
			if(currentMsg.id == 0)
				console.write("No more messages\n");
			else {
				show_message(currentMsg);
				board.mark_read(currentMsg.id);
				lastShown = currentMsg;
				auto topic = board.get_topic(currentMsg.topic);
				auto msgid = find_first_unread(topic.first_msg);
				if(msgid)
					currentMsg = board.get_message(msgid);
				else 
					currentMsg.id = 0;
			}

		} },

		{ "read", [&](const vector<string> &args) {
			uint64_t mid = std::stol(args[1]);
			auto msg = board.get_message(mid);
			show_message(msg);
			lastShown = msg;
			board.mark_read(mid);
		} },

		{ "create group", [&](const vector<string> &args) {
			auto g = board.create_group(args[2]);
			if(g == 0)
				console.write("Create group failed\n");
			else
				console.write(format("Group '%s' created\n", args[2]));
		} },

		{ "create user", [&](const vector<string> &args) {
			if(args.size() != 4) {
				console.write("Usage: create user [handle] [password]\n");
				return;
			}
			auto id = users.add_user(args[2], args[3]);
			if(id == 0)
				console.write("Create user failed\n");
			else
				console.write(format("User '%s' created\n", args[2]));
		} },

		{ "enter group", [&](const vector<string> &args) {
			auto g = board.enter_group(args[2]);
			if(g == 0)
				console.write("No such group\n");
			else {
				lastShown.id = 0;
			}
		} },

		{ "post", [&](const vector<string> &args) { 
			//LOGD("List groups"); 
			auto topic = console.getLine("TOPIC:");
			auto text = edit();
			board.post(topic, text);
			lastShown.id = 0;
		} },

		{ "reply", [&](const vector<string> &args) { 
			//LOGD("List groups"); 
			auto mid = lastShown.id;
			if(args.size() > 1)
				mid = std::stol(args[1]);
			if(mid > 0) {
				auto text = edit();
				board.reply(mid, text);
			} else
				console.write("Reply failed\n");
			lastShown.id = 0;	
		} },
	}
{}

void ComBoard::show_message(const MessageBoard::Message &msg) {
	auto topic = board.get_topic(msg.topic);
	console.write(format("Msg #%d by %s [%s%s]\n%s\n", msg.id, users.get(msg.creator), msg.parent == 0 ? "" : "Re:", topic.name, msg.text));
	auto replies = board.get_replies(msg.id);
	for(const auto &r : replies) {
		console.write("Reply #%d by %s\n", r.id, users.get(r.creator));
	}

	//console.write(currentMsg.text);
}

string ComBoard::edit() {
	auto ed = FullEditor(console);
	while(true) {
		auto rc = ed.update(500);
		if(rc == Console::KEY_F7) {
			break;
		}
	}
	return ed.getResult();
}

// Find first unread message in the thread rooted at msg_id
uint64_t ComBoard::find_first_unread(uint64_t msg_id) {
	if(!board.is_read(msg_id))
		return msg_id;
	//auto replies = get_replies(msg_id);
	for(const pair<uint64_t, MessageBoard::Message> &m : topicMap) {
		LOGD("Checking %d", m.first);
		if(m.second.parent == msg_id) {
			auto mid = find_first_unread(m.second.id);
			if(mid > 0)
				return mid;
		}
	}
	return 0;
}

void ComBoard::select_topic(uint64_t topic_id) {
	//unordered_map<uint64_t, MessageBoard::Message> msgmap;
	topicMap.clear();
	const auto messages = board.list_messages(topic_id);
	for(const auto &m : messages) {
		topicMap[m.id] = m;
	}
}


void ComBoard::exec(const string &line) {
	auto parts = split(line);
	if(parts.size() < 1)
		return;

	for(const auto &c : commands) {
		bool found = true;
		for(int i=0; i<c.text.size(); i++) {
			if(c.text[i].compare(0, parts[i].length(), parts[i]) != 0) {
				found = false;
				break;
			}
		}
		if(found) {
			c.func(parts);
			break;
		}
	}
}

#ifdef MY_UNIT_TEST

class LocalTerminal : public Terminal {
public:

	virtual void open() override {
	}

	virtual void close() override {
	}



	virtual int write(const std::vector<Char> &source, int len) { 
		return fwrite(&source[0], 1, len, stdout);
	}
	virtual int read(std::vector<Char> &target, int len) { 
		return fread(&target[0], 1, len, stdin);
	}
private:

};


#include "catch.hpp"

TEST_CASE("comboard", "ComBoard test") {

	using namespace utils;
	using namespace std;

	logging::useLogSpace("utils", false);

	try {
		File::remove("comtest.db");
	} catch(io_exception &e) {}	

	sqlite3db::Database db { "comtest.db" };

	LoginManager loginm { db };

	auto user0 = loginm.add_user("joker", "password");
	auto user1 = loginm.add_user("batman", "password");
	auto user2 = loginm.add_user("gordon", "password");

	MessageBoard mb { db, user0 };

	MessageBoard cmb { db, user1 };
	LocalTerminal lt;
	AnsiConsole console { lt }; 

	ComBoard cb { loginm, cmb, console };

	cb.exec("create group general");
	cb.exec("create group coding");

	mb.enter_group("coding");
	auto mid = mb.post("Coding topic", "Text in message");

	cb.exec("next topic");
	cb.exec("read next");


}

#endif