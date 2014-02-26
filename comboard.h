#ifndef COMBOARD_H
#define COMBOARD_H

#include "messageboard.h"

#include "bbs.h"

#include <bbsutils/console.h>
#include <map>
#include <unordered_map>

class ComBoard {
public:

	struct Command {
		enum ArgType {
			MESSAGE = 1,
			STRING,
			GROUP,
			USER,
			NUMBER,
			TOPIC,
		};

		struct Arg {
			Arg(ArgType at, bool opt) : argtype(at), optional(opt) {}
			ArgType argtype;
			bool optional;
		};

		static std::vector<Arg> parse_args(const std::string &s) {
			static std::map<char, ArgType> types = { { 'm', MESSAGE }, { 's', STRING }, { 'g', GROUP }, { 'u', USER }  };
			std::vector<Arg> args; 
			bool opt = false;
			for(auto c : s) {
				if(c == '?')
					opt = true;
				else if(c == '!')
					opt = false;
				else {
					auto at = types[c];		
					args.emplace_back(at, opt);
				}
			}
			return args;
		}

		typedef std::function<void(const std::vector<std::string> &args)> Function;
		//Command(const std::string &text, Function f) : func {f}, full_text(text), text {utils::split(text)} {}
		Command(const std::string &text, const std::string &desc, Function f, uint32_t umask = 0) : func {f}, description {desc}, full_text(text), text {utils::split(text)}, umask {umask} {}
		Command(const std::string &text, const std::string &desc, const std::string &args, Function f, uint32_t umask = 0) : func {f}, args {parse_args(args)}, description {desc}, full_text(text), text {utils::split(text)}, umask {umask} {}
		Function func;
		const std::vector<Arg> args;
		const std::string description;
		const std::string full_text;
		const std::vector<std::string> text;
		uint32_t umask;

	};

	ComBoard(BBS::Session &session, MessageBoard &board, bbs::Console &console);
	bool exec(const std::string &line);
	std::string suggested_command();
	template <class... A> void write(const std::string &text, const A& ... args);
	bool show_text(const std::string &what);
private:

	uint64_t find_first_unread(uint64_t msg_id);
	void select_topic(uint64_t topic_id);
	std::string edit();
	std::string fulledit(const std::string &text = "", bool comment = false);
	void show_message(const MessageBoard::Message &msg);

	BBS::Session &session;

	std::unordered_map<std::string, int> settings;
	//LoginManager &users;
	BBS &bbs;
	MessageBoard &board;
	bbs::Console &console;
	std::vector<Command> commands;
	MessageBoard::Message currentMsg;
	MessageBoard::Message lastShown;
	MessageBoard::Topic currentTopic;
	std::map<uint64_t, MessageBoard::Message> topicMap;

	std::unordered_map<std::string, std::string> &variables;
	std::unordered_map<std::string, std::string> &texts;

};

#endif // COMBOARD_H