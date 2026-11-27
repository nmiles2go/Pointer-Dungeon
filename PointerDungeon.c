#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define PERCENT_SCALE      100   /* rand() % 100 gives a 0-99 roll */
#define WALL_CHANCE_PCT     12   /* % of interior cells that become walls */
#define START_SAFE_DIST      5   /* keep x + y below this free of random walls */
#define TOTAL_GEMS           6   /* Total gems in the game. */
#define NUM_ENEMIES          3   
#define NUM_DIRECTIONS       4   /* UP, DOWN, RIGHT, LEFT. No Diagonal movement. */
#define MIN_CHASE_DISTANCE   4   /* Minimum distance the player should be from the Protector for the chase to start. */

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
static char *cell_at(Game *game, int16_t x, int16_t y)
{
    return game->map + (ptrdiff_t)y * game->width + x;
}

static int16_t sign_of(int16_t value) 
{ 
    return (value > 0) - (value < 0); 
}

/* Moving Entities and checking whether the movement is valid.*/
static bool try_move(Game *game, Entity *mover, int16_t delta_x, int16_t delta_y)
{
    int16_t new_x = delta_x + mover->x;
    int16_t new_y = delta_y + mover->y;

    if(*cell_at(game, new_x, new_y) == '#') return false;

    if(mover != game->player && new_x == game->player->x && new_y == game->player->y)
    {
        --game->health;
        return false;
    }

    mover->x = new_x;
    mover->y = new_y;

    return true;
}

static void ai_chase(Game *game, Entity *self)
{
    int16_t delta_x = sign_of(game->player->x - self->x);
    int16_t delta_y = sign_of(game->player->y - self->y);

    bool needs_horizontal = delta_x != 0;
    bool needs_vertical   = delta_y != 0;

    if (needs_horizontal && try_move(game, self, delta_x, 0)) return;

    if (needs_vertical)
    {
        try_move(game, self, 0, delta_y);
    } 
}

static void ai_wander(Game *game, Entity *self)
{
    int16_t direction = rand() % NUM_DIRECTIONS;
    
    int16_t delta_x = (direction == 0) - (direction == 1);
    int16_t delta_y = (direction == 2) - (direction == 3);

    try_move(game, self, delta_x, delta_y);
}

static void ai_protect(Game *game, Entity *self)
{
    int16_t distance = abs(game->player->x - self->x) + abs(game->player->y - self->y);
    
    if(distance < MIN_CHASE_DISTANCE)
    {
        ai_chase(game, self);
    }
}




/* PTR LESSON 3: out-parameters. We can't return two ints, so the caller
 * passes the addresses of its variables and we write through them. */
static void find_random_empty_cell(Game *game, const char *reachable, int16_t min_x_plus_y, int16_t *out_x, int16_t *out_y)
{
    int16_t candidate_x, candidate_y;
    
    do 
    {
        candidate_x = 1 + rand() % (game->width - 2);
        candidate_y = 1 + rand() % (game->height - 2);

    } while (*cell_at(game, candidate_x, candidate_y) != '.'                        || 
             *(reachable + (ptrdiff_t)candidate_y * game->width + candidate_x) == 0 || 
              candidate_x + candidate_y < min_x_plus_y);
    
    *out_x = candidate_x;
    *out_y = candidate_y;
}

/* PTR LESSON 4: pointer to the head pointer (Entity **).
 * We modify the caller's `list_head` variable itself, so we need its address. */
static Entity *spawn_entity(Entity **list_head, int16_t x, int16_t y, char symbol, AIFn ai_behavior)
{
    Entity *new_entity = malloc(sizeof *new_entity);
    
    if (!new_entity) exit(1);
    
    *new_entity = (Entity){ x, y, symbol, ai_behavior, NULL };
    new_entity->next = *list_head; 
    *list_head = new_entity;        
    
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
    for (int16_t gem_number = 0; gem_number < game->gems_remaining; gem_number++) 
    {
        int16_t gem_x, gem_y;
        
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

    /* PTR LESSON 8: a "table of function pointers" walked with pointer math.
     * We use a heap block instead of a [] array to keep our no-[] promise. */

    AIFn *ai_table = malloc(NUM_ENEMIES * sizeof *ai_table);

    char *symbol = malloc(NUM_ENEMIES * sizeof(char));

    /* Filling up the AI behavior and Symbol Arrays.*/
    *(ai_table + 0) = ai_chase; *(symbol + 0) = 'C';
    *(ai_table + 1) = ai_wander; *(symbol + 1) = 'W';
    *(ai_table + 1) = ai_protect; *(symbol + 1) = 'P';




    
    return 0;
}