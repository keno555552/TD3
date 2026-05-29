#pragma once
#include <cstdint>
#include <map>
#include <vector>
#include <string>
#include "MathsIncluder.h"
#include "Joint.h"

struct Skeleton {
    int32_t rootID{};                        // RootJointのIndex
    std::map<std::string, int32_t> jointMap; // Joint名とIndexとの辞書
    std::vector<Joint> jointList;            // 所属しているジョイント
};