
class MessageBoard {
public:

	MessageBoard(sqlite3db::Database &db) : db(db) {}

	struct Group {
		Group(uint64_t id, const std::string &n, uint64_t lp = 0,  std::string c = "") : id(id), name(n), last_post(lp), creator(c) {}
		const uint64_t id;
		const std::string name;
		const uint64_t last_post;
		const  std::string creator;
	};

	struct Topic {
		Topic(uint64_t id, const std::string &n, const std::string &s = "", const std::string &l = "") : id(id), name(n), starter(s), lastPoster(l) {}
		const uint64_t id;
		const std::string name;
		const std::string starter;
		const std::string lastPoster;
	};

	struct Message {
		Message(uint64_t id, const std::string &t, const std::string &p = "") : id(id), text(t), poster(p) {}
		const uint64_t id;
		const std::string text;
		const std::string poster;
	};

	void login(uint64_t userid) {
		currentUser = userid;
	}

	uint64_t create_group(const std::string &name) {
		db.exec("INSERT INTO msggroup (name, creatorid, lastpost) VALUES (%Q, %d, 0)", name, currentUser);
		return db.last_rowid();
	}
	const std::vector<Group> list_groups() {
		vector<Group> groups;
		db.execf("SELECT msggroup.ROWID, msggroup.name, msggroup.lastpost, bbsuser.handle FROM msggroup,bbsuser WHERE msggroup.creatorid=bbsuser.ROWID", [&](int i, const vector<string> &result) {
			groups.emplace_back(std::stol(result[0]), result[1], std::stol(result[2]), result[3]);
		});
		return groups; // NOTE: std::move ?
	}

	uint64_t enter_group(const std::string &group_name) {
		uint64_t id = 0;
		uint64_t lastpost = 0;
		db.execf("SELECT ROWID,lastpost FROM msggroup WHERE name=%Q", [&](int i, const vector<string> &result) {
			id = std::stol(result[0]);
			lastpost = std::stol(result[1]);
		}, group_name);
		//auto id = db.select_first<pair<uint64_t, uint64_t>>("SELECT ROWID,lastpost FROM msggroup WHERE name=%Q", group_name);
		LOGD("Last post %d", lastpost);
		if(id != 0) {
			currentGroup = id;
			uint64_t lastread;
			bool unread = true;
			if(db.select_first(lastread, "SELECT lastread FROM groupstate WHERE userid=%d", currentUser)) {
				LOGD("Last read %d", lastread);
			}
		}
		return id;
	}

	const std::vector<Topic> list_topics() {
		vector<Topic> topics;
		LOGD("currentGroup %d", currentGroup);
		db.execf("SELECT msgtopic.ROWID,name,handle,lastpost,lastby FROM msgtopic,bbsuser WHERE groupid=%d AND creatorid=bbsuser.ROWID", [&](int i, const vector<string> &result) {
			LOGD(result[1]);
			topics.emplace_back(std::stol(result[0]), result[1], result[2]);
		}, currentGroup);
		return topics; // NOTE: std::move ?
	}

	const std::vector<Message> list_messages(uint64_t topic_id) {
		vector<Message> messages;
		db.execf("SELECT ROWID,contents,creatorid FROM msgtopic WHERE topicid=%d", [&](int i, const vector<string> &result) {
			LOGD(result[1]);
			messages.emplace_back(std::stol(result[0]), result[1]);
		}, topic_id);
		return messages; // NOTE: std::move ?		
	}

	uint64_t post(const std::string &topic_name, const std::string &text) {
		auto ta = db.transaction();
		db.exec("INSERT INTO msgtopic (name, creatorid,groupid) VALUES (%Q, %d, %d)", topic_name, currentUser, currentGroup);
		auto topicid = db.last_rowid();
		db.exec("INSERT INTO message (contents, creatorid, parentid, topicid) VALUES (%Q, %d, 0, %d)", text, currentUser, topicid);
		auto msgid = db.last_rowid();

		time_t t;
		time(&t);
		//long d = 12345;
		db.exec("UPDATE msggroup SET lastpost=%d,lastby=%d WHERE ROWID=%d", t, currentUser,currentGroup);
		db.exec("UPDATE msgtopic SET lastpost=%d,lastby=%d WHERE ROWID=%d", t, currentUser, topicid);
		db.exec("INSERT INTO readmsg (userid, msgid) VALUES (%d,%d)", currentUser, msgid);
		//db.exec("INSERT INTO groupstate (userid, groupid) VALUES (%d, %d)", currentUser, currentGroup);
		//db.exec("INSERT INTO topicstate (userid, topicid) VALUES (%d, %d)", currentUser, currentGroup);
		ta.commit();
		return msgid;
	}
	uint64_t reply(uint64_t msgid, const std::string &text) {
		uint64_t topicid;
		db.execf("SELECT topicid FROM message WHERE ROWID=%d", [&](int i, const vector<string> &result) {
			topicid = std::stol(result[0]);
		}, currentGroup);
		db.exec("INSERT INTO message (contents, userid, parentid, topicid) VALUES (%Q, %d, ^d, %d)", text, currentUser, msgid, topicid);
		return db.last_rowid();		
	}
private:
	uint64_t currentUser;
	uint64_t currentGroup;
	sqlite3db::Database &db;
};
