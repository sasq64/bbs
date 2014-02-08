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

int main(int argc, char **argv) {

	sqlite3db::Database db { "bbs.db" };

	db.exec("CREATE TABLE IF NOT EXISTS bbsuser (sha TEXT, handle TEXT)");
	db.exec("CREATE TABLE IF NOT EXISTS msggroup (name TEXT, creatorid INT, lastpost INT, lastby INT, FOREIGN KEY(creatorid) REFERENCES bbsuser(ROWID), FOREIGN KEY(lastby) REFERENCES bbsuser(ROWID))");
	db.exec("CREATE TABLE IF NOT EXISTS msgtopic (name TEXT, creatorid INT, groupid INT, lastpost INT, lastby INT, FOREIGN KEY(creatorid) REFERENCES bbsuser(ROWID), FOREIGN KEY(groupid) REFERENCES msggroup(ROWID), FOREIGN KEY(lastby) REFERENCES bbsuser(ROWID))");
	db.exec("CREATE TABLE IF NOT EXISTS message (contents TEXT, creatorid INT, parentid INT, topicid INT, timestamp INT, FOREIGN KEY(creatorid) REFERENCES bbsuser(ROWID), FOREIGN KEY(topicid) REFERENCES msgtopic(ROWID))");

	db.exec("CREATE TABLE IF NOT EXISTS groupstate (userid INT, groupid INT, lastread INT, FOREIGN KEY(userid) REFERENCES bbsuser(ROWID), FOREIGN KEY(groupid) REFERENCES msggroup(ROWID))");
	db.exec("CREATE TABLE IF NOT EXISTS topicstate (userid INT, topicid INT, lastread INT, FOREIGN KEY(userid) REFERENCES bbsuser(ROWID), FOREIGN KEY(topicid) REFERENCES msgtopic(ROWID))");

	db.exec("CREATE TABLE IF NOT EXISTS readmsg (userid INT, msgid INT, topicid INT, FOREIGN KEY(userid) REFERENCES bbsuser(ROWID), FOREIGN KEY(msgid) REFERENCES message(ROWID), FOREIGN KEY(topicid) REFERENCES msgtopic(ROWID))");

	LoginManager loginManager(db);

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
