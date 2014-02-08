
class ComBoard {
public:

	struct Command {
		typedef std::function<void(const std::vector<std::string> &args)> Function;
		Command(const std::string &text, Function f) : func {f}, text {utils::split(text)} {}
		Function func;
		const std::vector<std::string> text;
	};

	ComBoard(MessageBoard &board, bbs::Console &console);
	void exec(const std::string &line);
private:
	MessageBoard &board;
	bbs::Console &console;
	std::vector<Command> commands;
};
