#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

typedef struct Game Game;
typedef struct Entity Entity;

// Utilizing Function pointers
typedef void (*AIFn)(Game *, Entity *);

struct Entity {
    char symbol; 
    int16_t x, y; 
    AIFn ai_behavior;   /* pointer to function */
    Entity *next;       /* pointer to next node: singly linked list */

    /*
    Memory layout of Entity structure:

    [symbol][x][y][ai_behavior][next] 
    ^

    */
};


struct Game {
    int16_t width, height;  
    int16_t gems_remaining;
    int16_t health; 
    int16_t turn_count; 
    char *map;  /*  block of width*height chars, treated as a 2D grid */
    Entity *player;  
    Entity *enemies;        /* head of linked list */
    
    /*
    Memory Layout of Game structure:
    
    [width][height][gems_remaining][health][turn_count][map][player][enemies]
    ^                      
    
    */ 
    
};

/* PTR LESSON 2: 2D -> 1D with pointer arithmetic.
 * `map + y*width + x` is a char* pointing at cell (x,y). Adding an int to a
 * pointer moves by that many ELEMENTS (here 1 byte each), not bytes. */
static char *cell_at(Game *game, int x, int y)
{
    return game->map + (ptrdiff_t)y * game->width + x;
}
/* ---- World generation ---- */

static void generate_world(Game *game)
{
    size_t cell_count = (size_t)game->height * game->width;
    
    for(char *cell_ptr = game->map, *map_end = game->map + cell_count; cell_ptr < map_end; cell_ptr++ )
    {
        // loop goes from cell 1 to cell height *  width

        ptrdiff_t offset = cell_ptr - game->map;   /* pointer - pointer = element count */
        int16_t x = (int)(offset % game->width);
        int16_t y = (int)(offset / game->width);

        bool is_border = x == 0 || y == 0 || x == game->width - 1 || y == game->height - 1;
        bool is_near_start = x + y < 5;

        *cell_ptr = (is_border || (!is_near_start && rand() % 100 < 12)) ? '#' : '.';
        
    }


}

int main(void)
{
    printf("Hello2!\n");

    srand(42);   /* fixed seed: same dungeon every run, handy for debugging */

    printf("Size of Game Structure: %zu\n", sizeof(Game));
    printf("Size of Entity Structure: %zu\n", sizeof(Entity));
    

    Game game_state = { 0 };
    Game *game = &game_state;
    game->width = 30;
    game->height = 12;
    game->health = 5;
    game->map = malloc((size_t)game->width * game->height);
    if (!game->map) return 1;

    generate_world(game);

    
    return 0;
}