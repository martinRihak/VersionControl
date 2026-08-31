#include "include/GitInitializer.hpp"
#include <iostream>
#include <fstream>
// Inicializuje repozitář vytvořením potřebných adresářů a souborů
// Volá jednotlivé pomocné metody pro vytvoření .git adresářové struktury
bool GitInitializer::initializeRepository(const std::string &path)
{
    basePath = std::filesystem::path(path);

    try
    {
        if (!createGitDirectory())
            return false;
        if (!createObjectsDirectory())
            return false;
        if (!createRefsDirectory())
            return false;
        if (!createMainBranchRef())
            return false; // Nová funkcia
        if (!createHeadFile())
            return false;

        std::cout << "Initialized git directory\n";
        return true;
    }
    catch (const std::filesystem::filesystem_error &e)
    {
        std::cerr << e.what() << '\n';
        return false;
    }
}
// Vytvoří adresář .git v dané cestě (repoPath)
bool GitInitializer::createGitDirectory()
{
    return std::filesystem::create_directory(basePath / ".git");
}
// Vytvoří adresář .git/objects a 256 podadresářů pro hash prefixy (00 až ff)
bool GitInitializer::createObjectsDirectory()
{
    return std::filesystem::create_directory(basePath / ".git/objects");
}
// Vytvoří adresář .git/refs/heads pro uložení referencí na větve
bool GitInitializer::createRefsDirectory()
{
    return std::filesystem::create_directory(basePath / ".git/refs");
}
// Vytvoří prázdný soubor .git/refs/heads/main, který bude reprezentovat hlavní větev
bool GitInitializer::createMainBranchRef()
{
    // Vytvorenie podadresára refs/heads
    std::filesystem::create_directory(basePath / ".git/refs/heads");
    // Vytvorenie súboru refs/heads/main
    std::ofstream mainRefFile(basePath / ".git/refs/heads/main");
    if (mainRefFile.is_open())
    {
        // Môžeme nechať prázdne, pretože prvý commit ho prepíše
        mainRefFile.close();
        return true;
    }
    std::cerr << "Failed to create .git/refs/heads/main file.\n";
    return false;
}
// Vytvoří soubor .git/HEAD a nastaví ho na referenci k hlavní větvi
bool GitInitializer::createHeadFile()
{
    std::ofstream headFile(basePath / ".git/HEAD");
    if (headFile.is_open())
    {
        headFile << "ref: refs/heads/main\n";
        headFile.close();
        return true;
    }
    std::cerr << "Failed to create .git/HEAD file.\n";
    return false;
}