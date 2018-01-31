#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string>
#include <netdb.h>
#include <errno.h>
#include <err.h>
#include <vector>
#include <iostream>
#include <random>


#define SOCKET_ERROR    -1
#define INVALID_SOCKET  -1

typedef int         SOCKET;
typedef sockaddr    SOCKADDR;
typedef sockaddr_in SOCKADDR_IN;

#define closesocket close

#ifdef __APPLE__

#define MSG_NOSIGNAL 0

#endif

class Ship {
    u_short length;
    u_short hits;

public:
    std::string name;

    Ship(u_short length, std::string name): length(length), name(name), hits(0){}

    bool hit() {
        hits++;
        bool ded = hits == length;
        return ded;
    }

    bool ded() {
        return hits == length;
    }

    char identifier() {
        return name[0];
    }

    short len() {
        return length;
    }

    friend std::ostream &operator<<(std::ostream &os, const Ship &s){
        os << s.name << ": " << s.length;
        return os;
    }
};

typedef struct PlayerShip {
    Ship *ship;
    bool available;

    PlayerShip(std::string name, u_short length){
        ship = new Ship(length, name);
        available = true;
    }
} PlayerShip;

class ShipList {
    std::vector<PlayerShip> ships;
    short availableShips = 5;

public:
    ShipList() {
        ships.push_back(PlayerShip{"Aircraft carrier", 5});
        ships.push_back(PlayerShip{"Battleship", 4});
        ships.push_back(PlayerShip{"Cruiser", 3});
        ships.push_back(PlayerShip{"Destroyer", 3});
        ships.push_back(PlayerShip{"Scout", 2});
    }

    ~ShipList(){
        for(PlayerShip &s: ships){
            delete s.ship;
        }
    }

    bool useShip(u_short index){
        if (!testShip(index)){
            return false;
        } else {
            availableShips -= 1;
            ships[index - 1].available = false;
            return true;
        }
    }

    bool testShip(u_short index){
        return index <= ships.size() && index > 0 && ships[index - 1].available;
    }
    
    std::vector<PlayerShip> getShips() {
        return ships;
    }

    bool available() {
        return availableShips > 0;
    }

    bool ended() {
        for (PlayerShip &ship: ships){
            if (!ship.ship->ded()){
                return false;
            }
        }
        return true;
    }

    friend std::ostream &operator<<(std::ostream &os, const ShipList &sl){
        for (int i = 0; i < sl.ships.size(); i++) {
            os << (i + 1) << "): " << *sl.ships[i].ship << (sl.ships[i].available ? "": " (used)") << std::endl;

        }
        return os;
    }
};

class Cell {
    Ship * ship;
    bool checked;

public:
    bool friendly;

    Cell(): ship(NULL), checked(false) {}
    Cell(Ship *ship, bool &friendly): ship(ship), checked(false), friendly(friendly) {
    }

    int attack() {
        if (checked) {
            std::cout << "That square has already been attacked. Please try again" << std::endl;
            return -1;
        } else if (ship != NULL){
            checked = true;
            bool ded = ship->hit();
            std::cout << "You " << (ded ? "sunk": "hit") << " the enemy " << ship->name << std::endl;
            return (ded ? 2: 1);
        } else {
            std::cout << "You missed" << std::endl;
            checked = true;
            return 0;
        }
    }

    friend std::ostream &operator<<(std::ostream &os, const Cell &c)
    {
        if (c.friendly) {
            if (c.ship == NULL) {
                os << (c.checked ? ".  | ": "   | ");
            } else {
                os << c.ship->identifier() << (c.checked ? "*": " ") << " | ";
            }
        } else {
            if (c.checked) {
                os << (c.ship != NULL ? "X ": "- ") << " | ";
            } else {
                os << "   | ";
            }
        }
        return os;
    }

    bool empty(){
        return ship == NULL;
    }

    void assignShip(Ship *ship) {
        this->ship = ship;
    }
};

class Board {
    std::vector<Cell> board;
    bool friendly;
    u_short size;

public:
    Board(bool friendly, u_short &size): friendly(friendly), size(size) {
        for (u_int8_t i = 0; i < size; i++) {
            for (u_int8_t j = 0; j < size; j++) {
                board.push_back(Cell{NULL, friendly});
            }
        }
    }

    friend std::ostream &operator<<(std::ostream &os, const Board &b){
        std::string newline = "   ";
        for (u_int8_t i = 0; i < b.size; i++){
            newline.append("-----");
        }
        newline.append("-");
        std::cout << "     ";
        for(int i = 0; i < b.size; i++){
            std::cout << (i + 1) << "    ";
        }
        std::cout << std::endl;
        for (int i = 0; i < b.size; i++) {
            os << (i + 1) << ") | ";
            for (u_int8_t j = 0; j < b.size; j++) {
                os << b.board[i * b.size + j];
            }
            os << std::endl << newline << std::endl;
        }
        return os;
    }

    short hit(u_short row, u_short column){
        u_int8_t index = row * size + column;
        if (index >= board.size()){
            std::cout << "That is an invalid index" << std::endl;
            return -2;
        } else {
            return board[index].attack();
        }
    }

    bool isValid(u_short row, u_short column){
        u_int8_t index = row * size + column;
        return index < board.size() && board[index].empty();
    }

    bool isValid(u_int8_t index){
        return index < board.size() && board[index].empty();
    }

    //1: up; 2: right, 3: down, 4: left
    bool placeShip(Ship *ship, u_short row, u_short column, u_short direction){
        u_short newRow = row - 1, newCol = column - 1;
        if (direction < 1 || direction > 4){
            return false;
        }
        for (u_int8_t i = 0; i < ship->len(); i++) {
            if (!isValid(newRow, newCol)){
                return false;
            }
            if (direction % 2 == 1){
                newRow = (direction == 1 ? -1: 1) + newRow;
            } else {
                newCol = (direction == 2 ? 1: -1) + newCol;
            }
        }
        if (isValid(newRow, newCol)){
            newRow = row - 1;
            newCol = column - 1;
            for (u_int8_t i = 0; i < ship->len(); i++) {
                u_int8_t index = newRow * size + newCol;
                board[index].assignShip(ship);
                if (direction % 2 == 1){
                    newRow = (direction == 1 ? -1: 1) + newRow;
                } else {
                    newCol = (direction == 2 ? 1: -1) + newCol;
                }
            }
            return true;
        } else {
            return false;
        }
    }

    void changeFriend(bool status){
        this->friendly = status;
        for (Cell &cell: board){
            cell.friendly = status;
        }
    }
};

class Game {
    Board *enemyBoard;
    ShipList *enemyShips;

public:
    Game(u_short size) {
        enemyBoard = new Board(false, size);
        enemyShips = new ShipList();
        int index = 1;
        std::mt19937_64 rng;
        rng.seed(std::random_device()());
        std::uniform_int_distribution<std::mt19937_64::result_type> dist(1,size);
        while (enemyShips->available()) {
            u_short row = dist(rng), col = dist(rng);
            Ship *ship = enemyShips->getShips()[index - 1].ship;
            std::vector<u_short> dirs;
            if (row - (ship->len() - 1) >= 1) {
                dirs.push_back(1);
            } if (row + (ship->len() - 1) <= size){
                dirs.push_back(3);
            } if (col + (ship->len() - 1) <= size){
                dirs.push_back(2);
            } if (col - (ship->len() - 1) >= 1){
                dirs.push_back(4);
            }
            std::uniform_int_distribution<std::mt19937_64::result_type> distS(0, dirs.size() - 1);
            u_short dir = dirs[distS(rng)];
            if (enemyBoard->placeShip(ship, row, col, dir)){
                enemyShips->useShip(index);
                index++;
            }
        }
    }

    void play(int chances){
        while (chances > 0 && !enemyShips->ended()) {
            u_short row, col;
            std::cout << "Current board:      You have (" << chances << ") chance" << (chances == 1 ? "": "s") << " left" << std::endl << *enemyBoard << std::endl;
            std::cout << "Row: ";
            std::cin >> row;
            std::cout << "Column: ";
            std::cin >> col;
            short res = enemyBoard->hit(row - 1, col - 1);
            if (res >= 0){
                chances--;
            }
        }
        if (chances > 0){
            std::cout << "You win with " << chances << " chances left" << std::endl;
        } else {
            std::cout << "You lost" << std::endl;
            enemyBoard->changeFriend(true);
            std::cout << *enemyBoard;

        }
    }

    ~Game() {
        delete enemyBoard;
        delete enemyShips;
    }

    friend std::ostream &operator<<(std::ostream &os, const Game &g){
        os << "Enemy board" << std::endl;
        os << *g.enemyBoard << std::endl;
        g.enemyBoard->changeFriend(false);
        os << *g.enemyBoard << std::endl;
        return os;
    }
};


int main()
{
    short size = 0;
    do {
        std::cout << "Please enter your board size (> 7): ";
        std::cin >> size;
    } while (size < 7);
    Game *game = new Game(size);
    int chances = 0;
    do {
        std::cout << "How many chances would you like? (17 <= x <= " << (size * size) << ")" << std::endl;
        std::cin >> chances;
    } while (chances < 0 || chances > size * size);
    game->play(chances);
    delete game;
    return 0;
}

