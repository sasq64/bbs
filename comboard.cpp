#include "bbs.h"
#include "comboard.h"

#include <bbsutils/ansiconsole.h>
#include <bbsutils/editor.h>

using namespace bbs;
using namespace utils;
using namespace std;
using std::chrono::system_clock;

ComBoard::ComBoard(BBS::Session &session, MessageBoard &board, Console &console) : session(session), bbs(BBS::instance()), board(board), console(console), 
	commands { 			
		{ "list groups", "List all groups", [&](const vector<string> &args) { 
			LOGD("List groups");
			write("\n");
			for(const auto &g : board.list_groups()) {
				//time_t tt = (time_t)g.last_post;
				//const char *t = ctime(&tt);
				write("~1%s~0 created by ~f%s~0\n", g.name, bbs.get_user(g.creator));
			}
		} },

		{ "edit plan", "Edit your plan (visible to all)", [&](const vector<string> &args) { 
			auto plan = BBS::instance().get_user_data(board.current_user(), "plan");
			fulledit(plan, false);
			BBS::instance().set_user_data(board.current_user(), "plan", plan);
		} },

		{ "help", "Read help files", "?s", [&](const vector<string> &args) { 
			write("\n");
			if(args.size() == 0) {
				show_text("help");
			} else {
				if(!show_text(string("help_") + args[0]))
					write("No such help topic");
			}
		} } ,
		{ "finger", "Read a users plan", "!u", [&](const vector<string> &args) { 
			auto uid = bbs.get_user_id(args[0]);
			auto plan = bbs.get_user_data(uid, "plan");
			write(plan);
		} },

		{ "logout", "Log out from bbs", [&](const vector<string> &args) { 
			board.flush_bits();
		} },

		{ "list commands", "List all commands", [&](const vector<string> &args) { 
			write("\n");
			for(auto &c : commands) {
				if(c.umask == 1 && board.current_user() != 1)
					continue;
				write("~1%s ~c- %s~0\n", c.full_text, c.description);
			}
		} },


		{ "status", "Show some status information", [&](const vector<string> &args) { 
			auto start = board.first_msg();
			auto end = board.last_msg();
			int unread_count = 0;
			for(auto i = start; i < end; i++) {
				if(!board.is_read(i))
					unread_count++;
			}

			auto tp = system_clock::now();
			auto tt = system_clock::to_time_t(tp);
		 	auto timeinfo = localtime(&tt);
 			char buffer[128];
  			strftime(buffer,sizeof(buffer),"%d %b %H:%M ",timeinfo);

  			auto group_name = board.current_group().name;
  			if(group_name == "")
  				group_name = "None";

  			write("\n%d unread messages\nCurrent group: %s\nCurrent time: %s\n", unread_count, group_name, buffer);

		} },

		{ "list users", "list all users", [&](const vector<string> &args) { 
			auto ulist = bbs.list_users();
			write("\n");
			for(auto &u : ulist)
				write("%s\n", u);
		} },

		{ "who", "Show who is online", [&](const vector<string> &args) { 
			auto ulist = bbs.list_logged_in();
			write("\n");
			for(auto &u : ulist)
				write("%s\n", u);
		} },

		{ "list topics", "List all topics in the current group", [&](const vector<string> &args) { 
			LOGD("List topics");
			auto group = board.current_group();
			write("\nTopics in group ~f%s~0\n", group.name);
			for(const auto &t : board.list_topics(group.id)) {
				write("~1%s~0 %d/%d%s\n", t.name, t.unread_count, t.msg_count, t.byme_count > 0 ? "*" : "");
			}
		} },

		{ "list unread", "List all unread messages", [&](const vector<string> &args) { 
			auto start = board.first_msg();
			auto end = board.last_msg();
			write("\n");
			for(auto i = start; i < end; i++) {
				if(!board.is_read(i)) {
					auto msg = board.get_message(i);
					auto topic = board.get_topic(msg.topic);
					auto group = board.get_group(topic.group);
					write("#%d %s [%s] (%s)\n", i, topic.name, group.name, bbs.get_user(msg.creator));
				}
			}
		} },

		{ "list messages", "List all messages", [&](const vector<string> &args) { 
			auto start = board.first_msg();
			auto end = board.last_msg();
			write("\n");
			for(auto i = start; i < end; i++) {
				auto msg = board.get_message(i);
				auto topic = board.get_topic(msg.topic);
				auto group = board.get_group(topic.group);
				write("#%d %s [%s] (%s)\n", i, topic.name, group.name, bbs.get_user(msg.creator));
			}
		} },

		{ "next group", "Go to next unread topic", [&](const vector<string> &args) {
			board.flush_bits();
			auto group = board.next_unread_group();
			if(group.id == 0) {
				write("\nNo more unread messages");
			} else
				board.enter_group(group.id);
		} },

		{ "next topic", "Go to next unread topic", [&](const vector<string> &args) {

			board.flush_bits();
			/*LOGD("First unread");
			auto msgid = board.get_first_unread_msg();
			if(msgid < 1) {
				write("No more topics\n");
				return;
			}
			LOGD("unread %d", msgid);
			auto msg = board.get_message(msgid);
			auto topic = board.get_topic(msg.topic);
			auto group = board.get_group(topic.group);*/

			auto group = board.current_group();
			auto topic = board.next_unread_topic(group.id);
			if(topic.id > 0) {
				write("\nGroup:~f%s~0 Topic:~f%s~0\n", group.name, topic.name);
				board.enter_group(group.id);
				select_topic(topic.id);
				//currentMsgThread = board.get_thread(topic.first_msgid);
				//auto msg_id = topic.first_msgid;
				auto msgid = find_first_unread(topic.first_msg);
				LOGD("First unread in topic %d is %d", topic.id, msgid);
				currentMsg = board.get_message(msgid);
				lastShown.id = 0;
			} else
				write("\nNo more unread topics in current group");
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
				write("Group '%s' created\n", args[0]);
		}, 1 },

		{ "create user", "Create a new user", "!s!s", [&](const vector<string> &args) {
			if(args.size() != 2) {
				write("Usage: create user [handle] [password]\n");
				return;
			}
			auto id = bbs.create_user(args[0], args[1]);
			if(id == 0)
				write("Create user failed\n");
			else
				write("User '%s' created\n", args[0]);
		}, 1 },

		{ "change password", "Change your password", "?u", [&](const vector<string> &args) {
			auto user = bbs.get_user(board.current_user());
			if(args.size() == 1) {
				user = args[0];
			}
			if(bbs.get_user_id(user) == 0) {
					write("\nNo such user\n");
					return;
			}
			if(board.current_user() != 1) {
				auto op = console.getPassword("Old password:");
				if(!bbs.verify_user(user, op)) {
					write("\nThat's not your password\n");
					return;
				}
			}
			auto np = console.getPassword("New password:");
			auto np2 = console.getPassword("New (again):");
			if(np != np2) {
				write("\nPasswords doesn't match\n");
				return;
			}
			if(bbs.change_password(user, np)) {
			}
		}, },

		{ "go", "Enter a specific group", "!g", [&](const vector<string> &args) {
			auto g = board.enter_group(args[0]);
			if(g.id == 0)
				write("No such group\n");
			else {
				write("\nEntered group %s\n", g.name);
				lastShown.id = 0;
			}
		} },

		{ "join group", "Join a specific group", "!g", [&](const vector<string> &args) {
			auto g = board.join_group(args[0]);
			if(g.id == 0)
				write("No such group\n");
			else {
				write("Joined group %s\n", g.name);
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
			if(topic != "") {
				auto text = args.size() > 1 ? args[1] : edit();
				if(text != "") {
					auto mid = board.post(topic, text);
					lastShown.id = 0;
					write("\nMessage #%d posted\n", mid);
				}
			}
		} },

		{ "comment", "Reply to a message", "?m", [&](const vector<string> &args) { 
			//LOGD("List groups"); 
			auto mid = lastShown.id;
			if(args.size() == 1)
				mid = std::stol(args[0]);
			if(mid > 0) {
				auto msg = board.get_message(mid);
				auto text = edit();
				auto rid = board.reply(mid, text);
				write("\nMessage #%d posted\n", rid);
			} else
				write("Reply failed\n");
			lastShown.id = 0;	
		} },
	},
	variables ( session.vars() ),
	texts ( BBS::instance().text() )
{
	variables["you"] = bbs.get_user(board.current_user());
	settings["fulledit"] = 1;
}
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

// Msg #%[msgno] ~1%[topic] ~0on %[date]

string bbs_format(std::string templ, std::unordered_map<std::string, std::string> &vars) {

	size_t tstart = 0;
	while(tstart != string::npos) {
		tstart = templ.find("%[", tstart);
		if(tstart != string::npos) {
			auto tend = templ.find("]", tstart+2);
			auto var = templ.substr(tstart+2, tend-tstart-2);
			LOGD("Found var %s at %d,%d", var, tstart, tend);
			if(vars.count(var) > 0)
				templ.replace(tstart, tend-tstart+1, vars[var]);
			tstart++;
		}
	}
	return templ;
}

void ComBoard::show_message(const MessageBoard::Message &msg) {
	auto topic = board.get_topic(msg.topic);
	auto replies = board.get_replies(msg.id);
	string s = "no replies";
	if(replies.size() == 1)
		s = "1 reply";
	else
		s = format("%d replies", replies.size());

	//auto tp = system_clock::from_time_t(msg.time_stamp);
	time_t tt = msg.timestamp;

 	auto timeinfo = localtime(&tt);
 	char buffer[128];
  	strftime(buffer,sizeof(buffer),"%d %b %H:%M ",timeinfo);

	//string timeString = ctime(&tt);

	auto lines = text_wrap(msg.text, console.getWidth());

	//unordered_map<string, string> vars;
	variables["msgno"] = to_string(msg.id);
	variables["topic"] = topic.name;
	variables["date"] = buffer;
	variables["poster"] = bbs.get_user(msg.creator);
	variables["replies"] = s;

	auto header = bbs_format(texts["msg_header"], variables);
	write(header);
	write("\n");
	//write("\n~0Msg #%d ~f%s%s ~0on %s~c\n", msg.id, msg.parent == 0 ? "" : "Re:", topic.name, buffer);
	for(auto &l : lines)
		write(l + "\n");
	auto footer = bbs_format(texts["msg_footer"], variables);
	write(footer);
	write("\n");
	//write("~0by %s. %s\n", bbs.get_user(msg.creator), s);

	//for(const auto &r : replies) {
	//	write("~9Reply #%d by ~8%s~0\n", r.id, bbs.get_user(r.creator));
	//}

	//write(currentMsg.text);
}

bool ComBoard::show_text(const std::string &what) {

	File f;
	if(console.name() == "ansi") {		
		f = File("text/" + what + ".ans");
	} else if (console.name() == "ansi") {
		f = File("text/" + what + ".pet");
	}
	if(!f.exists())
		f = File("text/" + what + ".txt");

	LOGD("FILE %s", f.getName());

	if(!f.exists())
		return false;

	auto text = f.read();
	LOGD("%s", text);
	write(text);
	return true;
}


string ComBoard::edit() {

	int lineno = 1;
	vector<string> lines;
	bool save = true;
	bool done = false;
	int jumpTo = -1;
	write("\n~fF7~0 = Post, ~fF5~0 = Cancel\n");
	while(!done) {
		write("~8%02d:~0", jumpTo >= 0 ? jumpTo : lineno);
		LineEditor ed(console);
		if(jumpTo >= 0)
			ed.setString(lines[jumpTo-1]);
		int key = 0;
		while(key != Console::KEY_ENTER && !done) {
			key = ed.update(500);
			switch(key) {
			case Console::KEY_F7:
				done = true;
				break;
			case Console::KEY_F5:
				save = false;
				done = true;
				break;
			}
		}
		auto line = ed.getResult();
		if(line[0] == '.') {
			switch(line[1]) {
			case 's':
				done = true;
				break;
			case 'q':
				save = false;
				done = true;
			case 'e':
				console.moveCursor(0, console.getCursorY());
				console.fillLine(console.getCursorY());
				jumpTo = stol(line.substr(2));
				break;
			}
		} else {
			if(jumpTo >= 0) {
				lines[jumpTo-1] = line;
				jumpTo = -1;
			} else {
				lineno++;
				lines.push_back(line);
			}
			auto wrapped = text_wrap(line, console.getWidth()-3);
			for(auto &w : wrapped) {
				console.moveCursor(3, console.getCursorY());
				console.write(w + "\n");
			}
		}
	}
	if(!save)
		return "";
	return join(lines, "\n");
}

string ComBoard::fulledit(const string &text, bool comment) {

	auto ed = FullEditor(console);
	if(comment)
		ed.setComment(text);
	else
		ed.setString(text);
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

		auto t = board.next_unread_topic(board.current_group().id);
		if(t.id != 0)
			return "next topic";
		auto g = board.next_unread_group();
		if(g.id != 0)
			return "next group";

		return "WTF!";

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

static int palette[] = { Console::WHITE, Console::CYAN, Console::GREY, Console::LIGHT_GREY, Console::RED };

template <class... A> void ComBoard::write(const std::string &text, const A& ... args) {
	string f = format(text, args...);
	auto parts = split(f, "~", true);
	bool first = true;
	for(auto s : parts) {
		if(!first) {
			uint8_t c = 0;
			if(s[0] <= '9')
				c = s[0] - '0';
			else
				c = s[0] - 'a' + 10;
			if(c >= 5) c = 0;
			console.setColor(palette[c]);
			s = s.substr(1);
		}
		first = false;
		console.write(s);
	}
}

bool ComBoard::exec(const string &line) {

	board.update_bits();

	auto parts = split(line);
	if(parts.size() < 1)
		return true;

	vector<Command*> foundCommands;
	size_t largestMatch = 0;
	for(auto &c : commands) {

		if(c.umask == 1 && board.current_user() != 1)
			continue;

		bool found = true;
		if(parts.size() < c.text.size()) {
			continue;
		}
		for(size_t i=0; i<c.text.size(); i++) {
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
		size_t i = 0;
		LOGD("We have %d args", parts.size());
		for(auto &a : c.args) {
			LOGD("%d %d", (int)a.argtype, i);
			if(i == parts.size()) {
				if(a.optional)
					continue;
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
					ok = false;
					for(auto &g : board.list_groups()) {
						if(g.name.compare(0, parts[i].length(), parts[i]) == 0) {
							parts[i] = g.name;
							ok = true;
							break;
						}
					}
					if(!ok)
						write("No such group\n");
				}
			} else if(a.argtype == Command::USER) {
				try {
					bbs.get_user_id(parts[i]);
				} catch (msgboard_exception &e) {
					ok = false;
					for(auto &s : bbs.list_users()) {
						if(s.compare(0, parts[i].length(), parts[i]) == 0) {
							parts[i] = s;
							ok = true;
							break;
						}
					}
					if(!ok)
						write("No such user\n");
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

	virtual void open() override {}
	virtual void close() override {}

	virtual int write(const std::vector<Char> &source, int len) { 
		return fwrite(&source[0], 1, len, stdout);
	}
	virtual int read(std::vector<Char> &target, int len) { 
		return fread(&target[0], 1, len, stdin);
	}
private:

};


#include "catch.hpp"
/*
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
*/
#endif