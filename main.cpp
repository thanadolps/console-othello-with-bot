// printing function for easier tinkercad port
// uncomment define below to switch to tinkercad serial display

// #define TINKERCAD

#ifdef TINKERCAD

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

#ifndef UINT64_MAX
    #define UINT64_MAX  0xffffffffffffffff
#endif
#ifndef UINT64_C
    #define UINT64_C(x)  ((x) + (UINT64_MAX - UINT64_MAX))
#endif


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

    return __builtin_ctzll(num) + start;
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
        
        char  count = 'a';
        for (int i = 0; i < BOARD_CELLS_COUNT; i++)
        {
            if ((this->black & (UINT64_C(1) << i)) != UINT64_C(0))
                ptr(" X ");
            else if ((this->white & (UINT64_C(1) << i)) != UINT64_C(0)) {
                ptr(" O ");
            }
            else if ((this->legal & (UINT64_C(1) << i)) != UINT64_C(0)) {
                ptr(" ");
                ptr(count);
                ptr(" ");
                count++;
            }
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
                    raycastDir(i, 1, LEFT_MASK, isBlack)
                    || raycastDir(i, -1, RIGHT_MASK, isBlack)
                    || raycastDir(i, BOARD_SIZE, UP_MASK, isBlack)
                    || raycastDir(i, -BOARD_SIZE, DOWN_MASK, isBlack)
                    || raycastDir(i, -BOARD_SIZE-1, RIGHT_MASK, isBlack)
                    || raycastDir(i, BOARD_SIZE+1, LEFT_MASK, isBlack)
                    || raycastDir(i, BOARD_SIZE-1, RIGHT_MASK, isBlack)
                    || raycastDir(i, -BOARD_SIZE+1, LEFT_MASK, isBlack);
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
        while(true)
        {
            
            if(shiftDir < 0) {
                bit >>= -shiftDir;
            }
            else {
                bit <<= shiftDir;
            }

            if((bit & mask) == 0) {
                return false;
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
        };
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
            while(true)
            {
                
                if(shiftDir < 0) {
                    bit >>= -shiftDir;
                }
                else {
                    bit <<= shiftDir;
                }

                if((bit & mask) == 0) {
                    return false;
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
            }
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
    void makeMoveUnchecked(int index) {
        bool currentPlayer = this->currentPlayer();
        
        uint64_t * myBoard = this->getBoardColorPtr(currentPlayer);
        uint64_t * thierBoard = this->getBoardColorPtr(currentPlayer);
        *myBoard |= (UINT64_C(1) << index);


        raycastDirReplace(index, 1, LEFT_MASK, currentPlayer);
        raycastDirReplace(index, -1, RIGHT_MASK, currentPlayer);
        raycastDirReplace(index, BOARD_SIZE, UP_MASK, currentPlayer);
        raycastDirReplace(index, -BOARD_SIZE, DOWN_MASK, currentPlayer);
        raycastDirReplace(index, -BOARD_SIZE-1, RIGHT_MASK, currentPlayer);
        raycastDirReplace(index, BOARD_SIZE+1, LEFT_MASK, currentPlayer);
        raycastDirReplace(index, BOARD_SIZE-1, RIGHT_MASK, currentPlayer);
        raycastDirReplace(index, -BOARD_SIZE+1, LEFT_MASK, currentPlayer);

        this->legal = calculateLegalMask(!currentPlayer);

        ply_count++;
    }

    bool makeMoveCheck(int index) {
        if((this->legal >> index) & UINT64_C(1)) {
            makeMoveUnchecked(index);
            return true;
        }
        return false;
    }

    bool makeMoveLegal(int legalIndex) {
        if(legalIndex < 0 || legalIndex > 64) {
            return false;
        }

        int pos = nnlsi(this->legal, legalIndex);
        if(pos >= 0) {
            makeMoveUnchecked(pos);
            return true;
        }
        else
        {
            return false;
        }
        
    }
};


float diffDivideOr0(float a, float b) {
    float sum = a+b;
    if(sum != 0) {
        return (a - b)/sum;
    }
    else {
        return 0;
    }
}

struct IndexScore {
    int index;
    float score;
};

class AI
{
public:
    AI() {
    }

    int generateRandomMove(class Othello * othello) {
        // TODO: optimize, try to pull it directly from bitboard
        int randLegalIndex = rand(countBits(othello->legal));
        // printf("legal: %d\n", randLegalIndex);
        return nnlsi(othello->legal, randLegalIndex);
    }

    float evaluationCoinParity(class Othello * othello) {
        int blackCount = __builtin_popcountll(othello->black);
        int whiteCount = __builtin_popcountll(othello->white);

        return diffDivideOr0(blackCount, whiteCount);
    }

    float evaluationMobility(class Othello * othello) {
        
        int whiteMobility, blackMobility;

        if(othello->currentPlayer()) {
            // BLACK
            blackMobility = countBits(othello->legal);
            whiteMobility = countBits(othello->calculateLegalMask(false));
        }
        else
        {
            // WHITE
            whiteMobility = countBits(othello->legal);
            blackMobility = countBits(othello->calculateLegalMask(true));
        }

        // printf("BM = %d, WM = %d\n", blackMobility, whiteMobility);

        return diffDivideOr0(blackMobility, whiteMobility);
    }

    float evaluationCorner(class Othello * othello) {

        uint64_t blackBoard = othello->black;
        int blackCorner = 
        (blackBoard & UINT64_C(1))
        + ((blackBoard >> (BOARD_SIZE-1)) & UINT64_C(1))
        + ((blackBoard >> (BOARD_SIZE*(BOARD_SIZE-1))) & UINT64_C(1))
        + ((blackBoard >> (BOARD_SIZE*BOARD_SIZE - 1)) & UINT64_C(1));

        uint64_t whiteBoard = othello->white;
        int whiteCorner = 
        (whiteBoard & UINT64_C(1))
        + ((whiteBoard >> (BOARD_SIZE-1)) & UINT64_C(1))
        + ((whiteBoard >> (BOARD_SIZE*(BOARD_SIZE-1))) & UINT64_C(1))
        + ((whiteBoard >> (BOARD_SIZE*BOARD_SIZE - 1)) & UINT64_C(1));

        // printf("BC: %d, WC: %d\n", blackCorner, whiteCorner);

        return diffDivideOr0(blackCorner, whiteCorner);
    }

    // random number between -1 and 1, to cause some variation
    float evalutionRandom() {
        return ((float) rand() / (RAND_MAX)) * 2 - 1;
    }

    // higher = black advantage, lower = white advantage
    float evaluation(class Othello * othello, int depth) {

        // should add up to 100
        const float coinFactor = 29.0;
        const float mobilityFactor = 30.0;
        const float cornerFactor = 40.0;
        const float randomFactor = 1.0; // this should be relativly small

        if(othello->legal) {

            float coin = evaluationCoinParity(othello);
            float mobility = evaluationMobility(othello);
            float corner = evaluationCorner(othello);
            float random = evalutionRandom();

            // printf("coi/m/cor = %2f/%2f/%2f\n", coin, mobility, corner);

            return 
                coinFactor * coin +
                mobilityFactor * mobility + 
                cornerFactor * corner +
                randomFactor * random;
        }
        else {
            int blackCount = __builtin_popcountll(othello->black);
            int whiteCount = __builtin_popcountll(othello->white);
            if(blackCount > whiteCount) {
                return 100 + depth;
            }
            else if(blackCount < whiteCount) {
                return -100 - depth;
            }
            else {
                return 0;
            }
        }
    }


    struct IndexScore negamaxPrelude(class Othello othello, int depth) {

        /*
        if(othello.legal == UINT64_C(0)) {
            
        }*/

        signed int color = othello.currentPlayer() ? 1 : -1;

        float bestScore = -200;
        int bestIndex = -1;

        float a = -200;
        float b = 200;

        int index = fnlsiai(othello.legal, 0);
        class Othello childernOthello;
        // printf("index=");
        while((index != -1) && (index <= 64)) {
            
            // printf("%d,",index);

            // (Copy)
            childernOthello = othello;
            childernOthello.makeMoveUnchecked(index);

            float score = -negamax(childernOthello, depth - 1, -b, -a, -color);


            if(score > bestScore) {
                bestScore = score;
                bestIndex = index;
            }

            if(score > a) { a = score; }
            if(a >= b) {
                break;
            }

            index = fnlsiai(othello.legal, index+1);
        }
        // printf("\n");
        // TODO: also return best score
        
        struct IndexScore out;
        out.index = bestIndex;
        out.score = bestScore;
        return out;
    }

    float negamax(class Othello othello, int depth, float a, float b, int color) {
        if((depth == 0) || (othello.legal == UINT64_C(0))) {
            return color * evaluation(&othello, depth);
        }


        float bestScore = -200;

        int index = fnlsiai(othello.legal, 0);
        class Othello childernOthello;
        
        while((index != -1) && (index <= 64)) {
            
            // (Copy)
            childernOthello = othello;
            childernOthello.makeMoveUnchecked(index);

            float score = -negamax(childernOthello, depth - 1, -b, -a, -color);


            if(score > bestScore) { bestScore = score; }

            if(score > a) { a = score; }
            if(a >= b) {
                break;
            }

            index = fnlsiai(othello.legal, index+1);
        }

        return bestScore;
    }
};






//// BELOW THIS IS NON-TINKERCAD
#include<time.h>
#include<ctype.h>
const int AI_LEVEL = 7;

void aiTurn(class Othello * othello, class AI * ai) {
    printf("\nAI TURN\n");
    
    struct IndexScore bestMove = ai->negamaxPrelude(*othello, AI_LEVEL);

    printf("\nmove = %d\n", bestMove.index);
    printf("EXPECTED SCORE = %f/100\n", othello->currentPlayer() ? bestMove.score : -bestMove.score);
    if(!othello->makeMoveCheck(bestMove.index)) {
        printf("\n%d\n", othello->legal >> 0);
        print_bitboard(othello->legal);
        exit(-1);
    }
}

void humanTurn(class Othello * othello) {
    printf("\nHUMAN TURN\n");
    othello->printBoard();
    // HUMAN
    char cindex = 'a';
    while(true) {
        printf("Input index (a->%c): ", countBits(othello->legal) + 'a' - 1);
        scanf(" %c", &cindex);
        int index = tolower(cindex) - 'a';
        if(othello->makeMoveLegal(index)) {
            othello->printBoard();
            break;
        }
        printf("Input Vaild please\n");
    }
}



int main() {

    // srand(789);

    printf("SIZE = %d\n", sizeof(Othello));
    
    Othello othello;
    AI ai;
    othello.printBoard();

    int seed;
    printf("Set seed (0=use time): ");
    scanf("%d", &seed);
    if(seed != 0) {
        srand(seed);
    }
    else {
        srand(time(0));
    }

    int choice;
    printf("Human, choose (0=white, 1=black, other=fuckyou): ");
    scanf("%d", &choice);
    switch (choice)
    {
    case 0:
        aiTurn(&othello, &ai);
        break;
    case 1:
        break;
    default:
        printf("FUCK YOU\n");
        exit(-1);
        break;
    }



    /*int index = ai.negamaxPrelude(othello);
    printf("IND: %d\n", index);*/
    
    while(true) {
        // othello.makeMoveUnchecked(ai.generateRandomMove(&othello));
        // othello.makeMoveLegal(0);
        
        // HUMAN
 
        humanTurn(&othello);
        if(othello.legal == UINT64_C(0)) {
            othello.printBoard();
            break;
        }

        // AI
        aiTurn(&othello, &ai);
        if(othello.legal == UINT64_C(0)) {
            othello.printBoard();
            break;
        }

        
    
    }

    return EXIT_SUCCESS;
}