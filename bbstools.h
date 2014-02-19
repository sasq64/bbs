#include <bbsutils/console.h>
#include <string>
#include <vector>

int menu(bbs::Console &console, const std::vector<std::pair<char, std::string>> &entries);
void showPetscii(bbs::Console &console, const std::string &name);