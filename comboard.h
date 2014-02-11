#ifndef COMBOARD_H
#define COMBOARD_H

#include "loginmanager.h"
#include "messageboard.h"
#include <bbsutils/console.h>
#include <map>

class ComBoard {
public:

	struct Command {
		typedef std::function<void(const std::vector<std::string> &args)> Function;
		Command(const std::string &text, Function f) : func {f}, text {utils::split(text)} {}
		Function func;
		const std::vector<std::string> text;
	};

	ComBoard(LoginManager &lm, MessageBoard &board, bbs::Console &console);
	void exec(const std::string &line);
private:

	uint64_t find_first_unread(uint64_t msg_id);
	void select_topic(uint64_t topic_id);
	std::string edit();
	void show_message(const MessageBoard::Message &msg);


	LoginManager users;
	MessageBoard &board;
	bbs::Console &console;
	std::vector<Command> commands;
	MessageBoard::Message currentMsg;
	MessageBoard::Message lastShown;
	MessageBoard::Topic currentTopic;
	std::map<uint64_t, MessageBoard::Message> topicMap;
};

#endif // COMBOARD_H