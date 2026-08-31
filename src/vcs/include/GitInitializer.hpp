#pragma once

#include <string>
#include <filesystem>

class GitInitializer {
private:
    std::filesystem::path basePath;
public:
    GitInitializer() = default;
    ~GitInitializer() = default;

    bool initializeRepository(const std::string& path = ".");
    bool createGitDirectory();
    bool createObjectsDirectory();
    bool createRefsDirectory();
    bool createHeadFile();
    bool createMainBranchRef();


};