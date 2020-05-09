// printing function for easier tinkercad port
// uncomment define below to switch to tinkercad serial display

// #define TINKERCAD

#ifdef TINKERCAD

#define UINT64_MAX  0xffffffffffffffff
#define UINT64_C(x)  ((x) + (UINT64_MAX - UINT64_MAX))

void ptr(int x) {
    Serial.print(x);
}

void ptr(char x) {
    Serial.print(x);
}

void ptr(const char * x) {
    Serial.print(x);
}

void ptrln() {
    Serial.println();
}
#else
#include <stdio.h>
#include <stdlib.h>

void ptr(int x) {
    printf("%d", x);

}

void ptr(char x) {
    printf("%c", x);
}

void ptr(const char * text) {
    printf(text);
}

void ptrln() {
    printf("\n");
}

int rand(int max) {
    return rand() % max;
}
#endif


#include <stdint.h>
#include <stdbool.h>

#define BOARD_SIZE                  8
#define BOARD_CELLS_COUNT          64
#define BOARD_SIZE_64     UINT64_C(8)
#define EMPTY_CELL                 -1
#define PASS               UINT64_MAX

#define INITIAL_BLACK_BB UINT64_C(0x810000000)
#define INITIAL_WHITE_BB UINT64_C(0x1008000000)
#define INITIAL_LEGAL_BB UINT64_C(0x102004080000)

#define RIGHT_MASK  9187201950435737471LL
#define LEFT_MASK -72340172838076674LL
#define UP_MASK -256LL
#define DOWN_MASK 72057594037927935LL

unsigned int countBits(uint64_t n)
{   
    unsigned int c; // c accumulates the total bits set in v
    for (c = 0; n; c++) 
        n &= n - UINT64_C(1); // clear the least significant bit set
    return c;
}   



void print_bitboard(uint64_t board)
{    
    for (int i = 0; i < BOARD_CELLS_COUNT; i++)
    {
        if ((board & (UINT64_C(1) << i)) != UINT64_C(0))
            ptr(" X ");
        else
            ptr(" - ");
        
        if (i % BOARD_SIZE == BOARD_SIZE - 1)
            ptrln();
    }
    
    ptrln();
}

// n'th non-zero least significant bit's index, -1 for not found
int nnlsi(uint64_t num, int n) {
    n -= num & UINT64_C(1);
    int i=0;
    while(n >= 0) {
        num >>= 1;
        i++;
        if(i > 64) { return -1; }
        n -= num & UINT64_C(1);
    }
    return i;
}

// first non-zero least significant index after index (inclusive), -1 for not found
int fnlsiai(uint64_t num, int start) {
    num >>= start;
    if(num == UINT64_C(0)) {
        return -1;
    }

    return __builtin_ctz(num) + start;
}


class Othello {    
public:
    int ply_count;
    bool game_over;
    uint64_t black;
    uint64_t white;
    uint64_t legal;

    Othello()
    {
        this->ply_count = 0;
        this->game_over = false;
        this->black = INITIAL_BLACK_BB;
        this->white = INITIAL_WHITE_BB;
        this->legal = INITIAL_LEGAL_BB;
    }



    int currentPlayer()
    {
        return (this->ply_count % 2) == 0;
    }

    void printBoard()
    {
        ptr("Ply: ");
        ptr(this->ply_count);
        ptrln();
        
        ptr("Black count: ");
        ptr((int)countBits(this->black));
        ptrln();

        ptr("White count: ");
        ptr((int)countBits(this->white));
        ptrln();

        ptr("Number of moves: ");
        ptr((int)countBits(this->legal));
        ptrln();

        ptr("Current player: ");
        ptr(currentPlayer());
        ptrln();
        
        for (int i = 0; i < BOARD_CELLS_COUNT; i++)
        {
            if ((this->black & (UINT64_C(1) << i)) != UINT64_C(0))
                ptr(" X ");
            else if ((this->white & (UINT64_C(1) << i)) != UINT64_C(0))
                ptr(" O ");
            else if ((this->legal & (UINT64_C(1) << i)) != UINT64_C(0))
                ptr(" P ");
            else
                ptr(" - ");
            
            if (i % BOARD_SIZE == BOARD_SIZE - 1)
                ptr("\n");
        }
        
        ptrln();
    }

    uint64_t getBoardColor(bool isBlack) {
        return isBlack ? this->black : this->white;
    }

    uint64_t getBoardInvColor(bool isBlack) {
        return getBoardColor(!isBlack);
    }

    uint64_t* getBoardColorPtr(bool isBlack) {
        return isBlack ? &this->black : &this->white;
    }

    uint64_t* getBoardInvColorPtr(bool isBlack) {
        return getBoardColorPtr(!isBlack);
    }

    // P0: free space
    uint64_t legalMaskP0(uint64_t mask) {
        return mask & (UINT64_MAX ^ (this->black | this->white));
    }

    // P1: adjason
    uint64_t legalMaskP1(uint64_t mask, bool isBlack) {
        uint64_t opponent_board = getBoardInvColor(isBlack);

        // prevent shift to other row/col
        uint64_t op = opponent_board;
        op |= (RIGHT_MASK & (op >> 1)) | (LEFT_MASK & (op << 1));
        op |= (op << BOARD_SIZE_64) | (op >> BOARD_SIZE_64);
        op &= UINT64_MAX ^ opponent_board;

        return mask & op;
    }

    uint64_t legalMaskP2(uint64_t mask, bool isBlack) {
        for (int i = 0; i <= 63; i++)
        {
            if((mask >> i) & UINT64_C(1)) {
                bool vaild = 
                    raycastDir(i, 1, RIGHT_MASK, isBlack)
                    || raycastDir(i, -1, LEFT_MASK, isBlack)
                    || raycastDir(i, BOARD_SIZE, DOWN_MASK, isBlack)
                    || raycastDir(i, -BOARD_SIZE, UP_MASK, isBlack)
                    || raycastDir(i, -BOARD_SIZE-1, LEFT_MASK, isBlack)
                    || raycastDir(i, BOARD_SIZE+1, RIGHT_MASK, isBlack)
                    || raycastDir(i, BOARD_SIZE-1, LEFT_MASK, isBlack)
                    || raycastDir(i, -BOARD_SIZE+1, RIGHT_MASK, isBlack);
                if(!vaild) {
                    mask &= UINT64_MAX ^ (UINT64_C(1) << i);
                }
            }
        }
        
        return mask;
    }

    // simulate ray-casting via bitshifting using shiftDir
    // return true if the ray meet p2 othello condition
    // otherwise return false
    bool raycastDir(int index, int shiftDir, uint64_t mask, bool isBlack) {
        uint64_t my_board = getBoardColor(isBlack);
        uint64_t thier_board = getBoardInvColor(isBlack);
        uint64_t bit = (UINT64_C(1) << index);

        bool isPassedThierBoard = false;
        do
        {
            
            if(shiftDir < 0) {
                bit <<= -shiftDir;
            }
            else {
                bit >>= shiftDir;
            }
            
            if(bit & my_board) {
                return isPassedThierBoard;
            }
            if(bit & thier_board) {
                isPassedThierBoard = true;
            }
            else
            {
                return false;
            }
        } while ((bit & mask) != 0);
        return false;
    }

    // like raycastDir, but replace enemy disc if raycastDir return true
    bool raycastDirReplace(int index, int shiftDir, uint64_t mask, bool isBlack) {
        uint64_t my_board = getBoardColor(isBlack);
        uint64_t thier_board = getBoardInvColor(isBlack);
        uint64_t bit = (UINT64_C(1) << index);

        uint64_t conversionMask = UINT64_C(0);
        
        bool raycastResult = [&](){
            bool isPassedThierBoard = false;
            do
            {
                
                if(shiftDir < 0) {
                    bit <<= -shiftDir;
                }
                else {
                    bit >>= shiftDir;
                }
                conversionMask |= bit;
                
                if(bit & my_board) {
                    return isPassedThierBoard;
                }
                if(bit & thier_board) {
                    isPassedThierBoard = true;
                }
                else
                {
                    return false;
                }
            } while ((bit & mask) != 0);
            return false;
        }();

        if(raycastResult) {
            /*
            printf("CM:\n");
            print_bitboard(conversionMask);*/
            *getBoardColorPtr(isBlack) |= conversionMask;
            *getBoardInvColorPtr(isBlack) &= (UINT64_MAX ^ conversionMask);
        }
    }

    

    uint64_t calculateLegalMask(bool isBlack) {
        uint64_t p0 = this->legalMaskP0(UINT64_MAX);
        // print_bitboard(p0);
        uint64_t p1 = this->legalMaskP1(p0, isBlack);
        // print_bitboard(p1);
        uint64_t p2 = this->legalMaskP2(p1, isBlack);
        // print_bitboard(p2);
        return p2;
    }



    // make move, assume legal index
    bool makeMoveUnchecked(int index) {
        bool currentPlayer = this->currentPlayer();
        
        uint64_t * myBoard = this->getBoardColorPtr(currentPlayer);
        uint64_t * thierBoard = this->getBoardColorPtr(currentPlayer);
        *myBoard |= (UINT64_C(1) << index);
        raycastDirReplace(index, 1, RIGHT_MASK, currentPlayer);
        raycastDirReplace(index, -1, LEFT_MASK, currentPlayer);
        raycastDirReplace(index, BOARD_SIZE, DOWN_MASK, currentPlayer);
        raycastDirReplace(index, -BOARD_SIZE, UP_MASK, currentPlayer);
        raycastDirReplace(index, -BOARD_SIZE-1, LEFT_MASK, currentPlayer);
        raycastDirReplace(index, BOARD_SIZE+1, DOWN_MASK, currentPlayer);
        raycastDirReplace(index, BOARD_SIZE-1, LEFT_MASK, currentPlayer);
        raycastDirReplace(index, -BOARD_SIZE+1, RIGHT_MASK, currentPlayer);

        this->legal = calculateLegalMask(!currentPlayer);

        ply_count++;
        return true;
    }

    bool makeMoveCheck(int index) {
        if((this->legal >> index) & UINT64_C(1)) {
            makeMoveUnchecked(index);
        }
        return false;
    }

    void makeMoveLegal(int legalIndex) {
        int pos = nnlsi(this->legal, legalIndex);
        if(pos >= 0) {
            makeMoveUnchecked(pos);
        }
    }
};

class AI
{
public:
    AI() {
    }

    int generateRandomMove(class Othello * othello) {
        // TODO: optimize, try to pull it directly from bitboard
        int randLegalIndex = rand(countBits(othello->legal));
        printf("legal: %d\n", randLegalIndex);
        return nnlsi(othello->legal, randLegalIndex);
    }
};




////
int main() {

    srand(789);


    Othello othello;
    AI ai;

    othello.printBoard();

    while(othello.legal) {
        othello.makeMoveUnchecked(ai.generateRandomMove(&othello));
        othello.printBoard();
    }
}