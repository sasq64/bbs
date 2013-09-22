// 
#include <coreutils/log.h>
#include <coreutils/utils.h>
#include <bbsutils/telnetserver.h>
#include <bbsutils/console.h>
#include <bbsutils/editor.h>

#include <bbsutils/ansiconsole.h>
#include <bbsutils/petsciiconsole.h>

#include <string>
#include <algorithm>

using namespace std;
using namespace bbs;
using namespace utils;

void square(Console &console, int x, int y, int w, int h, bool shadow = false) {
//┌┐
//└┘─│

	for(auto xx = 0; xx<w; xx++) {
		console.put(x+xx,y, L'─');
		console.put(x+xx,y+h-1, L'─');
		console.put(x+xx+1,y+h, L'▒');
	}

	for(auto yy = 0; yy<h; yy++) {
		console.put(x,y+yy, L'│');
		console.put(x+w-1,y+yy, L'│');
		console.put(x+w,y+yy+1, L'▒');
	}

	console.put(x,y, L'┌');
	console.put(x+w-1,y, L'┐');
	console.put(x,y+h-1, L'└');
	console.put(x+w-1,y+h-1, L'┘');

}

int menu(Console &console, const vector<pair<char, string>> &entries) {

	auto contents = console.getTiles();

	auto w = console.getWidth();
	auto h = console.getHeight();

	// Find max string length
	auto maxit = max_element(entries.begin(), entries.end(), [](const pair<char, string> &e0, const pair<char, string> &e1) {
		return e0.second.length() < e1.second.length();
	});
	auto maxl = maxit->second.length();

	// Bounds of menu rectangle
	auto menuw = maxl + 7;
	auto menuh = entries.size() + 2;
	auto menux = (w - menuw)/2;
	auto menuy = (h - menuh)/2;

	console.setColor(Console::WHITE, Console::LIGHT_GREY);

	console.fill(Console::LIGHT_GREY, menux, menuy, menuw, menuh);
	square(console, menux, menuy, menuw, menuh);
	auto y = menuy+1;
	for(const auto &e : entries) {
		console.put(menux+1, y++, format("[%c] %s", e.first, e.second));
	}
	console.flush();

	console.setColor(Console::BLACK, Console::BLACK);

	while(true) {
		auto k = console.getKey();

		// Check that the key is among the ones given
		auto it = find_if(entries.begin(), entries.end(), [&](const pair<char, string> &e) {
			return (k == e.first);
		});
		if(it != entries.end()) {
			console.setTiles(contents);
			console.setColor(Console::WHITE, Console::BLACK);
			return it - entries.begin();
		}
	}
	//return k;

}

string makeSize(int bytes) {
	auto suffix = vector<string>{ "B", "K", "M", "G", "T" };
	auto s = 0;
	while(bytes > 1024) {
		bytes /= 1024;
		s++;
	}
	return format("%d%s", bytes, suffix[s]);
}

void shell(Console &console) {

	console.write("System shell. Use 'exit' to quit\n\n");

	File rootDir = { "/home/sasq/projects/bbs" };
	auto rootName = rootDir.getName();
	auto rootLen = rootName.length();
	auto currentDir = rootDir;

	while(true) {

		auto d = currentDir.getName().substr(rootLen);
		if(d == "") d = "/";

		console.write(d + " # ");
		auto line = console.getLine();
		console.write("\n");

		auto parts = split(line);
		if(parts.size() < 1)
			continue;
		auto cmd = parts[0];


		if(cmd == "ls") {
			for(const auto &f : currentDir.listFiles()) {
				auto n = path_filename(f.getName());
				if(f.isDir())
					n += "/";
				console.write(format("%-32s %s\n", n, makeSize(f.getSize())));
			}
		} else if(cmd == "cd") {
			File newDir { currentDir, parts[1] };
			if(newDir.isChildOf(rootDir) && newDir.exists())
				currentDir = newDir;
		} else if(cmd == "cat") {
			File file { currentDir, parts[1] };
			string contents((char*)file.getPtr(), file.getSize());
			console.write(contents);
			console.write("\n");
		} else if(cmd == "ed") {
			auto saved = console.getTiles();
			auto x = console.getCursorX();
			auto y = console.getCursorY();
			File file { currentDir, parts[1] };
			string contents((char*)file.getPtr(), file.getSize());
			FullEditor ed(console);
			ed.setString(contents);
			while(true) {
				auto rc = ed.update(500);
				if(rc == Console::KEY_F1) {
					console.setTiles(saved);					
					console.flush();
					console.moveCursor(x, y);
					break;
				}
			}
		} else if(cmd == "exit")
			return;
	}
}

int main(int argc, char **argv) {

	setvbuf(stdout, NULL, _IONBF, 0);
	//logging::setLevel(logging::INFO);

	vector<string> chatLines;
	mutex chatLock;

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

			console.flush();

			auto h = console.getHeight();
			auto w = console.getWidth();
			LOGD("New connection, TERMTYPE '%s' SIZE %dx%d", termType, w, h);
			while(true) {	
				int what = menu(console, { { 'c', "Enter chat" }, { 's', "Start shell" }, { 'x', "Log out" } });
				LOGD("WHAT %d", what);
				if(what == 2) {
					//session.disconnect();
					session.close();
					return;
				} else
				if(what == 1)
					shell(console);
				else
					break;
			}

			console.put(0,0,"┏──────────────┐");
			console.put(0,1,"│NAME:         │");
			console.put(0,2,"└──────────────┛");
			console.flush();
/*
╋━┃
  ╋			
┏┓┗┛
╱╲╳
╭─╮
╰─╯
┌┐
└┘
─│┼
 
▖▗▘▙▚▛▜▝▞▟

▒
*/
			//console.write("\nNAME:");
			console.moveCursor(6, 1);
			auto userName = console.getLine(9);
			chatLines.push_back(userName + " joined");

			uint lastLine = 0;
			auto ypos = 0;
			console.clear();
			console.fill(Console::BLUE, 0, -2, 0, 1);
			console.put(0, -2, "NAME: " + userName, Console::CURRENT_COLOR, Console::BLUE);
			//session.write("░▒▓█");

			console.moveCursor(0, -1);
			
			auto lineEd = make_unique<LineEditor>(console, 10);

			{ lock_guard<mutex> guard(chatLock);
				if((int)chatLines.size() > h)
					lastLine = chatLines.size() - h;
			}
			while(true) {

				auto key = lineEd->update(500);
				if(key >= 0)
					LOGD("Key %d", key);
				switch(key) {
				case Console::KEY_ENTER:
					{ lock_guard<mutex> guard(chatLock);
						auto line = lineEd->getResult();
						chatLines.push_back(userName + ": " + line);
					}
					console.fill(Console::BLACK, 0, -1, 0, 1);
					console.moveCursor(0, -1);
					lineEd = make_unique<LineEditor>(console, 10);
					break;
				case Console::KEY_F7:
					menu(console, { { 'c', "Change Name" }, { 'x', "Leave chat" }, { 'l', "List Users" } });
					break;
				}

				{ lock_guard<mutex> guard(chatLock);
					auto newLines = false;
					while(chatLines.size() > lastLine) {
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
		} catch (TelnetServer::disconnect_excpetion e) {
			LOGD("Client disconnected");
		}
	});
	telnet.run();

	return 0;
}
