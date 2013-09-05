
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
		while(true) {
			console->write("\nNAME:");
			auto line = console->getLine();
			if(line == "sasq") {
				console->write("Welcome!\n");
				break;
			}
		}
		while(true) {
			console->write("\n>> ");
			auto line = console->getLine();
			if(line == "quit")
				break;
		}

	});
	telnet.run();

	return 0;
}
