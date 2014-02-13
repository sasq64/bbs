
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
				write("~2%s ~3Created by %s ~0LAST POST: %d\n", g.name, users.get(g.creator), 0);
			}
		} },

		{ "flush", [&](const vector<string> &args) { 
			board.flush_bits();
		} },

		{ "logout", [&](const vector<string> &args) { 
		} },
		{ "quit", [&](const vector<string> &args) { 
		} },

		{ "list users", [&](const vector<string> &args) { 
			auto ulist = users.list_users();
			for(auto &u : ulist)
				write("%s\n", u);
		} },

		{ "who", [&](const vector<string> &args) { 
			auto ulist = users.list_logged_in();
			for(auto &u : ulist)
				write("%s\n", u);
		} },

		{ "list topics", "List all topics in the current group", [&](const vector<string> &args) { 
			LOGD("List topics");
			auto group = board.current_group();
			write("Topics in group '%s'", group.name);
			for(const auto &t : board.list_topics()) {
				write(format("%s (%d)\n", t.name, users.get(t.creator)));
			}
		} },

		{ "list unread", "List all unread messages", [&](const vector<string> &args) { 
			auto start = board.first_msg();
			auto end = board.last_msg();
			for(auto i = start; i < end; i++) {
				if(!board.is_read(i)) {
					auto msg = board.get_message(i);
					auto topic = board.get_topic(msg.topic);
					auto group = board.get_group(topic.group);
					write(format("#%d %s [%s] (%s)\n", i, topic.name, group.name, users.get(msg.creator)));
				}
			}
		} },

		{ "list all", "List all messages", [&](const vector<string> &args) { 
			auto start = board.first_msg();
			auto end = board.last_msg();
			for(auto i = start; i < end; i++) {
				auto msg = board.get_message(i);
				auto topic = board.get_topic(msg.topic);
				auto group = board.get_group(topic.group);
				write(format("#%d %s [%s] (%s)\n", i, topic.name, group.name, users.get(msg.creator)));
			}
		} },

		{ "next topic", "Go to next unread topic", [&](const vector<string> &args) {

			board.flush_bits();
			LOGD("First unread");
			auto msgid = board.get_first_unread_msg();
			if(msgid < 1) {
				write("No more topics\n");
				return;
			}
			LOGD("unread %d", msgid);
			auto msg = board.get_message(msgid);
			auto topic = board.get_topic(msg.topic);
			auto group = board.get_group(topic.group);
			write("Group '%s' topic '%s'\n", group.name, topic.name);
			board.enter_group(group.id);
			select_topic(topic.id);
			//currentMsgThread = board.get_thread(topic.first_msgid);
			//auto msg_id = topic.first_msgid;
			msgid = find_first_unread(topic.first_msg);
			currentMsg = board.get_message(msgid);
			lastShown.id = 0;
		} },

		{ "read next", "Read next unread message in current topic", [&](const vector<string> &args) {
			if(currentMsg.id == 0)
				write("No more messages\n");
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

		{ "read", "Read specific message", "!m", [&](const vector<string> &args) {
			try {
				uint64_t mid = std::stol(args[0]);
				auto msg = board.get_message(mid);
				show_message(msg);
				lastShown = msg;
				board.mark_read(mid);
			} catch (invalid_argument &e) {
			}
		} },

		{ "create group", "Create a new group", "!s", [&](const vector<string> &args) {
			auto g = board.create_group(args[0]);
			if(g == 0)
				write("Create group failed\n");
			else
				write(format("Group '%s' created\n", args[0]));
		} },

		{ "create user", "Create a new user", "!s!s", [&](const vector<string> &args) {
			if(args.size() != 2) {
				write("Usage: create user [handle] [password]\n");
				return;
			}
			auto id = users.add_user(args[0], args[1]);
			if(id == 0)
				write("Create user failed\n");
			else
				write(format("User '%s' created\n", args[0]));
		} },

		{ "enter group", "Enter a specific group", "!g", [&](const vector<string> &args) {
			auto g = board.enter_group(args[0]);
			if(g == 0)
				write("No such group\n");
			else {
				lastShown.id = 0;
			}
		} },

		{ "post", "Post a new topic", "?s?s", [&](const vector<string> &args) { 
			//LOGD("List groups");
			auto group = board.current_group();
			if(group.id < 1) {
				write("You need to enter a group first");
				return;
			}

			auto topic = args.size() > 0 ? args[0] : console.getLine("TOPIC:");
			auto text = args.size() > 1 ? args[1] : edit();
			board.post(topic, text);
			lastShown.id = 0;
		} },

		{ "reply", "Reply to a message", "?m", [&](const vector<string> &args) { 
			//LOGD("List groups"); 
			auto mid = lastShown.id;
			if(args.size() == 1)
				mid = std::stol(args[0]);
			if(mid > 0) {
				auto text = edit();
				board.reply(mid, text);
			} else
				write("Reply failed\n");
			lastShown.id = 0;	
		} },
	}
{}
/*
	enum Color {
	0	WHITE,
	1	RED,
	2	GREEN,
	3	BLUE,
	4	ORANGE,
	5	BLACK,
	6	BROWN,
	7	PINK,
	8	DARK_GREY,
	9	GREY,
	a	LIGHT_GREEN,
	b	LIGHT_BLUE,
	c	LIGHT_GREY,
	d	PURPLE,
	e	YELLOW,
	f	CYAN,
	};
*/

void ComBoard::show_message(const MessageBoard::Message &msg) {
	auto topic = board.get_topic(msg.topic);
	auto replies = board.get_replies(msg.id);
	string s = "no replies";
	if(replies.size() == 1)
		s = "1 reply";
	else
		s = format("%d replies", replies.size());
	write(format("~0Msg #%d ~f%s%s~c\n%s~0\nby %s. %s\n", msg.id, msg.parent == 0 ? "" : "Re:", topic.name, msg.text, users.get(msg.creator), s));

	//for(const auto &r : replies) {
	//	write("~9Reply #%d by ~8%s~0\n", r.id, users.get(r.creator));
	//}

	//write(currentMsg.text);
}

string ComBoard::edit() {
	auto ed = FullEditor(console);
	while(true) {
		auto rc = ed.update(500);
		if(rc == Console::KEY_F7) {
			break;
		}
	}
	string r = rstrip(ed.getResult(), '\n');
	return r;
}

string ComBoard::suggested_command() {
	if(currentMsg.id == 0) {
		auto msgid = board.get_first_unread_msg();
		if(msgid < 1) {
			return "status";
		}
		return "next topic";
	} else
		return "read next";

};

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

template <class... A> void ComBoard::write(const std::string &text, const A& ... args) {
	string f = format(text, args...);
	auto parts = split(f, "~", true);
	bool first = true;
	for(auto s : parts) {
		if(!first) {
			char c = 0;
			if(s[0] <= '9')
				c = s[0] - '0';
			else
				c = s[0] - 'a' + 10;
			console.setColor(c);
			s = s.substr(1);
		}
		first = false;
		console.write(s);
	}
}

bool ComBoard::exec(const string &line) {
	auto parts = split(line);
	if(parts.size() < 1)
		return true;

	vector<Command*> foundCommands;
	int largestMatch = 0;
	for(auto &c : commands) {
		bool found = true;
		if(parts.size() < c.text.size()) {
			continue;
		}
		for(int i=0; i<c.text.size(); i++) {
			if(c.text[i].compare(0, parts[i].length(), parts[i]) != 0) {
				found = false;
				break;
			}
		}

		if(found) {
			if(c.text.size() >= largestMatch) {
				if(c.text.size() > largestMatch) {
					LOGD("clearing previous, longer");
					largestMatch = c.text.size();
					foundCommands.clear();
				}
				LOGD("Adding %s", c.full_text);
				foundCommands.push_back(&c);
			}
		}
	}

	if(foundCommands.size() == 1) {
		auto &c = *(foundCommands[0]);
		LOGD("Matched '%s'", c.full_text);
		//int acount = parts.size() - c.text.size();
		parts.erase(parts.begin(), parts.begin() + c.text.size());
		bool ok = true;
		int i = 0;
		LOGD("We have %d args", parts.size());
		for(auto &a : c.args) {
			LOGD("%d %d", (int)a.argtype, i);
			if(i == parts.size()) {
				write("Too few arguments\n");
				ok = false;
				break;
			}
			if(a.argtype == Command::MESSAGE) {
				try {
					auto mid = std::stol(parts[i]);
					auto msg = board.get_message(mid);
				} catch (msgboard_exception &e) {
					write("No such message\n");
					ok = false;
				} catch (invalid_argument &e) {
					write("You need to provide a message number\n");
					ok = false;
				}
			} else if(a.argtype == Command::GROUP) {
				try {
					auto group = board.get_group(parts[i]);
				} catch (msgboard_exception &e) {
					write("No such group\n");
					ok = false;
				}
			}
			if(!ok) break;
			i++;
		}
		if(i > parts.size()) {
			write("Too many arguments\n");
			ok = false;
		}
		if(ok) {
			c.func(parts);
			if(c.full_text == "logout" || c.full_text == "quit")
				return false;
		}
	} else if(foundCommands.size() > 1) {
		write("Ambigous command:\n");
		for(auto *c : foundCommands) {
			write(c->full_text + "\n");
		}
	} else {
		console.write("Unknown command\n");
	}
	return true;
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