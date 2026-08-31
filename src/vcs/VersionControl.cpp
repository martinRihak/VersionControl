#include "include/VersionControl.hpp"
// Pomocná funkce pro rozdělení řetězce podle zadaného oddělovače
// Vrací vektor obsahující jednotlivé části rozděleného řetězce
std::vector<std::string> split(const std::string &str, char delimiter)
{
    std::vector<std::string> tokens;
    std::string token;
    std::stringstream ss(str);
    while (std::getline(ss, token, delimiter))
    {
        tokens.push_back(token);
    }
    return tokens;
}

// Konstruktor třídy VersionControl
VersionControl::VersionControl() {}

// Inicializuje nový Git repozitář v zadané cestě
// Vytvoří potřebnou adresářovou strukturu pomocí GitInitializer
// Vrací true pokud inicializace proběhla úspěšně
bool VersionControl::init(const std::string &path)
{
    repoPath = path;
    try
    {
        GitInitializer initializer;
        initializer.initializeRepository(path);
    }
    catch (const std::exception &e)
    {
        std::cout << e.what() << '\n';
        return false;
    }
    return true;
}

// Zobrazí obsah objektu (blob) podle zadaného hash identifikátoru
// Dekomprimuje objekt a zobrazí jeho obsah
// Vrací true pokud byl objekt nalezen a úspěšně zobrazen
bool VersionControl::catFile(const std::string_view &hash)
{
    if (!isValidRepo())
        return false;

    const auto blobDir = hash.substr(0, 2);
    const auto blobName = hash.substr(2);
    const auto blobPath = std::filesystem::path(repoPath) / ".git" / "objects" / blobDir / blobName;

    auto in = std::ifstream(blobPath, std::ios::binary);
    if (!in.is_open())
    {
        std::cout << "Failed to open file: " << blobPath << std::endl;
        return false;
    }

    auto blobData = std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
    auto buf = std::string();
    buf.resize(blobData.size());

    while (true)
    {
        uLongf len = buf.size();
        if (auto res = uncompress((uint8_t *)buf.data(), &len, (const uint8_t *)blobData.data(), blobData.size()); res == Z_BUF_ERROR)
        {
            buf.resize(buf.size() * 2);
        }
        else if (res != Z_OK)
        {
            std::cout << "Failed to uncompress Zlib. (code: " << res << ")\n";
            return false;
        }
        else
        {
            buf.resize(len);
            break;
        }
    }
    size_t nullPos = buf.find('\0');
    if (nullPos != std::string::npos)
    {
        std::cout << buf.substr(nullPos + 1) << std::endl;
    }
    else
    {
        std::cout << buf << std::endl;
    }

    return true;
}

// Vytvoří hash objektu ze zadaného souboru
// Pokud je parametr write=true, uloží objekt do Git repozitáře
// Vrací SHA-1 hash vytvořeného objektu
std::string VersionControl::hashObject(const std::string &filePath, bool write)
{
    if (!isValidRepo())
    {
        throw std::runtime_error("Could not open file: " + filePath);
    }

    if (!std::filesystem::exists(filePath))
    {
        throw std::runtime_error("File not found: " + filePath);
    }

    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open())
    {
        throw std::runtime_error("Failed to open file: " + filePath);
    }

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    std::string header = "blob " + std::to_string(content.size()) + std::string(1, '\0');
    std::string canonicalForm = header + content;

    unsigned char hash[20];
    SHA1((unsigned char *)canonicalForm.c_str(), canonicalForm.size(), hash);

    std::stringstream ss;
    for (int i = 0; i < 20; i++)
    {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    std::string hashStr = ss.str();

    std::cout << hashStr << std::endl;
    if (write)
    {
        std::string blobDir = hashStr.substr(0, 2);
        std::string blobName = hashStr.substr(2);
        std::filesystem::path blobPath = std::filesystem::path(repoPath) / ".git" / "objects" / blobDir / blobName;
        std::cout << "Blob path: " << blobPath << std::endl;
        std::filesystem::create_directories(blobPath.parent_path());

        std::ofstream outFile(blobPath, std::ios::binary);
        if (!outFile.is_open())
        {
            throw std::runtime_error("Failed to create blob file");
        }

        uLongf compressedSize = compressBound(canonicalForm.size());
        std::string compressed;
        compressed.resize(compressedSize);

        int result = compress((Bytef *)compressed.data(), &compressedSize,
                              (const Bytef *)canonicalForm.c_str(), canonicalForm.size());

        if (result != Z_OK)
        {
            throw std::runtime_error("Compression hash failed with code: " + std::to_string(result));
        }
        compressed.resize(compressedSize);
        outFile.write(compressed.c_str(), compressed.size());
        outFile.close();
    }
    return hashStr;
}

// Zobrazí obsah tree objektu podle zadaného hash identifikátoru
// Dekomprimuje tree objekt a vypíše jména souborů/adresářů, které obsahuje
// Vrací true pokud byl tree objekt nalezen a úspěšně zobrazen
bool VersionControl::lsTree(const std::string &hash)
{
    if (!isValidRepo())
    {
        std::cerr << "Not a valid repository" << std::endl;
        return false;
    }

    const auto blobDir = hash.substr(0, 2);
    const auto blobName = hash.substr(2);
    const auto blobPath = std::filesystem::path(repoPath) / ".git" / "objects" / blobDir / blobName;

    auto in = std::ifstream(blobPath, std::ios::binary);
    if (!in.is_open())
    {
        std::cout << "Failed to open file: " << blobPath << std::endl;
        return false;
    }

    const auto treeData = std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
    std::string buffer;
    buffer.resize(treeData.size());

    while (true)
    {
        uLongf len = buffer.size();
        uLongf compressedSize = treeData.size();
        if (auto res = uncompress2((uint8_t *)buffer.data(), &len, (const uint8_t *)treeData.data(), &compressedSize); res == Z_BUF_ERROR)
        {
            buffer.resize(buffer.size() * 2);
        }
        else if (res != Z_OK)
        {
            std::cout << "Failed to uncompress Zlib. (code: " << res << ")\n";
            return false;
        }
        else
        {
            buffer.resize(len);
            break;
        }
    }

    size_t nullPos = buffer.find('\0');
    if (nullPos != std::string::npos)
    {
        buffer = buffer.substr(nullPos + 1);
    }

    std::vector<std::string> entries;
    size_t pos = 0;

    while (pos < buffer.size())
    {
        size_t nullPos = buffer.find('\0', pos);
        if (nullPos == std::string::npos || nullPos + 20 >= buffer.size())
            break;

        std::string entry = buffer.substr(pos, nullPos - pos);

        std::string hexHash;
        for (int i = 0; i < 20; i++)
        {
            unsigned char byte = static_cast<unsigned char>(buffer[nullPos + 1 + i]);
            char hex[3];
            std::snprintf(hex, sizeof(hex), "%02x", byte);
            hexHash += hex;
        }

        entry += ":" + hexHash;
        entries.push_back(entry);

        pos = nullPos + 21;
    }

    for (const auto &entry : entries)
    {
        std::vector<std::string> parts = split(entry, ' ');
        if (parts.size() >= 2)
        {
            std::cout << parts[1] << std::endl;
        }
    }
    return true;
}

// Vytvoří tree objekt ze zadaného adresáře a jeho obsahu
// Rekurzivně prochází adresářovou strukturu a vytváří tree objekty pro každý podadresář
// Vrací SHA-1 hash vytvořeného tree objektu
std::string VersionControl::writeTree(const std::string &path)
{
    if (!isValidRepo())
    {
        throw std::runtime_error("Not a valid repository");
    }

    std::vector<std::pair<std::string, std::string>> entries;
    std::string mode;
    std::string sha1;

    for (const auto &entry : std::filesystem::directory_iterator(path))
    {
        std::string name = entry.path().filename().string();
        if (name == ".git")
            continue;
        if (entry.is_directory())
        {
            mode = "40000";
            sha1 = writeTree(entry.path().string());
        }
        else if (entry.is_regular_file())
        {
            mode = "100644";
            sha1 = hashObject(entry.path().string(), true);
        }
        if (!sha1.empty())
        {
            std::string binaryHash;
            for (size_t i = 0; i < sha1.size(); i += 2)
            {
                std::string byte = sha1.substr(i, 2);
                char chr = static_cast<char>(std::stoi(byte, nullptr, 16));
                binaryHash += chr;
            }
            std::string entry = mode + " " + name + '\0' + binaryHash;
            std::cout << "Entry: " << entry << std::endl;
            entries.push_back({name, entry});
        }
    }
    std::sort(entries.begin(), entries.end());
    std::string treeContent;
    for (const auto &entry : entries)
    {
        treeContent += entry.second;
    }
    std::string header = "tree " + std::to_string(treeContent.size()) + std::string(1, '\0');
    std::string canonicalForm = header + treeContent;
    unsigned char hash[20];
    SHA1((unsigned char *)canonicalForm.c_str(), canonicalForm.size(), hash);
    std::stringstream ss;
    for (int i = 0; i < 20; i++)
    {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    std::string hashStr = ss.str();

    std::string blobDir = hashStr.substr(0, 2);
    std::string blobName = hashStr.substr(2);
    std::filesystem::path blobPath = std::filesystem::path(repoPath) / ".git" / "objects" / blobDir / blobName;
    std::filesystem::create_directories(blobPath.parent_path());
    std::ofstream outFile(blobPath, std::ios::binary);
    if (!outFile.is_open())
    {
        throw std::runtime_error("Failed to create blob file");
    }

    uLongf compressedSize = compressBound(canonicalForm.size());
    std::string compressed;
    compressed.resize(compressedSize);

    int result = compress((Bytef *)compressed.data(), &compressedSize, (const Bytef *)canonicalForm.c_str(), canonicalForm.size());
    if (result != Z_OK)
    {
        throw std::runtime_error("Compression failed with code: " + std::to_string(result));
    }

    compressed.resize(compressedSize);
    outFile.write(compressed.c_str(), compressed.size());
    outFile.close();

    return hashStr;
}

// Načte index soubor (.git/index) do paměti
// Index obsahuje informace o souborech ve staging area
void VersionControl::loadIndex()
{
    std::filesystem::path indexPath = std::filesystem::path(repoPath) / ".git" / "index";
    indexCache.clear();
    std::cout << "Loading index from: " << indexPath << std::endl;
    if (!std::filesystem::exists(indexPath))
    {
        std::cout << "Index file does not exist, creating empty index" << std::endl;
        return;
    }
    std::ifstream indexFile(indexPath);
    if (!indexFile.is_open())
    {
        throw std::runtime_error("Failed to open index file");
    }
    std::string line;
    while (std::getline(indexFile, line))
    {
        std::vector<std::string> parts = split(line, ' ');
        if (parts.size() >= 3)
        {
            std::string mode = parts[0];
            std::string hash = parts[1];
            std::string path = parts[2];
            indexCache[path] = {mode, hash};
            std::cout << "Loaded: " << mode << " " << hash << " " << path << std::endl;
        }
    }
    indexFile.close();
}

// Uloží obsah indexu (staging area) do souboru .git/index
void VersionControl::saveIndex()
{
    std::filesystem::path indexPath = std::filesystem::path(repoPath) / ".git" / "index";
    std::cout << "Saving index to: " << indexPath << std::endl;
    std::filesystem::create_directories(indexPath.parent_path());
    std::ofstream indexFile(indexPath);
    if (!indexFile.is_open())
    {
        throw std::runtime_error("Failed to create index file");
    }
    for (const auto &entry : indexCache)
    {
        indexFile << entry.second.first << " " << entry.second.second << " " << entry.first << "\n";
        std::cout << "Saved: " << entry.second.first << " " << entry.second.second << " " << entry.first << std::endl;
    }
    indexFile.close();
}

// Přidá soubor nebo adresář do staging area (index)
// Pokud je zadána cesta ".", přidá všechny soubory v aktuálním adresáři
void VersionControl::add(const std::string &path)
{
    loadIndex();
    std::cout << "Adding path: " << path << std::endl;

    if (path == "." || std::filesystem::is_directory(path))
    {
        std::filesystem::path basePath = path == "." ? std::filesystem::current_path() : std::filesystem::path(path);
        for (const auto &entry : std::filesystem::recursive_directory_iterator(basePath))
        {
            if (entry.is_regular_file())
            {
                std::string filePath = entry.path().string();
                // Převod cesty na relativní k aktuálnímu adresáři
                std::filesystem::path relativePath = std::filesystem::relative(filePath, basePath);
                std::string relativeStr = relativePath.string();

                // Ignorování .git a skrytých souborů (volitelné, jako v Gitu)
                if (relativeStr.find(".git") != std::string::npos || relativeStr[0] == '.')
                    continue;

                std::string hash = hashObject(filePath, true);
                indexCache[relativeStr] = {"100644", hash};
                std::cout << "Added: " << relativeStr << " with hash " << hash << std::endl;
            }
        }
    }
    else if (std::filesystem::is_regular_file(path))
    {
        std::string hash = hashObject(path, true);
        std::filesystem::path relativePath = std::filesystem::relative(path, std::filesystem::current_path());
        std::string relativeStr = relativePath.string();
        indexCache[relativeStr] = {"100644", hash};
        std::cout << "Added: " << relativeStr << " with hash " << hash << std::endl;
    }
    else
    {
        throw std::runtime_error("Invalid path: " + path);
    }

    saveIndex();
}

// Získá aktuální čas ve formátu používaném pro Git commity
// Vrací čas jako řetězec ve formátu Unix timestamp a časová zóna
std::string VersionControl::getCurrentTime()
{
    time_t now = time(0);
    struct tm tstruct;
    char buf[80];
    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "%s %z", &tstruct);
    return std::string(buf);
}

// Získá hash commitu, na který aktuálně ukazuje HEAD
// Vrací hash aktuálního commitu nebo prázdný řetězec, pokud žádný commit neexistuje
std::string VersionControl::getHeadCommitHash()
{
    std::filesystem::path headPath = std::filesystem::path(repoPath) / ".git" / "HEAD";
    std::ifstream headFile(headPath);
    if (!headFile.is_open())
    {
        throw std::runtime_error("Failed to open .git/HEAD");
    }
    std::string headContent;
    std::getline(headFile, headContent);
    headFile.close();

    if (headContent.find("ref: ") == 0)
    {
        std::string refPath = headContent.substr(5);
        std::filesystem::path refFilePath = std::filesystem::path(repoPath) / ".git" / refPath;
        if (std::filesystem::exists(refFilePath))
        {
            std::ifstream refFile(refFilePath);
            if (!refFile.is_open())
            {
                throw std::runtime_error("Failed to open ref file: " + refFilePath.string());
            }
            std::string commitHash;
            std::getline(refFile, commitHash);
            refFile.close();
            return commitHash;
        }
    }
    return headContent;
}

// Vytvoří tree objekt z aktuálního obsahu indexu (staging area)
// Vrací hash vytvořeného tree objektu
std::string VersionControl::writeTreeFromIndex()
{
    loadIndex();
    std::cout << "Index size: " << indexCache.size() << std::endl;
    for (const auto &entry : indexCache)
    {
        std::cout << "Index entry: " << entry.second.first << " " << entry.second.second << " " << entry.first << std::endl;
    }
    if (indexCache.empty())
    {
        throw std::runtime_error("Index is empty");
    }

    std::map<std::string, std::vector<std::tuple<std::string, std::string, std::string>>> dirEntries;

    for (const auto &item : indexCache)
    {
        std::string path = item.first;
        std::string mode = item.second.first;
        std::string hash = item.second.second;
        std::filesystem::path fsPath(path);
        std::string parentDir = fsPath.parent_path().string();
        if (parentDir.empty())
        {
            parentDir = ".";
        }
        dirEntries[parentDir].emplace_back(mode, fsPath.filename().string(), hash);
    }

    std::function<std::string(const std::string &)> createTree;
    createTree = [&](const std::string &dirPath) -> std::string
    {
        std::vector<std::pair<std::string, std::string>> entries;
        std::string mode;
        std::string sha1;

        auto it = dirEntries.find(dirPath);
        if (it != dirEntries.end())
        {
            for (const auto &[entryMode, name, hash] : it->second)
            {
                if (entryMode == "100644")
                {
                    mode = "100644";
                    sha1 = hash;
                }
                else
                {
                    continue;
                }
                std::string binaryHash;
                for (size_t i = 0; i < sha1.size(); i += 2)
                {
                    std::string byte = sha1.substr(i, 2);
                    char chr = static_cast<char>(std::stoi(byte, nullptr, 16));
                    binaryHash += chr;
                }
                std::string entry = mode + " " + name + '\0' + binaryHash;
                entries.push_back({name, entry});
            }
        }

        for (const auto &[subDir, _] : dirEntries)
        {
            if (std::filesystem::path(subDir).parent_path().string() == dirPath)
            {
                std::string name = std::filesystem::path(subDir).filename().string();
                mode = "40000";
                sha1 = createTree(subDir);
                if (!sha1.empty())
                {
                    std::string binaryHash;
                    for (size_t i = 0; i < sha1.size(); i += 2)
                    {
                        std::string byte = sha1.substr(i, 2);
                        char chr = static_cast<char>(std::stoi(byte, nullptr, 16));
                        binaryHash += chr;
                    }
                    std::string entry = mode + " " + name + '\0' + binaryHash;
                    entries.push_back({name, entry});
                }
            }
        }

        if (entries.empty())
        {
            return "";
        }

        std::sort(entries.begin(), entries.end());
        std::string treeContent;
        for (const auto &entry : entries)
        {
            treeContent += entry.second;
        }
        std::string header = "tree " + std::to_string(treeContent.size()) + std::string(1, '\0');
        std::string canonicalForm = header + treeContent;
        unsigned char hash[20];
        SHA1((unsigned char *)canonicalForm.c_str(), canonicalForm.size(), hash);
        std::stringstream ss;
        for (int i = 0; i < 20; i++)
        {
            ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
        }
        std::string hashStr = ss.str();

        std::string blobDir = hashStr.substr(0, 2);
        std::string blobName = hashStr.substr(2);
        std::filesystem::path blobPath = std::filesystem::path(repoPath) / ".git" / "objects" / blobDir / blobName;
        std::filesystem::create_directories(blobPath.parent_path());
        std::ofstream outFile(blobPath, std::ios::binary);
        if (!outFile.is_open())
        {
            throw std::runtime_error("Failed to create blob file");
        }

        uLongf compressedSize = compressBound(canonicalForm.size());
        std::string compressed;
        compressed.resize(compressedSize);

        int result = compress((Bytef *)compressed.data(), &compressedSize, (const Bytef *)canonicalForm.c_str(), canonicalForm.size());
        if (result != Z_OK)
        {
            throw std::runtime_error("Compression failed with code: " + std::to_string(result));
        }

        compressed.resize(compressedSize);
        outFile.write(compressed.c_str(), compressed.size());
        outFile.close();

        return hashStr;
    };

    return createTree(".");
}

// Vytvoří nový commit se zadanou zprávou
// Commit bude obsahovat tree objekt vytvořený z aktuálního indexu
// a bude ukazovat na předchozí commit (parent)
void VersionControl::commit(const std::string &message)
{
    if (!isValidRepo())
    {
        throw std::runtime_error("Not a valid repository");
    }

    std::cout << "Attempting commit with message: " << message << std::endl;
    std::string treeHash = writeTreeFromIndex();
    std::string parentHash = getHeadCommitHash();

    std::string commitContent = "tree " + treeHash + "\n";
    if (!parentHash.empty())
    {
        commitContent += "parent " + parentHash + "\n";
    }
    std::string timestamp = getCurrentTime();
    commitContent += "author User <rih0075@vsb.cz> " + timestamp + "\n";
    commitContent += "committer User <rih0075@vsb.cz> " + timestamp + "\n";
    commitContent += "\n" + message + "\n";

    std::string header = "commit " + std::to_string(commitContent.size()) + std::string(1, '\0');
    std::string canonicalForm = header + commitContent;

    unsigned char hash[20];
    SHA1((unsigned char *)canonicalForm.c_str(), canonicalForm.size(), hash);

    std::stringstream ss;
    for (int i = 0; i < 20; i++)
    {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    std::string commitHash = ss.str();

    std::string blobDir = commitHash.substr(0, 2);
    std::string blobName = commitHash.substr(2);
    std::filesystem::path blobPath = std::filesystem::path(repoPath) / ".git" / "objects" / blobDir / blobName;
    std::filesystem::create_directories(blobPath.parent_path());
    std::ofstream outFile(blobPath, std::ios::binary);
    if (!outFile.is_open())
    {
        throw std::runtime_error("Failed to create commit file");
    }

    uLongf compressedSize = compressBound(canonicalForm.size());
    std::string compressed;
    compressed.resize(compressedSize);

    int result = compress((Bytef *)compressed.data(), &compressedSize, (const Bytef *)canonicalForm.c_str(), canonicalForm.size());
    if (result != Z_OK)
    {
        throw std::runtime_error("Compression failed with code: " + std::to_string(result));
    }

    compressed.resize(compressedSize);
    outFile.write(compressed.c_str(), compressed.size());
    outFile.close();

    std::filesystem::path headPath = std::filesystem::path(repoPath) / ".git" / "HEAD";
    std::ifstream headFile(headPath);
    if (!headFile.is_open())
    {
        throw std::runtime_error("Failed to open .git/HEAD");
    }
    std::string headContent;
    std::getline(headFile, headContent);
    headFile.close();

    if (headContent.find("ref: ") == 0)
    {
        std::string refPath = headContent.substr(5);
        std::filesystem::path refFilePath = std::filesystem::path(repoPath) / ".git" / refPath;
        std::ofstream refFile(refFilePath);
        if (!refFile.is_open())
        {
            throw std::runtime_error("Failed to open ref file: " + refFilePath.string());
        }
        refFile << commitHash << "\n";
        refFile.close();
    }
    else
    {
        throw std::runtime_error("HEAD is not a ref");
    }

    std::cout << "Commit created: " << commitHash << std::endl;
}

bool VersionControl::isValidRepo() const
{
    std::filesystem::path currentPath = std::filesystem::current_path();
    return std::filesystem::exists(currentPath / ".git");
}

std::string VersionControl::getRepoPath() const
{
    return repoPath;
}

std::string VersionControl::readObject(const std::string &hash)
{
    const auto blobDir = hash.substr(0, 2);
    const auto blobName = hash.substr(2);
    const auto blobPath = std::filesystem::path(repoPath) / ".git" / "objects" / blobDir / blobName;

    std::ifstream in(blobPath, std::ios::binary);
    if (!in.is_open())
    {
        throw std::runtime_error("Failed to open object file: " + blobPath.string());
    }

    std::string blobData((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    std::string buf;
    buf.resize(blobData.size());

    while (true)
    {
        uLongf len = buf.size();
        int res = uncompress((uint8_t *)buf.data(), &len, (const uint8_t *)blobData.data(), blobData.size());
        if (res == Z_BUF_ERROR)
        {
            buf.resize(buf.size() * 2);
        }
        else if (res != Z_OK)
        {
            std::cerr << "Decompression failed with code: " << res << " for object: " << hash << std::endl;
            throw std::runtime_error("Failed to uncompress object: " + hash);
        }
        else
        {
            buf.resize(len);
            break;
        }
    }

    return buf;
}

void VersionControl::log()
{
    std::string currentHash = getHeadCommitHash();
    if (currentHash.empty())
    {
        std::cout << "No commits yet" << std::endl;
        return;
    }

    while (!currentHash.empty())
    {
        try
        {
            std::string content = readObject(currentHash);
            size_t nullPos = content.find('\0');
            if (nullPos == std::string::npos)
            {
                std::cerr << "Invalid commit object format: " << currentHash << std::endl;
                break;
            }
            std::string commitData = content.substr(nullPos + 1);
            std::stringstream ss(commitData);
            std::string line;
            std::string treeHash;
            std::vector<std::string> parents;
            std::string author;
            std::string committer;
            std::string message;
            bool inMessage = false;

            while (std::getline(ss, line))
            {
                if (line.empty())
                {
                    inMessage = true;
                    continue;
                }
                if (inMessage)
                {
                    message += line + "\n";
                }
                else
                {
                    if (line.substr(0, 5) == "tree ")
                    {
                        treeHash = line.substr(5);
                    }
                    else if (line.substr(0, 7) == "parent ")
                    {
                        parents.push_back(line.substr(7));
                    }
                    else if (line.substr(0, 7) == "author ")
                    {
                        author = line.substr(7);
                    }
                    else if (line.substr(0, 10) == "committer ")
                    {
                        committer = line.substr(10);
                    }
                }
            }

            std::cout << "commit " << currentHash << std::endl;
            std::cout << "Author: " << author << std::endl;
            std::vector<std::string> parts;
            std::istringstream ssAuthor(author);
            std::string tok;
            while (ssAuthor >> tok)
                parts.push_back(tok);
            if (parts.size() >= 3)
            {
                std::string timestampStr = parts[2];
                time_t timestamp = std::stoll(timestampStr);
                std::tm *tm = std::localtime(&timestamp);
                char dateBuf[80];
                std::strftime(dateBuf, sizeof(dateBuf), "%a %b %d %H:%M:%S %Y %z", tm);
                std::cout << "Date:   " << dateBuf << std::endl;
            }
            std::cout << "\n    " << message << std::endl;

            if (!parents.empty())
            {
                currentHash = parents[0];
            }
            else
            {
                currentHash = "";
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error reading commit: " << e.what() << std::endl;
            break;
        }
    }
}

std::string VersionControl::binToHex(const std::string &bin)
{
    std::stringstream ss;
    for (unsigned char c : bin)
    {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)c;
    }
    return ss.str();
}

void VersionControl::collectTreeEntries(const std::string &treeHash, const std::string &path,
                                        std::set<std::string> &files,
                                        std::map<std::string, std::pair<std::string, std::string>> &entries)
{
    std::string treeContent = readObject(treeHash);
    size_t headerEnd = treeContent.find('\0');
    if (headerEnd == std::string::npos)
    {
        throw std::runtime_error("Invalid tree object: no null terminator in header");
    }
    size_t pos = headerEnd + 1;
    while (pos < treeContent.size())
    {
        size_t spacePos = treeContent.find(' ', pos);
        if (spacePos == std::string::npos)
            break;
        std::string mode = treeContent.substr(pos, spacePos - pos);
        pos = spacePos + 1;
        size_t nullPos = treeContent.find('\0', pos);
        if (nullPos == std::string::npos)
            break;
        std::string name = treeContent.substr(pos, nullPos - pos);
        pos = nullPos + 1;
        if (pos + 20 > treeContent.size())
        {
            throw std::runtime_error("Insufficient data for hash in tree object");
        }
        std::string hash = treeContent.substr(pos, 20);
        pos += 20;
        std::string hexHash = binToHex(hash);
        std::string fullPath = path == "." ? name : path + "/" + name;

        if (mode == "40000")
        {
            collectTreeEntries(hexHash, fullPath, files, entries);
        }
        else
        {
            files.insert(fullPath);
            entries[fullPath] = {mode, hexHash};
        }
    }
}

/**
 * Obnoví obsah stromu objektů na zadané cestě.
 * @param treeHash Hash stromu, který má být obnoven
 * @param path Cesta, kam má být strom obnoven
 */
void VersionControl::checkoutTree(const std::string &treeHash, const std::string &path)
{
    // Načtení obsahu stromu podle hashe
    std::string treeContent = readObject(treeHash);

    // Nalezení konce hlavičky objektu (první null byte)
    size_t headerEnd = treeContent.find('\0');
    if (headerEnd == std::string::npos)
    {
        throw std::runtime_error("Invalid tree object: no null terminator in header");
    }

    // Zahájení procházení položek stromu od pozice za hlavičkou
    size_t pos = headerEnd + 1;
    while (pos < treeContent.size())
    {
        // Načtení módu souboru (oprávnění)
        size_t spacePos = treeContent.find(' ', pos);
        if (spacePos == std::string::npos)
            break;
        std::string mode = treeContent.substr(pos, spacePos - pos);
        pos = spacePos + 1;

        // Načtení názvu souboru nebo adresáře
        size_t nullPos = treeContent.find('\0', pos);
        if (nullPos == std::string::npos)
            break;
        std::string name = treeContent.substr(pos, nullPos - pos);
        pos = nullPos + 1;

        // Kontrola, zda je dostatek dat pro hash
        if (pos + 20 > treeContent.size())
        {
            throw std::runtime_error("Insufficient data for hash in tree object");
        }

        // Získání hashe objektu v binární formě a převod na hexadecimální formát
        std::string hash = treeContent.substr(pos, 20);
        pos += 20;
        std::string hexHash = binToHex(hash);

        // Sestavení úplné cesty pro aktuální položku
        std::string fullPath = path == "." ? name : path + "/" + name;

        // Zpracování podle typu objektu (adresář nebo soubor)
        if (mode == "40000")
        {
            // Pro adresáře vytvoříme nový adresář a rekurzivně zavoláme checkoutTree
            std::filesystem::create_directory(fullPath);
            checkoutTree(hexHash, fullPath);
        }
        else
        {
            // Pro soubory načteme obsah blob objektu a uložíme jej do souboru
            std::string content = readObject(hexHash);
            size_t contentNullPos = content.find('\0');
            if (contentNullPos != std::string::npos)
            {
                // Extrakce dat blobu (za null bytem)
                std::string blobData = content.substr(contentNullPos + 1);

                // Zápis obsahu do souboru
                std::ofstream outFile(fullPath, std::ios::binary);
                if (outFile.is_open())
                {
                    outFile.write(blobData.c_str(), blobData.size());
                    outFile.close();
                }
                else
                {
                    throw std::runtime_error("Failed to open file for writing: " + fullPath);
                }
            }
            else
            {
                throw std::runtime_error("Invalid blob object: no null terminator");
            }
        }
    }
}
void VersionControl::checkout(const std::string &commitHash, bool deleteFiles)
{
    std::string commitContent = readObject(commitHash);
    size_t nullPos = commitContent.find('\0');
    if (nullPos == std::string::npos)
    {
        throw std::runtime_error("Invalid commit object");
    }
    std::string data = commitContent.substr(nullPos + 1);
    std::stringstream ss(data);
    std::string line;
    std::string treeHash;
    while (std::getline(ss, line))
    {
        if (line.substr(0, 5) == "tree ")
        {
            treeHash = line.substr(5);
            break;
        }
    }
    if (treeHash.empty())
    {
        throw std::runtime_error("No tree found in commit");
    }

    // Krok 1: Pokud je deleteFiles true, vyčistit pracovní adresář kromě .git
    if (deleteFiles)
    {
        std::cout << "Deleting all files in working directory except .git" << std::endl;
        for (const auto &entry : std::filesystem::directory_iterator("."))
        {
            std::string name = entry.path().filename().string();
            if (name != ".git")
            {
                std::filesystem::remove_all(entry.path());
            }
        }
    }
    else
    {
        // Krok 2: Načíst aktuální index pro identifikaci sledovaných souborů
        loadIndex();
        std::set<std::string> currentFiles;
        for (const auto &entry : indexCache)
        {
            currentFiles.insert(entry.first);
        }

        // Krok 3: Shromáždit soubory ve stromu cílového commitu
        std::set<std::string> targetFiles;
        std::map<std::string, std::pair<std::string, std::string>> targetTreeEntries;
        collectTreeEntries(treeHash, ".", targetFiles, targetTreeEntries);

        // Krok 4: Aktualizovat pouze nezbytné soubory, zachovat nesledované
        for (const auto &file : currentFiles)
        {
            auto targetIt = targetFiles.find(file);
            if (targetIt == targetFiles.end())
            {
                std::cout << "File " << file << " not in target commit, skipping removal to preserve untracked state" << std::endl;
            }
            else
            {
                auto targetEntry = targetTreeEntries[file];
                auto currentEntry = indexCache[file];
                if (currentEntry.second != targetEntry.second)
                {
                    std::cout << "Updating file with different content: " << file << std::endl;
                    std::filesystem::path filePath(file);
                    if (std::filesystem::exists(filePath))
                    {
                        std::filesystem::remove(filePath);
                    }
                }
            }
        }
    }

    // Krok 5: Obnovit nebo aktualizovat soubory ze stromu cílového commitu
    checkoutTree(treeHash, ".");

    // Krok 6: Aktualizovat index, aby odpovídal cílovému commitu
    indexCache.clear();
    add(".");

    // Krok 7: Aktualizovat HEAD na odpojený stav
    std::filesystem::path headPath = std::filesystem::path(repoPath) / ".git" / "HEAD";
    std::ofstream headOut(headPath);
    if (!headOut.is_open())
    {
        throw std::runtime_error("Failed to write to .git/HEAD");
    }
    headOut << commitHash << "\n";
    headOut.close();
}