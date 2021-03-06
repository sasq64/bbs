// 
#include <coreutils/log.h>
#include <coreutils/utils.h>
#include <coreutils/file.h>
#include <bbsutils/console.h>

#include <bbsutils/ansiconsole.h>
#include <bbsutils/petsciiconsole.h>

#include <unordered_set>
#include <string>
#include <algorithm>
#include <functional>

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
	console.putChar(142);
	console.setTiles(tiles);
	console.flush();
	console.moveCursor(39, 24);
	console.setColor(Console::BLACK, Console::BLACK);
	console.getKey();
	console.clear();
	console.putChar(14);
}

struct Area {
	Area(int x0, int y0, int x1, int y1) : x0(x0), y0(y0), x1(x1), y1(y1) {}
	Area(std::initializer_list<int> &il) {
		auto it = il.begin();
		x0 = *it;
		++it;
		y0 = *it;
		++it;
		x1 = *it;
		++it;
		y1 = *it;
	}
	int x0;
	int y0;
	int x1;
	int y1;
};

template <typename T> class ListView {
public:

	typedef function<void(Console &c, Area &a, int item, const T &data)> RenderFunction;
	typedef function<T(int item)> SourceFunction;

	ListView(Console &c, const Area &a, RenderFunction renderFunc, SourceFunction sourceFunc, int items) :
		ypos(0), console(c), area(a), renderFunc(renderFunc), sourceFunc(sourceFunc), items(items) {}

	void update() {
		//auto key = console.getKey(100);
		console.fill(Console::CURRENT_COLOR, area.x0, area.y0, area.x1-area.x0, area.y1-area.y0);
		console.clipArea(area.x0, area.y0, area.x1, area.y1);
		Area a = area;
		for(int i = ypos; i<items; i++) {
			if(a.y0 >= a.y1)
				break;
			renderFunc(console, a, i, sourceFunc(i));
		}
		console.clipArea();
		console.flush();
	}

	static SourceFunction makeVectorSource(const vector<T> &v) {
		return [&](int item) -> T {
			return v[item];
		};
	};

	static RenderFunction simpleRenderFunction;

private:

	int ypos;
	Console &console;
	Area area;
	RenderFunction renderFunc;
	SourceFunction sourceFunc;
	int items;
};

/*
template<> typename ListView<string>::RenderFunction ListView<string>::simpleRenderFunction =
	[](Console &c, Area &a, int item, const string &data) {
		c.put(a.x0, a.y0++, data);
	};
*/