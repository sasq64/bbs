
#include <coreutils/log.h>
#include <bbsutils/telnetserver.h>
#include <bbsutils/console.h>
#include <bbsutils/editor.h>

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
		auto termType = session.getTermType();		

		unique_ptr<Console> console;
		if(termType.length() > 0) {
			console = unique_ptr<Console>(new AnsiConsole { session });
		} else {
			console = unique_ptr<Console>(new PetsciiConsole { session });
		}
		console->flush();
		auto h = console->getHeight();
		auto w = console->getWidth();
		LOGD("New connection, TERMTYPE '%s' SIZE %dx%d", termType, w, h);

		console->write("\nNAME:");
		auto userName = console->getLine();
		chatLines.push_back(userName + " joined");

		int lastLine = 0;
		int ypos = 0;
		console->clear();
		console->put(0, h-2, string(w,'-'), Console::CURRENT_COLOR, Console::BLUE);
		console->moveCursor(0, h-1);
		
		//auto line = console->getLineAsync();
		auto le =  unique_ptr<LineEditor>(new LineEditor(*console));

		{ lock_guard<mutex> guard(chatLock);
			if((int)chatLines.size() > h)
				lastLine = chatLines.size() - h;
		}
		while(true) {

			if(le->update(500) == 0) {
				auto line = le->getResult();
				{ lock_guard<mutex> guard(chatLock);
					chatLines.push_back(userName + ": " + line);
				}
				console->put(0, h-1, string(w,' '), Console::CURRENT_COLOR, Console::BLACK);
				console->moveCursor(0,h-1);
				le = unique_ptr<LineEditor>(new LineEditor(*console));
			}

			{ lock_guard<mutex> guard(chatLock);
				bool newLines = false;
				while((int)chatLines.size() > lastLine) {
					newLines = true;
					auto msg = chatLines[lastLine++];
					ypos++;
					if(ypos > h-2) {
						console->scrollScreen(1);
						console->put(0, h-3, string(w,' '), Console::CURRENT_COLOR, Console::BLACK);
						console->put(0, h-2, string(w,'-'), Console::CURRENT_COLOR, Console::BLUE);
						ypos--;
					}
					console->put(0, ypos-1, msg);

				}
				if(newLines)
					le->refresh();

				console->flush();
			}

		}

	});
	telnet.run();

	return 0;
}
