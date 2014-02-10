
#include "comboard.h"

#include <bbsutils/editor.h>

using namespace bbs;
using namespace utils;
using namespace std;

ComBoard::ComBoard(MessageBoard &board, Console &console) : board(board), console(console), 
	commands { 			
		{ "list groups", [&](const vector<string> &args) { 
			LOGD("List groups");
			for(const auto &g : board.list_groups()) {
				//time_t tt = (time_t)g.last_post;
				//const char *t = ctime(&tt);
				console.write(format("%s [Created by %d] LAST POST: %s\n", g.name, g.creator, 0));
			}
		} },

		{ "list topics", [&](const vector<string> &args) { 
			LOGD("List topics");
			for(const auto &t : board.list_topics()) {
				console.write(format("%s (%d)\n", t.name, t.creator));
			}
		} },

		{ "next topic", [&](const vector<string> &args) {
			auto msgid = board.get_first_unread_msg();
			auto msg = board.get_message(msgid);
			auto topic = board.get_topic(msg.topic);
			auto group = board.get_group(topic.group);
			console.write("Group '%s' topic '%s'\n", group.name, topic.name);
			board.enter_group(group.id);
			select_topic(topic.id);
			//currentMsgThread = board.get_thread(topic.first_msgid);
			//auto msg_id = topic.first_msgid;
			msgid = find_first_unread(topic.first_msg);
			currentMsg = board.get_message(msgid);
		} },


		{ "read next", [&](const vector<string> &args) {
			console.write(currentMsg.text);
			auto msgid = find_first_unread(currentMsg.id);
			currentMsg = board.get_message(msgid);

		} },
		{ "create group", [&](const vector<string> &args) {
			auto g = board.create_group(args[2]);
			if(g == 0)
				console.write("Create group failed\n");
			else
				console.write(format("Group '%s' created\n"));
		} },

		{ "enter group", [&](const vector<string> &args) {
			auto g = board.enter_group(args[2]);
			if(g == 0)
				console.write("No such group\n");
		} },

		{ "post", [&](const vector<string> &args) { 
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

// Find first unread message in the thread rooted at msg_id
uint64_t ComBoard::find_first_unread(uint64_t msg_id) {
	if(!board.is_read(msg_id))
		return msg_id;
	//auto replies = get_replies(msg_id);
	for(const pair<uint64_t, MessageBoard::Message> &m : topicMap) {
		if(m.second.parent == msg_id) {
			auto mid = find_first_unread(m.second.id);
			if(mid > 0)
				return mid;
		}
	}
	return 0;
}

void ComBoard::select_topic(uint64_t topic_id) {
	//unordered_map<uint64_t, MessageBoard::Message> msgmap;
	topicMap.clear();
	const auto messages = board.list_messages(topic_id);
	for(const auto &m : messages) {
		topicMap[m.id] = m;
	}
}


void ComBoard::exec(const string &line) {
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
