#pragma once
#include <vector>
#include <string>
#include <unordered_map>



class IDManager
{
public:
	static int GetNewID(std::string whatever);

	static const std::string& GetTag (int id);

private:
	static int currentID_;

	static std::unordered_map<int, std::string> ipRecord_;
};