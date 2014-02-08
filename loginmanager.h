class LoginManager {
public:
	struct User {
		User(uint64_t id = 0, const std::string &handle = "") : id(id), handle(handle) {}
		const uint64_t id;
		const std::string handle;
	};

	static const uint64_t NO_USER = 0;

	LoginManager() {}

	uint64_t verify_user(const std::string &handle, const std::string &password) {
		auto s = sha256(handle + "\t" + password);
		uint64_t id = NO_USER;
		db.execf("SELECT ROWID FROM bbsuser WHERE sha=%Q;", [&](int i, const vector<string> &result) {
			id = std::stol(result[0]);
		}, s);
		return id;
	}

	uint64_t get_user(const std::string &handle) {
		uint64_t id = NO_USER;
		db.execf("SELECT ROWID FROM bbsuser WHERE handle=%Q;", [&](int i, const vector<string> &result) {
			id = std::stol(result[0]);
		}, handle);
		return id;
	}

	uint64_t add_user(const std::string &handle, const std::string &password) {
		auto sha = sha256(handle + "\t" + password);
		db.exec("INSERT INTO bbsuser (sha, handle) VALUES (%Q, %Q)", sha, handle);
		return db.last_rowid();
	}
};

