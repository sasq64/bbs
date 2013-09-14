
#include <coreutils/log.h>
#include <coreutils/utils.h>
#include <bbsutils/telnetserver.h>
#include <bbsutils/console.h>
#include <bbsutils/editor.h>

#include <string>

using namespace std;
using namespace bbs;
using namespace utils;

int menu(Console &console, const vector<pair<char, string>> &entries) {

	auto contents = console.getTiles();

	auto w = console.getWidth();
	auto h = console.getHeight();
	size_t maxl = 0;


	for(const auto &e : entries) {
		auto l = e.second.length();
		if(l > maxl)
			maxl = l;
	}

	auto menuw = maxl + 7;
	auto menuh = entries.size() + 2;
	auto menux = (w - menuw)/2;
	auto menuy = (h - menuh)/2;

	console.fill(Console::LIGHT_GREY, menux, menuy, menuw, menuh);
	auto y = menuy+1;
	for(const auto &e : entries) {
		console.put(menux+1, y++, format("[%c] %s", e.first, e.second));
	}
	console.flush();

	auto k = console.getKey();

	console.setTiles(contents);

	return k;

}

string makeSize(int bytes) {
	auto suffix = vector<string>{ "B", "K", "M", "G", "T" };
	auto s = 0;
	while(bytes > 1024) {
		bytes /= 1024;
		s++;
	}
	return format("%d%s", bytes, s);
}

void shell(Console &console) {

	console.write("System shell. Use 'exit' to quit\n\n");

	string rootDir = "/home/sasq/projects/bbs";
	File currentDir { rootDir };

	while(true) {

		console.write(currentDir.getName() + " # ");
		auto line = console.getLine();
		console.write("\n");

		auto parts = split(line);
		if(parts.size() < 1)
			continue;
		auto cmd = parts[0];


		if(cmd == "ls") {
			for(const auto &f : currentDir.listFiles()) {
				console.write(format("%-32s %s\n", path_filename(f.getName()), makeSize(f.getSize())));
			}
		} else if(cmd == "cd") {
			File newDir { currentDir, parts[1] };
			if(newDir.exists())
				currentDir = newDir;
		} else if(cmd == "cat") {
			File file { currentDir, parts[1] };
			string contents((char*)file.getPtr(), file.getSize());
			console.write(contents);
		} else if(cmd == "exit")
			return;
	}
}

int main(int argc, char **argv) {

	setvbuf(stdout, NULL, _IONBF, 0);

	logging::setLevel(logging::DEBUG);

	vector<string> chatLines;
	mutex chatLock;

	TelnetServer telnet { 12345 };
	telnet.setOnConnect([&](TelnetServer::Session &session) {
		session.echo(false);
		auto termType = session.getTermType();		

		unique_ptr<Console> con;
		if(termType.length() > 0) {
			con = make_unique<AnsiConsole>(session);
		} else {
			con = make_unique<PetsciiConsole>(session);
		}
		Console &console = *con;

		console.flush();
		auto h = console.getHeight();
		auto w = console.getWidth();
		LOGD("New connection, TERMTYPE '%s' SIZE %dx%d", termType, w, h);

		while(true) {
			int what = menu(console, { { 'c', "Enter chat" }, { 's', "Start shell" }, { 'x', "Log out" } });
			LOGD("WHAT %d", what);
			//if(what == 1)
				shell(console);
			//else
			//	break;
		}

		console.write("\nNAME:");
		auto userName = console.getLine();
		chatLines.push_back(userName + " joined");

		auto lastLine = 0;
		auto ypos = 0;
		console.clear();
		console.fill(Console::BLUE, 0, -2, 0, 1);
		console.put(0, -2, "NAME: " + userName, Console::CURRENT_COLOR, Console::BLUE);
		console.moveCursor(0, -1);
		
		auto lineEd = make_unique<LineEditor>(console);

		{ lock_guard<mutex> guard(chatLock);
			if((int)chatLines.size() > h)
				lastLine = chatLines.size() - h;
		}
		while(true) {

			auto key = lineEd->update(500);
			if(key >= 0)
				LOGD("Key %d", key);
			switch(key) {
			case 0:
				{ lock_guard<mutex> guard(chatLock);
					auto line = lineEd->getResult();
					chatLines.push_back(userName + ": " + line);
				}
				console.fill(Console::BLACK, 0, -1, 0, 1);
				console.moveCursor(0, -1);
				lineEd = make_unique<LineEditor>(console);
				break;
			case Console::KEY_F7:
				menu(console, { { 'c', "Change Name" }, { 'x', "Leave chat" }, { 'l', "List Users" } });
				break;
			}

			{ lock_guard<mutex> guard(chatLock);
				auto newLines = false;
				while((int)chatLines.size() > lastLine) {
					newLines = true;
					auto msg = chatLines[lastLine++];
					ypos++;
					if(ypos > h-2) {
						console.scrollScreen(1);
						console.fill(Console::BLACK, 0, -3, 0, 1);
						console.fill(Console::BLUE, 0, -2, 0, 1);
						ypos--;
					}
					console.put(0, ypos-1, msg);

				}
				if(newLines)
					lineEd->refresh();

				console.flush();
			}
		}
	});
	telnet.run();

	return 0;
}
