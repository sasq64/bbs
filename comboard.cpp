
ComBoard::ComBoard(MessageBoard &board, Console &console) : board(board), console(console), 
	commands { 			
		{ "list groups", [&](const std::vector<std::string> &args) { 
			LOGD("List groups");
			for(const auto &g : board.list_groups()) {
				time_t tt = (time_t)g.last_post;
				const char *t = ctime(&tt);
				console.write(format("%s [Created by %d] LAST POST: %s\n", g.name, g.creator, t));
			}
		} },
		{ "list topics", [&](const std::vector<std::string> &args) { 
			LOGD("List topics");
			for(const auto &t : board.list_topics()) {
				console.write(format("%s (%s)\n", t.name, t.starter));
			}
		} },
		{ "next group", [&](const std::vector<std::string> &args) { 
			for(const auto &g : board.list_groups()) {
				time_t tt = (time_t)g.last_post;
				const char *t = ctime(&tt);
				console.write(format("%s [Created by %d] LAST POST: %s\n", g.name, g.creator, t));
			}

		} },
		{ "create group", [&](const std::vector<std::string> &args) {
			auto g = board.create_group(args[2]);
			if(g == 0)
				console.write("Create group failed\n");
			else
				console.write(format("Group '%s' created\n"));
		} },
		{ "enter group", [&](const std::vector<std::string> &args) {
			auto g = board.enter_group(args[2]);
			if(g == 0)
				console.write("No such group\n");
		} },
		{ "post", [&](const std::vector<std::string> &args) { 
			//LOGD("List groups"); 
			auto topic = console.getLine("TOPIC:");
			auto ed = FullEditor(console);
			while(true) {
				auto rc = ed.update(500);
				if(rc == Console::KEY_F7) {
					break;
				}
			}
			auto text = ed.getResult();
			board.post(topic, text);
		} },
	}
{}

void ComBoard::exec(const std::string &line) {
	auto parts = split(line);
	if(parts.size() < 1)
		return;

	for(const auto &c : commands) {
		bool found = true;
		for(int i=0; i<c.text.size(); i++) {
			if(c.text[i].compare(0, parts[i].length(), parts[i]) != 0) {
				found = false;
				break;
			}
		}
		if(found) {
			c.func(parts);
			break;
		}
	}
}
