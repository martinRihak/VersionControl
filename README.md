# VersionControl

Jednoduchý systém pro správu verzí (klon Gitu) napsaný v C++20 s využitím knihoven **OpenSSL** (SHA-1 hashování) a **zlib** (komprese objektů).

Podrobnější technický popis fungování a nízkoúrovňových detailů najdete v souboru [ARCHITECTURE_AND_INTERNALS.md](ARCHITECTURE_AND_INTERNALS.md).

---

## 🛠 Požadavky a sestavení

### Závislosti (Ubuntu/Debian)
```bash
sudo apt update
sudo apt install -y build-essential cmake libssl-dev zlib1g-dev
```

### Kompilace
```bash
mkdir build && cd build
cmake ..
make -j
```
Spustitelný soubor `VersionControl` vznikne v kořenovém adresáři projektu.

---

## 🚀 Přehled příkazů

Spouštění příkazů:
```bash
./VersionControl <příkaz> [parametry]
```

| Příkaz | Popis | Příklad použití |
| :--- | :--- | :--- |
| `init` | Inicializuje nový repozitář (vytvoří adresář `.git`). | `./VersionControl init` |
| `add <cesta>` | Přidá soubor nebo celý adresář (`.`) do staging area (indexu). | `./VersionControl add file.txt`<br>`./VersionControl add .` |
| `commit -m <zpráva>` | Vytvoří nový commit z obsahu indexu s danou zprávou. | `./VersionControl commit -m "Initial commit"` |
| `log` | Vypíše historii commitů (hash, autor, datum, zpráva). | `./VersionControl log` |
| `checkout [-D] <hash>` | Přepne stav repozitáře na daný commit.<br>• bez `-D`: zachová nesledované soubory<br>• s `-D`: smaže vše kromě `.git` a obnoví stav | `./VersionControl checkout <hash>`<br>`./VersionControl checkout -D <hash>` |
| `cat-file -p <hash>` | Vypíše dekomprimovaný obsah objektu (blob, tree, commit). | `./VersionControl cat-file -p <hash>` |
| `hash-object [-w] <soubor>` | Spočítá SHA-1 hash souboru. S `-w` jej navíc uloží do `.git/objects`. | `./VersionControl hash-object -w file.txt` |
| `ls-tree <hash>` | Vypíše obsah stromového objektu (soubory a složky). | `./VersionControl ls-tree <hash>` |
| `write-tree` | Zapíše aktuální stav pracovního adresáře do databáze jako strom. | `./VersionControl write-tree` |

---

## 📖 Rychlá ukázka použití

```bash
# 1. Inicializace repozitáře
./VersionControl init

# 2. Vytvoření souboru a přidání do indexu
echo "Ahoj svete" > hello.txt
./VersionControl add hello.txt

# 3. Vytvoření prvního commitu
./VersionControl commit -m "Prvni commit"

# 4. Zobrazení historie
./VersionControl log

# 5. Úprava a druhý commit
echo "Dalsi radek" >> hello.txt
./VersionControl add hello.txt
./VersionControl commit -m "Druhy commit"

# 6. Přepnutí na předchozí verzi
./VersionControl checkout <hash_prvniho_commitu>
```

---

## 📁 Struktura projektu

- `src/main.cpp` – Vstupní bod programu, zpracování příkazů přes `switch-case`.
- `src/vcs/VersionControl.cpp` / `VersionControl.hpp` – Implementace verzovacích operací (blob, tree, commit, index, checkout).
- `src/vcs/GitInitializer.cpp` / `GitInitializer.hpp` – Inicializace adresářové struktury `.git`.
- `ARCHITECTURE_AND_INTERNALS.md` – Podrobná technická dokumentace interního fungování.