#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define PERCENT_SCALE      100   /* rand() % 100 gives a 0-99 roll */
#define WALL_CHANCE_PCT     12   /* % of interior cells that become walls */
#define START_SAFE_DIST      5   /* keep x + y below this free of random walls */
#define TOTAL_GEMS           6   /* Total gems in the game */

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

    

/* PTR LESSON 3: out-parameters. We can't return two ints, so the caller
 * passes the addresses of its variables and we write through them. */
static void find_random_empty_cell(Game *game, const char *reachable, int min_x_plus_y, int *out_x, int *out_y)
{
    int candidate_x, candidate_y;
    
    do 
    {
        candidate_x = 1 + rand() % (game->width - 2);
        candidate_y = 1 + rand() % (game->height - 2);

    } while (*cell_at(game, candidate_x, candidate_y) != '.' || 
             *(reachable + (ptrdiff_t)candidate_y * game->width + candidate_x) == 0 || 
              candidate_x + candidate_y < min_x_plus_y);
    
    *out_x = candidate_x;
    *out_y = candidate_y;
}

/* PTR LESSON 4: pointer to the head pointer (Entity **).
 * We modify the caller's `list_head` variable itself, so we need its address. */
static Entity *spawn_entity(Entity **list_head, int x, int y, char symbol, AIFn ai_behavior)
{
    Entity *new_entity = malloc(sizeof *new_entity);
    
    if (!new_entity) exit(1);
    
    *new_entity = (Entity){ x, y, symbol, ai_behavior, NULL };
    new_entity->next = *list_head;   /* new node points at old first node */
    *list_head = new_entity;         /* head now points at new node */
    
    return new_entity;

}
// World Generation
static void generate_world(Game *game)
{
    size_t cell_count = (size_t)game->height * game->width;
    
    for(char *cell_ptr = game->map, *map_end = game->map + cell_count; cell_ptr < map_end; cell_ptr++ )
    {
        // loop goes from cell 1 to cell height *  width

        ptrdiff_t offset = cell_ptr - game->map;   /* pointer - pointer = element count */
        int16_t x = offset % game->width;
        int16_t y = offset / game->width;

        bool is_border = x == 0 || y == 0 || x == game->width - 1 || y == game->height - 1;
        bool is_near_start = x + y < START_SAFE_DIST;

        /* Placing walls and floor appropriately.
        The rules are that if the cell is a border put a '#' and if it a florr*/
        *cell_ptr = (is_border || (!is_near_start && rand() % PERCENT_SCALE < WALL_CHANCE_PCT)) ? '#' : '.';
    }
        
    // 
    char *reachable = calloc((size_t)(cell_count), sizeof(char *));
    
    if(!reachable) exit(EXIT_FAILURE);

    *(reachable + game->width + 1) = 1; // reachable[game->width + 1] = 1

    bool changed = true;
    do {
        changed = false;
        for (int16_t y = 1; y < game->height - 1; y++)  /* y = [1, game->height - 1) */
        {
            for (int16_t x = 1; x < game->width - 1; x++) /* x = [1, game->width - 1) */
            {
                int16_t cell_index = y * game->width + x;

                if (*(reachable + cell_index) || *(game->map + cell_index) == '#') continue;

                bool next_to_reachable = *(reachable + cell_index - 1) || 
                                         *(reachable + cell_index + 1) ||
                                         *(reachable + cell_index - game->width) || 
                                         *(reachable + cell_index + game->width);
                                    
                if (next_to_reachable) 
                {
                    *(reachable + cell_index) = 1;
                    changed = true;
                }
            }
        }
    } while (changed);

    game->gems_remaining = TOTAL_GEMS;
    for (int gem_number = 0; gem_number < game->gems_remaining; gem_number++) 
    {
        int gem_x, gem_y;
        
        find_random_empty_cell(game, reachable, 0, &gem_x, &gem_y);
        
        *cell_at(game, gem_x, gem_y) = '$';
    }

    free(reachable);

}

int main(void)
{
    printf("Hello2!\n");

    srand(42);   /* fixed seed: same dungeon every run, handy for debugging */

    printf("Size of Game Structure: %zu\n", sizeof(Game));
    printf("Size of Entity Structure: %zu\n", sizeof(Entity));
    printf("Size of ptrdiff_t: %zu\n", sizeof(ptrdiff_t));
    printf("Size of size_t: %zu\n", sizeof(size_t));

    
    

    Game game_state = { 0 };
    Game *game = &game_state;
    game->width = 30;
    game->height = 12;
    game->health = 5;
    game->map = malloc((size_t)game->width * game->height);
    if (!game->map) return 1;

    generate_world(game);

    Entity *standalone_list_head = NULL;

    game->player = spawn_entity(&standalone_list_head, 1, 1, '@', NULL);

    
    return 0;
}