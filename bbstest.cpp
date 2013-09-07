
#include <coreutils/log.h>
#include <bbsutils/telnetserver.h>
#include <bbsutils/console.h>

#include <string>

using namespace std;
using namespace bbs;

#if 0
int main(int argc, char **argv) {

	TelnetServer telnet { 12345 };
	telnet.setOnConnect([&](TelnetServer::Session &session) {
		session.write("hello world\r\n");
		telnet.quit();
	});
	telnet.run();

	return 0;
}
#endif

int main(int argc, char **argv) {

	setvbuf(stdout, NULL, _IONBF, 0);

	logging::setLevel(logging::DEBUG);
/*
	{
		logging::setLevel(logging::OFF);
		unique_ptr<Console> console { createLocalConsole() };
		console->write("Hello:");
		auto text = console->getLine();
		console->write("\nHello '" + text + "'\n");
		logging::setLevel(logging::DEBUG);
	}*/

	vector<string> chatLines;
	mutex chatLock;

	TelnetServer telnet { 12345 };
	telnet.setOnConnect([&](TelnetServer::Session &session) {
		session.echo(false);
		string termType = session.getTermType();		
		LOGD("New connection, TERMTYPE '%s'", termType);

		unique_ptr<Console> console;
		if(termType.length() > 0) {
			console = unique_ptr<Console>(new AnsiConsole { session });
		} else {
			console = unique_ptr<Console>(new PetsciiConsole { session });
		}
		console->flush();
		auto h = console->getHeight();
		auto w = console->getWidth();

		string userName;
		console->write("\nNAME:");
		userName = console->getLine();
		chatLines.push_back(userName + " joined");

		console->clear();
		int ypos = 0;
		console->put(0, h-2, string(w,'-'));
		int lastLine = 0;

		console->moveCursor(0, h-1);
		auto line = console->getLineAsync();

		{ lock_guard<mutex> guard(chatLock);
			if((int)chatLines.size() > h)
				lastLine = chatLines.size() - h;
		}
		while(true) {
			if(line.wait_for(chrono::milliseconds(250)) == future_status::ready) {
				{ lock_guard<mutex> guard(chatLock);
					chatLines.push_back(userName + ": " + line.get());
				}
				console->put(0, h-1, string(w,' '));
				console->moveCursor(0,h-1);
				line = console->getLineAsync();
			}

			{ lock_guard<mutex> guard(chatLock);
				while((int)chatLines.size() > lastLine) {
					auto msg = chatLines[lastLine++];
					ypos++;
					if(ypos > h-2) {
						console->scroll(0,1);
						console->put(0, h-3, string(w,' '));
						console->put(0, h-2,string(w,'-'));
						ypos--;
					}
					console->put(0, ypos-1, msg);

				}
				console->flush();
			}

		}

	});
	telnet.run();

	return 0;
}