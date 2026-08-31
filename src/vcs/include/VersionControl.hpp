#pragma once
#include <iostream>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <zlib.h>
#include <openssl/sha.h>
#include <iomanip>
#include <vector>
#include <map>
#include <algorithm>
#include <functional>
#include <ctime>
#include <iostream>
#include <set>
#include "GitInitializer.hpp"
class VersionControl
{
private:
    std::string repoPath;
    std::map<std::string, std::pair<std::string, std::string>> indexCache; // cesta -> (mode, hash)
    void loadIndex();
    void saveIndex();
    std::string getCurrentTime();
    std::string getHeadCommitHash();
    std::string writeTreeFromIndex();

    std::string readObject(const std::string &hash);

    void checkoutTree(const std::string &treeHash, const std::string &path);
    std::string binToHex(const std::string &bin);

    void collectTreeEntries(const std::string &treeHash, const std::string &path,
                            std::set<std::string> &files,
                            std::map<std::string, std::pair<std::string, std::string>> &entries);

public:
    VersionControl();
    ~VersionControl() = default;

    // Inicializace repozitáře
    bool init(const std::string &path);

    // Příkazy pro práci se soubory
    bool catFile(const std::string_view &hash);
    std::string hashObject(const std::string &filePath, bool write);
    bool lsTree(const std::string &hash);
    std::string writeTree(const std::string &path);

    void add(const std::string &path);
    void commit(const std::string &message);

    void log();

    // Mazani
    void checkout(const std::string &commitHash,bool deleteFiles);
    // Pomocné metody
    bool isValidRepo() const;
    std::string getRepoPath() const;
};