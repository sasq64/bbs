#include <coreutils/log.h>
#include <coreutils/utils.h>
#include <coreutils/file.h>
#include <coreutils/bitfield.h>
#include <bbsutils/telnetserver.h>
#include <bbsutils/console.h>
#include <bbsutils/editor.h>

#include <bbsutils/ansiconsole.h>
#include <bbsutils/petsciiconsole.h>

#include <crypto/sha256.h>
#include <sqlite3/database.h>
#include <time.h>

#include "loginmanager.h"
#include "messageboard.h"
#include "comboard.h"

using namespace bbs;
using namespace utils;
using namespace std;

#define BACKWARD_HAS_BFD 1
#include "backward-cpp/backward.hpp"
namespace backward {
backward::SignalHandling sh;
} // namespace backward

#ifdef UNIT_TEST

#include "catch.hpp"

TEST_CASE("utils::format", "format operations") {

	using namespace utils;
	using namespace std;


	BitField bf(1234);
	bf.set(5, true);
	bf.set(7, true);
	bf.set(1015, true);
	bf.set(1016, true);
	bf.set(1017, true);
	LOGD("%d %d %d %d %d %d", (int)bf.get(5), (int)bf.get(6), (int)bf.get(7), (int)bf.get(1015), (int)bf.get(1016), (int)bf.get(1018));
	//REQUIRE(res == "14 014 test 20 case");
}

#endif

int main(int argc, char **argv) {

	sqlite3db::Database db { "bbs.db" };

	auto q = db.query<int, string>("SELECT ROWID,name FROM msggroup WHERE ROWID=?", 1);
	auto group = q.get<MessageBoard::Group>();
/*
	auto q = db.query("SELECT name,creatorid FROM msggroup WHERE ROWID=?", 1);
	auto y = q.step<string,int>();
	LOGD("RESULT %s %d", get<0>(y), get<1>(y));
	q = db.query("SELECT creatorid FROM msggroup WHERE ROWID=?", 2);
	auto x = q.step<int>();
	LOGD("RESULT %d", x);
	return 0;
*/
	LoginManager loginManager(db);

	if(loginManager.get_id("admin") == LoginManager::NO_USER) {
		loginManager.add_user("admin", "password");
	}

	setvbuf(stdout, NULL, _IONBF, 0);
	//logging::setLevel(logging::INFO);
	// Turn off logging from the utility classes
	//logging::useLogSpace("utils", false);

	TelnetServer telnet { 12345 };
	telnet.setOnConnect([&](TelnetServer::Session &session) {
		uint64_t userId = 0;
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

			//throw sqlite3db::db_exception("testing");

			auto h = session.getHeight();
			auto w = session.getWidth();
			LOGD("New connection, TERMTYPE '%s' SIZE %dx%d", termType, w, h);
			console.flush();
			console.clear();
			int tries = 2;
			while(true) {
				//break;
				auto l = console.getLine("LOGON:");
				auto p = console.getLine("PASSWORD:");
				userId = loginManager.login_user(l, p);
				if(userId != LoginManager::NO_USER)
					break;
				if(tries == 0)
					session.disconnect();
				else
					tries--;
			}

			MessageBoard board(db, userId);
			ComBoard comboard(loginManager, board, console);

			console.clear();
			console.moveCursor(0,0);

			//std::tuple<int, std::string> result;
			//db.exec2(result, "q", a0, a1, "hello");
			vector<string> history;
			auto history_pos = history.begin();

			while(true) {
				auto sc = comboard.suggested_command();
				comboard.write("\n~0(~8%s~0) # ", sc);
				LineEditor lineEd(console);
				int key = 0;
				string savedLine = "";
				while(key != Console::KEY_ENTER) {
					key = lineEd.update(500);
					switch(key) {
					case Console::KEY_UP:
						if(history_pos > history.begin()) {
							history_pos--;
							lineEd.setString(*history_pos);
							lineEd.setCursor(99999);
							lineEd.refresh();
						}
						break;
					case Console::KEY_DOWN:
						if(history_pos < history.end()) {
							history_pos++;
							if(history_pos == history.end())
								lineEd.setString("");
							else
								lineEd.setString(*history_pos);
							lineEd.setCursor(99999);
							lineEd.refresh();
						}
						break;
					}
				}

				auto line = lineEd.getResult();

				if(line == "")
					line = sc;
				else {
					history.push_back(line);
					history_pos = history.end();
				}
				console.write("\n");
				if(!comboard.exec(line)) {
					loginManager.logout_user(userId);
					session.disconnect();
				}
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
			loginManager.logout_user(userId);
			//addChatLine(user.name() + " disconnected");
			//{ lock_guard<mutex> guard(chatLock);
			//	users.erase(user);
			//}
		}
	});
	telnet.run();

	return 0;
}
