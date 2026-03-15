#include "JsonManager.h"
#include "Logger.h"
#include <filesystem>
#include <fstream>

std::optional<nlohmann::json> JsonManager::Load(const std::string &filePath) {
  // ファイルの存在チェック
  if (!Exists(filePath)) {
    Logger::Log("[JsonManager] File not found: %s", filePath.c_str());
    return std::nullopt;
  }

  // ファイルを開く
  std::ifstream file(filePath);
  if (!file.is_open()) {
    Logger::Log("[JsonManager] Failed to open file: %s", filePath.c_str());
    return std::nullopt;
  }

  // JSON としてパース
  try {
    nlohmann::json data = nlohmann::json::parse(file);
    return data;
  } catch (const nlohmann::json::parse_error &e) {
    Logger::Log("[JsonManager] Parse error in %s: %s", filePath.c_str(),
                e.what());
    return std::nullopt;
  }
}

bool JsonManager::Save(const std::string &filePath,
                       const nlohmann::json &data) {
  // 保存先のディレクトリが存在しなければ作成
  std::filesystem::path path(filePath);
  std::filesystem::path parentDir = path.parent_path();

  if (!parentDir.empty() && !std::filesystem::exists(parentDir)) {
    try {
      std::filesystem::create_directories(parentDir);
    } catch (const std::filesystem::filesystem_error &e) {
      Logger::Log("[JsonManager] Failed to create directory: %s", e.what());
      return false;
    }
  }

  // ファイルに書き出し（インデント 2 スペース）
  std::ofstream file(filePath);
  if (!file.is_open()) {
    Logger::Log("[JsonManager] Failed to create file: %s", filePath.c_str());
    return false;
  }

  try {
    file << data.dump(2);
  } catch (const nlohmann::json::type_error &e) {
    Logger::Log("[JsonManager] Write error for %s: %s", filePath.c_str(),
                e.what());
    return false;
  }

  return true;
}

std::vector<std::string>
JsonManager::GetFileList(const std::string &directoryPath) {
  std::vector<std::string> fileList;

  // ディレクトリの存在チェック
  if (!std::filesystem::exists(directoryPath) ||
      !std::filesystem::is_directory(directoryPath)) {
    Logger::Log("[JsonManager] Directory not found: %s", directoryPath.c_str());
    return fileList;
  }

  // サブフォルダも含めて再帰的に .json ファイルを探索
  for (const auto &entry :
       std::filesystem::recursive_directory_iterator(directoryPath)) {
    if (entry.is_regular_file() && entry.path().extension() == ".json") {
      fileList.push_back(entry.path().string());
    }
  }

  return fileList;
}

bool JsonManager::Exists(const std::string &filePath) {
  return std::filesystem::exists(filePath);
}
