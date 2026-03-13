#include "IDManager.h"

int IDManager::currentID_ = 0;
std::unordered_map<int, std::string> IDManager::ipRecord_;

int IDManager::GetNewID(std::string whatever) {
	int outputID = currentID_;

	ipRecord_[outputID] = whatever;

	currentID_++;
	return outputID;
}


const std::string& IDManager::GetTag(int id) {
	static const std::string unknown = "UNKNOWN";
	auto it = ipRecord_.find(id);
	return (it != ipRecord_.end()) ? it->second : unknown;
}