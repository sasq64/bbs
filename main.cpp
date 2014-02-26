#include "loginmanager.h"
#include "messageboard.h"
#include "comboard.h"
#include "bbstools.h"

#include "bbs.h"

#include <coreutils/log.h>
#include <coreutils/utils.h>
#include <coreutils/file.h>
#include <coreutils/bitfield.h>
#include <bbsutils/telnetserver.h>
#include <bbsutils/console.h>
#include <bbsutils/editor.h>
#include <bbsutils/ansiconsole.h>
#include <bbsutils/petsciiconsole.h>
//#include <crypto/sha256.h>
//#include <sqlite3/database.h>

#include <time.h>
#include <cmath>

using namespace bbs;
using namespace utils;
using namespace std;

//#define BACKWARD_HAS_BFD 1
//#include "backward-cpp/backward.hpp"
//namespace backward {
//backward::SignalHandling sh;
//} // namespace backward

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

void petsciiArt(Console &console) {
	while(true) {
		static vector<string> pics = { "pilt.c64", "ninja.c64", "floppy.c64", "gary.c64", "victor.c64", "robot.c64",
		                                "bigc.c64", "play.c64", "voodoo.c64", "santa.c64", "saved.c64", "bigc.c64" };
		int pic = menu(console, {
			{ '1', "Pal - Petscii & Pilt" },
			{ '2', "wile coyote - The Last Ninja" },
			{ '3', "Redcrab - I HAS FLOPPY!!" },
			{ '4', "Mermaid - Gary" },
			{ '5', "Mermaid - Victor Charlie" },
			{ '6', "wile coyote - Daft Robot" },
			{ '7', "Nuckhead - Big C" },
			{ '8', "ilesj - Hey let's play!" },
			{ '9', "wile coyote - voodoo love" },
			{ 'a', "Hermit - Mikulas vs Krampusz" },
			{ 'b', "Uneksija - T.D.T.W.W.N.S" },
			{ 'x', "BACK" },
		});
		if(pic == 11)
			break;
		if(pic >= 0) {
			try {
				showPetscii(console, string("art/") + pics[pic]);
			} catch(utils::file_not_found_exception &e) {
				LOGD("Could not find '%s'", pics[pic]);
			}
		}
	}
}

int main(int argc, char **argv) {

	setvbuf(stdout, NULL, _IONBF, 0);
	//logging::setLevel(logging::INFO);
	// Turn off logging from the utility classes
	//logging::useLogSpace("utils", false);
#ifdef BACKWARD_CPP
	std::set_terminate([]() {
		LOGD("In terminate");
		//if(std::uncaught_exception()) {
			auto e = std::current_exception();
			try {
            	rethrow_exception(e);
            } catch (sqlite3db::db_exception &de) {
       			backward::Printer p;
       			de.print_stack();
            	rethrow_exception(e);
            } catch (msgboard_exception &me) {
       			backward::Printer p;
       			p.print(me.stack_trace);
            	rethrow_exception(e);
            } catch (std::exception &se) {
            	LOGD(se.what());
            }
        //}
 	});
#endif

	BBS::init( "bbs.db" );

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
			BBS::Session bbs_session;
			while(true) {
				//break;
				auto l = console.getLine("LOGON:");
				auto p = console.getPassword("PASSWORD:");
				//userId = loginManager.login_user(l, p);
				bbs_session = BBS::instance().login(l, p);
				userId = bbs_session.user_id();
				if(userId != LoginManager::NO_USER)
					break;
				if(tries == 0)
					session.disconnect();
				else
					tries--;
			}


			MessageBoard board(BBS::instance().getdb(), userId);
			ComBoard comboard(bbs_session, board, console);

			console.clear();
			console.moveCursor(0,0);

			//console.write(L"┌────────────────────────────┐\n");
			//console.write(L"│▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒│\n");
			//console.write(L"└────────────────────────────┘\n");
			comboard.show_text("login");
			//console.write(L"▒▒▒▒▒▒▒▒▒ NIGHTMODE ▒▒▒▒▒▒▒▒▒\n");

			comboard.show_text("intro");

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
				if(line == "petscii") {
					petsciiArt(console);
				} else
				if(!comboard.exec(line)) {
					//loginManager.logout_user(userId);
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
			//loginManager.logout_user(userId);
			//addChatLine(user.name() + " disconnected");
			//{ lock_guard<mutex> guard(chatLock);
			//	users.erase(user);
			//}
		}
	});
	telnet.run();

	return 0;
}
