// 
#include <coreutils/log.h>
#include <coreutils/utils.h>
#include <coreutils/file.h>
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

void draw_square(Console &console, int x, int y, int w, int h, bool shadow = false) {

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
	draw_square(console, menux, menuy, menuw, menuh);
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
}

vector<string> chatLines;
vector<string> users;
mutex chatLock;

void chat(Console &console, string userName) {

	{ lock_guard<mutex> guard(chatLock);
		chatLines.push_back(userName + " joined");
		users.push_back(userName);
	}

	auto w = console.getWidth();
	auto h = console.getHeight();

	uint lastLine = 0;
	auto ypos = 0;

	auto addLine = [&](const string &line) {
		ypos++;
		if(ypos > h-2) {
			console.scrollScreen(1);
			console.fill(Console::BLACK, 0, -3, 0, 1);
			console.fill(Console::BLUE, 0, -2, 0, 1);
			console.put(-13, -2, "F7 = Options", Console::WHITE, Console::BLUE);
			ypos--;
		}
		console.put(0, ypos-1, line);
	};

	console.clear();
	console.fill(Console::BLUE, 0, -2, 0, 1);
	console.put(-13, -2, "F7 = Options", Console::WHITE, Console::BLUE);
	console.moveCursor(0, -1);
	
	auto lineEd = make_unique<LineEditor>(console);

	{ lock_guard<mutex> guard(chatLock);
		if((int)chatLines.size() > h)
			lastLine = chatLines.size() - h;
	}
	while(true) {

		{ lock_guard<mutex> guard(chatLock);
			auto newLines = false;
			while(chatLines.size() > lastLine) {
				newLines = true;
				auto msg = chatLines[lastLine++];
				auto wrapped = text_wrap(msg, w, w-2);
				bool first = true;
				for(auto &l : wrapped) {
					addLine(l);
					if(first) {
						auto colon = l.find(':');
						if(colon != string::npos) {									
							console.put(0, ypos-1, l.substr(0,colon), Console::LIGHT_BLUE);
							console.put(colon, ypos-1, l.substr(colon));
						} else
							console.put(0, ypos-1, l);
					} else {
						console.put(0, ypos-1, "::", Console::DARK_GREY);
						console.put(2, ypos-1, l);
					}
					first = false;
				}
			}
			if(newLines)
				lineEd->refresh();

			console.flush();
		}

		auto key = lineEd->update(500);

		switch(key) {
		case Console::KEY_ENTER:
			{ lock_guard<mutex> guard(chatLock);
				auto line = lineEd->getResult();
				chatLines.push_back(userName + ": " + line);
			}
			console.fill(Console::BLACK, 0, -1, 0, 1);
			console.moveCursor(0, -1);
			lineEd->setString("");
			break;
		case Console::KEY_F7:
			{
				int rc = menu(console, { { 'c', "Change Name" }, { 'x', "Leave chat" }, { 'l', "List Users" } });
				if(rc == 0) {
					console.moveCursor(0,-2);
					console.setColor(Console::WHITE, Console::BLUE);
					console.write("NEW NAME:");
					auto newName = console.getLine();
					console.setColor(Console::WHITE, Console::BLACK);
					addLine(format("%s changed his name to %s", userName, newName));
					userName = newName;
					console.fill(Console::BLUE, 0, -2, 0, 1);
					console.put(-13, -2, "F7 = Options", Console::WHITE, Console::BLUE);
					console.moveCursor(0, -1);
				} else
				if(rc == 1) {
					return;
				} else
				if(rc == 2) {
					lock_guard<mutex> guard(chatLock);
					for(auto &u : users) {
						addLine(u);
					}
				}
			}
			break;
		}
	}
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

	console.clear();
	console.write("\nSystem shell. 'help' for commands\n\n");

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

		if(cmd == "help") {
			console.write("Commands:\n ls\n cd <directory>\n cat <filename>\n ed <filename>\n exit\n");
		} else if(cmd == "ls") {
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
			auto xy = console.getCursor();
			File file { currentDir, parts[1] };
			string contents((char*)file.getPtr(), file.getSize());
			FullEditor ed(console);
			ed.setString(contents);
			while(true) {
				auto rc = ed.update(500);
				if(rc == Console::KEY_F7) {
					console.setTiles(saved);					
					console.flush();
					console.moveCursor(xy);
					break;
				}
			}
		} else if(cmd == "exit")
			return;
	}
}

int screen2pet(int x) {
	if(x == 94) x = 255;
	else if(x < 32) x += 64;
	else if(x < 64) x += 0;
	else if(x < 96) x += 128;
	else if(x < 128) x += 64;
	else if(x < 160) x -= 64;
	else if(x < 192) x -= 128;
	else if(x >= 224) x -= 64;
	return x;
}

void showPetscii(Console &console, const std::string &name) {

	static vector<Console::Color> colors = { Console::BLACK, Console::WHITE, Console::RED, Console::CYAN, Console::PURPLE, Console::GREEN, Console::BLUE, Console::YELLOW, Console::ORANGE, Console::BROWN, Console::PINK, Console::DARK_GREY, Console::GREY, Console::LIGHT_GREEN, Console::LIGHT_BLUE, Console::LIGHT_GREY };

	auto tiles = console.getTiles();
	File f { name };
	vector<uint8_t> ch(1000);
	vector<uint8_t> col(1000);
	f.read(&ch[0], 1000);
	f.read(&col[0], 1000);

	int i = 0;
	for(auto &t : tiles) {
		t.c = screen2pet(ch[i]);
		if(ch[i] < 128) {
			t.fg = colors[col[i++] & 0xf];
			t.bg = Console::BLACK;
		} else {
			t.bg = colors[col[i++] & 0xf];
			t.fg = Console::BLACK;
		}
	}
	console.rawPut(142);
	console.setTiles(tiles);
	console.flush();
	console.moveCursor(39, 24);
	console.setColor(Console::BLACK, Console::BLACK);
	console.getKey();
	console.clear();
	console.rawPut(14);
}

int main(int argc, char **argv) {

	setvbuf(stdout, NULL, _IONBF, 0);
	//logging::setLevel(logging::INFO);

	// Turn off logging from the utility classes
	//logging::useLogSpace("utils", false);

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

			console.clear();

			auto h = console.getHeight();
			auto w = console.getWidth();
			LOGD("New connection, TERMTYPE '%s' SIZE %dx%d", termType, w, h);


			console.write("NAME:");
			auto userName = console.getLine();

			//console.write("\nPASSWORD:");
			//auto password = console.getPassword();

			LOGD("%s logged in", userName);

			while(true) {	
				int what = menu(console, { { 'p', "Petscii Art" }, { 'c', "Enter chat" }, { 'x', "Log out" } });
				if(what == 2) {
					session.close();
					return;
				} else
				if(what == 0) {
					//shell(console);
					static vector<string> pics = { "art/pilt.c64", "art/ninja.c64", "art/loppy.c64", "art/gary.c64", "art/victor.c64", "art/robot.c64" };
					int pic = menu(console, {
						{ '1', "Pal - Petscii & Pilt" },
						{ '2', "wile coyote - The Last Ninja" },
						{ '3', "Redcrab - I HAS FLOPPY!!" },
						{ '4', "Mermaid - Gary" },
						{ '5', "Mermaid - Victor Charlie" },
						{ '6', "wile coyote - Daft Robot" },
				});
					if(pic >= 0)
						showPetscii(console, pics[pic]);
				} else
				if(what == 1)
					chat(console, userName);
			}
		} catch (TelnetServer::disconnect_excpetion e) {
			LOGD("Client disconnected");
		}
	});
	telnet.run();

	return 0;
}
