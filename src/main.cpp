#include <iostream>
#include <filesystem>
#include <vector>
#include <string>
#include <map>
#include <chrono>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <memory>
#include <openssl/sha.h>

#include "vcs/include/GitInitializer.hpp"
#include "vcs/include/VersionControl.hpp"

// Enum pro příkazy
enum class Command
{
    Init,
    CatFile,
    HashObject,
    WriteTree,
    LsTree,
    Add,
    Commit,
    Log,
    Checkout,
    Unknown
};

// Pomocná funkce pro převod řetězce na enum
Command getCommand(const std::string &commandStr)
{
    static const std::map<std::string, Command> commandMap = {
        {"init", Command::Init},
        {"cat-file", Command::CatFile},
        {"hash-object", Command::HashObject},
        {"write-tree", Command::WriteTree},
        {"ls-tree", Command::LsTree},
        {"add", Command::Add},
        {"commit", Command::Commit},
        {"log", Command::Log},
        {"checkout", Command::Checkout}
    };

    auto it = commandMap.find(commandStr);
    if (it != commandMap.end())
    {
        return it->second;
    }
    return Command::Unknown;
}

// Hlavni main funkce se switch-case pro spuštění správného příkazu
int main(int argc, char **argv)
{
    if (argc < 2)
    {
        std::cerr << "Použití: " << argv[0] << " <příkaz> [parametry]" << std::endl;
        return 1;
    }

    std::string commandStr = argv[1];
    VersionControl vcs;

    switch (getCommand(commandStr))
    {
        case Command::Init:
        {
            GitInitializer initializer;
            initializer.initializeRepository();
            break;
        }
        case Command::CatFile:
        {
            if (argc < 4 || std::string(argv[2]) != "-p")
            {
                std::cerr << "Invalid arguments" << std::endl;
                return EXIT_FAILURE;
            }
            vcs.catFile(std::string_view(argv[3], 40));
            break;
        }
        case Command::HashObject:
        {
            if (argc > 4)
            {
                std::cerr << "Invalid arguments" << std::endl;
                return EXIT_FAILURE;
            }
            if (std::string(argv[2]) == "-w")
            {
                std::cout << "Writing object to database" << std::endl;
                vcs.hashObject(argv[3], true);
            }
            else
            {
                std::cout << "Reading object from database" << std::endl;
                vcs.hashObject(argv[2], false);
            }
            break;
        }
        case Command::WriteTree:
        {
            std::cout << "Writing tree to database" << std::endl;
            std::string hash = vcs.writeTree(".");
            if (!hash.empty())
            {
                std::cout << "Tree written successfully" << std::endl;
                std::cout << "Hash: " << hash << std::endl;
            }
            else
            {
                std::cerr << "Failed to write tree" << std::endl;
            }
            break;
        }
        case Command::LsTree:
        {
            std::cout << "Listing tree" << std::endl;
            if (argc < 3)
            {
                std::cerr << "Invalid arguments" << std::endl;
                return EXIT_FAILURE;
            }
            vcs.lsTree(argv[2]);
            break;
        }
        case Command::Add:
        {
            std::cout << "Add path" << std::endl;
            if (argc < 3)
            {
                std::cerr << "Invalid arguments" << std::endl;
                return EXIT_FAILURE;
            }
            vcs.add(argv[2]);
            break;
        }
        case Command::Commit:
        {
            if (argc < 4 || std::string(argv[2]) != "-m")
            {
                std::cerr << "Invalid arguments: use -m \"message\"" << std::endl;
                return EXIT_FAILURE;
            }
            vcs.commit(argv[3]);
            break;
        }
        case Command::Log:
        {
            vcs.log();
            break;
        }
        case Command::Checkout:
        {
            if (argc < 3)
            {
                std::cerr << "Invalid arguments: specify commit hash" << std::endl;
                return EXIT_FAILURE;
            }
            bool deleteFiles = false;
            std::string hash;
            if (argc == 3)
            {
                hash = argv[2];
            }
            else if (argc == 4 && std::string(argv[2]) == "-D")
            {
                deleteFiles = true;
                hash = argv[3];
            }
            else
            {
                std::cerr << "Invalid arguments for checkout" << std::endl;
                return EXIT_FAILURE;
            }
            vcs.checkout(hash, deleteFiles);
            break;
        }
        case Command::Unknown:
        default:
        {
            std::cerr << "Unknown command: " << commandStr << std::endl;
            return EXIT_FAILURE;
        }
    }
    return 0;
}