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




/*
next group
read next
comment/reply
post
*/


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

	//uint64_t x = 0x1201f;
	//LOGD("%x %d", ~x, __builtin_ctzl(~x));
	//return 0;

	sqlite3db::Database db { "bbs.db" };

	//db.exec("CREATE TABLE IF NOT EXISTS bbsuser (sha TEXT, handle TEXT)");

	//db.exec("CREATE TABLE IF NOT EXISTS groupstate (userid INT, groupid INT, lastread INT, FOREIGN KEY(userid) REFERENCES bbsuser(ROWID), FOREIGN KEY(groupid) REFERENCES msggroup(ROWID))");
	//db.exec("CREATE TABLE IF NOT EXISTS topicstate (userid INT, topicid INT, lastread INT, FOREIGN KEY(userid) REFERENCES bbsuser(ROWID), FOREIGN KEY(topicid) REFERENCES msgtopic(ROWID))");
	//db.exec("CREATE TABLE IF NOT EXISTS readmsg (userid INT, msgid INT, topicid INT, FOREIGN KEY(userid) REFERENCES bbsuser(ROWID), FOREIGN KEY(msgid) REFERENCES message(ROWID), FOREIGN KEY(topicid) REFERENCES msgtopic(ROWID))");

	LoginManager loginManager(db);

	if(loginManager.get_id("admin") == LoginManager::NO_USER) {
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

			while(true) {
				auto sc = comboard.suggested_command();
				comboard.write("\n~0(~8%s~0) # ", sc);
				auto line = console.getLine();
				if(line == "")
					line = sc;
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
