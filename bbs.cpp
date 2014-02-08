#include <coreutils/log.h>
#include <coreutils/utils.h>
#include <coreutils/file.h>
#include <bbsutils/telnetserver.h>
#include <bbsutils/console.h>
#include <bbsutils/editor.h>

#include <bbsutils/ansiconsole.h>
#include <bbsutils/petsciiconsole.h>

#include <crypto/sha256.h>
#include <sqlite3/database.h>
#include <time.h>
using namespace bbs;
using namespace utils;
using namespace std;


sqlite3db::Database db { "bbs.db" };

class LoginManager {
public:
	struct User {
		User(uint64_t id = 0, const std::string &handle = "") : id(id), handle(handle) {}
		const uint64_t id;
		const std::string handle;
	};

	static const uint64_t NO_USER = 0;

	LoginManager() {}

	uint64_t verify_user(const std::string &handle, const std::string &password) {
		auto s = sha256(handle + "\t" + password);
		uint64_t id = NO_USER;
		db.execf("SELECT ROWID FROM bbsuser WHERE sha=%Q;", [&](int i, const vector<string> &result) {
			id = std::stol(result[0]);
		}, s);
		return id;
	}

	uint64_t get_user(const std::string &handle) {
		uint64_t id = NO_USER;
		db.execf("SELECT ROWID FROM bbsuser WHERE handle=%Q;", [&](int i, const vector<string> &result) {
			id = std::stol(result[0]);
		}, handle);
		return id;
	}

	uint64_t add_user(const std::string &handle, const std::string &password) {
		auto sha = sha256(handle + "\t" + password);
		db.exec("INSERT INTO bbsuser (sha, handle) VALUES (%Q, %Q)", sha, handle);
		return db.last_rowid();
	}
};


class MessageBoard {
public:

	MessageBoard(sqlite3db::Database &db) : db(db) {}

	struct Group {
		Group(uint64_t id, const std::string &n, uint64_t lp = 0,  std::string c = "") : id(id), name(n), last_post(lp), creator(c) {}
		const uint64_t id;
		const std::string name;
		const uint64_t last_post;
		const  std::string creator;
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

	void login(uint64_t userid) {
		currentUser = userid;
	}

	uint64_t create_group(const std::string &name) {
		db.exec("INSERT INTO msggroup (name, creatorid, lastpost) VALUES (%Q, %d, 0)", name, currentUser);
		return db.last_rowid();
	}
	const std::vector<Group> list_groups() {
		vector<Group> groups;
		db.execf("SELECT msggroup.ROWID, msggroup.name, msggroup.lastpost, bbsuser.handle FROM msggroup,bbsuser WHERE msggroup.creatorid=bbsuser.ROWID", [&](int i, const vector<string> &result) {
			groups.emplace_back(std::stol(result[0]), result[1], std::stol(result[2]), result[3]);
		});
		return groups; // NOTE: std::move ?
	}

	uint64_t enter_group(const std::string &group_name) {
		uint64_t id = 0;
		uint64_t lastpost = 0;
		db.execf("SELECT ROWID,lastpost FROM msggroup WHERE name=%Q", [&](int i, const vector<string> &result) {
			id = std::stol(result[0]);
			lastpost = std::stol(result[1]);
		}, group_name);
		//auto id = db.select_first<pair<uint64_t, uint64_t>>("SELECT ROWID,lastpost FROM msggroup WHERE name=%Q", group_name);
		LOGD("Last post %d", lastpost);
		if(id != 0) {
			currentGroup = id;
			uint64_t lastread;
			bool unread = true;
			if(db.select_first(lastread, "SELECT lastread FROM groupstate WHERE userid=%d", currentUser)) {
				LOGD("Last read %d", lastread);
			}
		}
		return id;
	}

	const std::vector<Topic> list_topics() {
		vector<Topic> topics;
		LOGD("currentGroup %d", currentGroup);
		db.execf("SELECT msgtopic.ROWID,name,handle,lastpost,lastby FROM msgtopic,bbsuser WHERE groupid=%d AND creatorid=bbsuser.ROWID", [&](int i, const vector<string> &result) {
			LOGD(result[1]);
			topics.emplace_back(std::stol(result[0]), result[1], result[2]);
		}, currentGroup);
		return topics; // NOTE: std::move ?
	}

	const std::vector<Message> list_messages(uint64_t topic_id) {
		vector<Message> messages;
		db.execf("SELECT ROWID,contents,creatorid FROM msgtopic WHERE topicid=%d", [&](int i, const vector<string> &result) {
			LOGD(result[1]);
			messages.emplace_back(std::stol(result[0]), result[1]);
		}, topic_id);
		return messages; // NOTE: std::move ?		
	}

	uint64_t post(const std::string &topic_name, const std::string &text) {
		auto ta = db.transaction();
		db.exec("INSERT INTO msgtopic (name, creatorid,groupid) VALUES (%Q, %d, %d)", topic_name, currentUser, currentGroup);
		auto topicid = db.last_rowid();
		db.exec("INSERT INTO message (contents, creatorid, parentid, topicid) VALUES (%Q, %d, 0, %d)", text, currentUser, topicid);
		auto msgid = db.last_rowid();

		time_t t;
		time(&t);
		//long d = 12345;
		db.exec("UPDATE msggroup SET lastpost=%d,lastby=%d WHERE ROWID=%d", t, currentUser,currentGroup);
		db.exec("UPDATE msgtopic SET lastpost=%d,lastby=%d WHERE ROWID=%d", t, currentUser, topicid);
		db.exec("INSERT INTO readmsg (userid, msgid) VALUES (%d,%d)", currentUser, msgid);
		//db.exec("INSERT INTO groupstate (userid, groupid) VALUES (%d, %d)", currentUser, currentGroup);
		//db.exec("INSERT INTO topicstate (userid, topicid) VALUES (%d, %d)", currentUser, currentGroup);
		ta.commit();
		return msgid;
	}
	uint64_t reply(uint64_t msgid, const std::string &text) {
		uint64_t topicid;
		db.execf("SELECT topicid FROM message WHERE ROWID=%d", [&](int i, const vector<string> &result) {
			topicid = std::stol(result[0]);
		}, currentGroup);
		db.exec("INSERT INTO message (contents, userid, parentid, topicid) VALUES (%Q, %d, ^d, %d)", text, currentUser, msgid, topicid);
		return db.last_rowid();		
	}
private:
	uint64_t currentUser;
	uint64_t currentGroup;
	sqlite3db::Database &db;
};

struct Command {
	typedef std::function<void(const std::vector<std::string> &args)> Function;
	Command(const std::string &text, Function f) : func {f}, text {split(text)} {}
	Function func;
	const std::vector<std::string> text;
};

LoginManager loginManager;

class ComBoard {
public:
	ComBoard(MessageBoard &board, Console &console) : board(board), console(console), 
		commands { 			
			{ "list groups", [&](const std::vector<std::string> &args) { 
				LOGD("List groups");
				for(const auto &g : board.list_groups()) {
					time_t tt = (time_t)g.last_post;
					const char *t = ctime(&tt);
					console.write(format("%s [Created by %d] LAST POST: %s\n", g.name, g.creator, t));
				}
			} },
			{ "list topics", [&](const std::vector<std::string> &args) { 
				LOGD("List topics");
				for(const auto &t : board.list_topics()) {
					console.write(format("%s (%s)\n", t.name, t.starter));
				}
			} },
			{ "next group", [&](const std::vector<std::string> &args) { 
				for(const auto &g : board.list_groups()) {
					time_t tt = (time_t)g.last_post;
					const char *t = ctime(&tt);
					console.write(format("%s [Created by %d] LAST POST: %s\n", g.name, g.creator, t));
				}

			} },
			{ "create group", [&](const std::vector<std::string> &args) {
				auto g = board.create_group(args[2]);
				if(g == 0)
					console.write("Create group failed\n");
				else
					console.write(format("Group '%s' created\n"));
			} },
			{ "enter group", [&](const std::vector<std::string> &args) {
				auto g = board.enter_group(args[2]);
				if(g == 0)
					console.write("No such group\n");
			} },
			{ "post", [&](const std::vector<std::string> &args) { 
				//LOGD("List groups"); 
				auto topic = console.getLine("TOPIC:");
				auto ed = FullEditor(console);
				while(true) {
					auto rc = ed.update(500);
					if(rc == Console::KEY_F7) {
						break;
					}
				}
				auto text = ed.getResult();
				board.post(topic, text);
			} },
		}
	{}

	void exec(const std::string &line) {
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
private:
	MessageBoard &board;
	Console &console;
	std::vector<Command> commands;
};


/*
next group
read next
comment/reply
post

*/

int main(int argc, char **argv) {

	db.exec("CREATE TABLE IF NOT EXISTS bbsuser (sha TEXT, handle TEXT)");
	db.exec("CREATE TABLE IF NOT EXISTS msggroup (name TEXT, creatorid INT, lastpost INT, lastby INT, FOREIGN KEY(creatorid) REFERENCES bbsuser(ROWID), FOREIGN KEY(lastby) REFERENCES bbsuser(ROWID))");
	db.exec("CREATE TABLE IF NOT EXISTS msgtopic (name TEXT, creatorid INT, groupid INT, lastpost INT, lastby INT, FOREIGN KEY(creatorid) REFERENCES bbsuser(ROWID), FOREIGN KEY(groupid) REFERENCES msggroup(ROWID), FOREIGN KEY(lastby) REFERENCES bbsuser(ROWID))");
	db.exec("CREATE TABLE IF NOT EXISTS message (contents TEXT, creatorid INT, parentid INT, topicid INT, timestamp INT, FOREIGN KEY(creatorid) REFERENCES bbsuser(ROWID), FOREIGN KEY(topicid) REFERENCES msgtopic(ROWID))");

	db.exec("CREATE TABLE IF NOT EXISTS groupstate (userid INT, groupid INT, lastread INT, FOREIGN KEY(userid) REFERENCES bbsuser(ROWID), FOREIGN KEY(groupid) REFERENCES msggroup(ROWID))");
	db.exec("CREATE TABLE IF NOT EXISTS topicstate (userid INT, topicid INT, lastread INT, FOREIGN KEY(userid) REFERENCES bbsuser(ROWID), FOREIGN KEY(topicid) REFERENCES msgtopic(ROWID))");

	db.exec("CREATE TABLE IF NOT EXISTS readmsg (userid INT, msgid INT, topicid INT, FOREIGN KEY(userid) REFERENCES bbsuser(ROWID), FOREIGN KEY(msgid) REFERENCES message(ROWID), FOREIGN KEY(topicid) REFERENCES msgtopic(ROWID))");


	if(loginManager.get_user("admin") == LoginManager::NO_USER) {
		loginManager.add_user("admin", "password");

		//board.login(1);
		//board.create_group("misc");
		//board.enter_group("misc");
		//board.post("First thread", "This is the first text in the first post of the first thread.... PHEW");
	}

	setvbuf(stdout, NULL, _IONBF, 0);
	//logging::setLevel(logging::INFO);

	// Turn off logging from the utility classes
	//logging::useLogSpace("utils", false);

	//loginManager.add_user("sasq", "password");
	//loginManager.add_user("jonas", "secret");
	//db.get_user("sasq");

	TelnetServer telnet { 12345 };
	telnet.setOnConnect([&](TelnetServer::Session &session) {
		try {
			session.echo(false);
			auto termType = session.getTermType();

			unique_ptr<Console> con;
			if(termType.length() > 0) {
				con = make_unique<AnsiConsole>(session);
			} else {
				con = make_unique<PetsciiConsole>(session);
			}
			Console &console = *con;

			MessageBoard board(db);
			board.login(1);
			ComBoard comboard(board, console);

			auto h = session.getHeight();
			auto w = session.getWidth();
			LOGD("New connection, TERMTYPE '%s' SIZE %dx%d", termType, w, h);
			console.flush();
			console.clear();
			int tries = 2;
			while(true) {
				break;
				auto l = console.getLine("LOGON:");
				auto p = console.getLine("PASSWORD:");
				auto id = loginManager.verify_user(l, p);
				if(id != LoginManager::NO_USER)
					break;
				if(tries == 0)
					session.disconnect();
				else
					tries--;
			}
			console.clear();
			console.moveCursor(0,0);

			while(true) {
				console.write(">");
				auto line = console.getLine();
				console.write("\n");
				comboard.exec(line);
			}

			/*
			console.write("Welcome...");
			Area a (0,0,10,10);
			vector<string> s = { "First item", "Second item", "Last item" };
			ListView<string>::RenderFunction rf = ListView<string>::simpleRenderFunction;// ListView<string>::simpleRenderFunction();
			//[&](Console &c, Area &a, int item, const string &data) {
			//	c.put(a.x0, a.y0, data);
			//	a.y0++;
			//};
			ListView<string>::SourceFunction sf = ListView<string>::makeVectorSource(s);

			ListView<string> lv {console, {1,1,10,10}, rf, sf, 3};
			while(true) {
				lv.update();
			} */
		
		} catch (TelnetServer::disconnect_excpetion &e) {
			LOGD("Client disconnected");
			//addChatLine(user.name() + " disconnected");
			//{ lock_guard<mutex> guard(chatLock);
			//	users.erase(user);
			//}
		}
	});
	telnet.run();

	return 0;
}
