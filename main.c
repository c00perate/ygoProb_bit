#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

#define MAX_DECK_SIZE 60
#define MAX_HAND_SIZE 15
#define MAX_EXTRAS_SIZE 15
#define MAX_CARD_NAME 50
#define MAX_CARDS 60
#define MAX_CONDITIONS 20
#define MAX_POSSIBILITIES 20

typedef struct {
    int card_id;
    int count;
} Condition;

typedef struct {
    Condition conditions[MAX_CONDITIONS];
    int num_conditions;
} Possibility;

typedef struct {
    int cards[MAX_DECK_SIZE];
    int size;
} Deck;

typedef struct {
    uint64_t presence_mask;
    int card_counts[MAX_CARDS];
    int cards[MAX_HAND_SIZE];
    int size;
} Hand;

typedef struct {
    int cards[MAX_EXTRAS_SIZE];
    int size;
} Extras;

char card_names[MAX_CARDS][MAX_CARD_NAME];
int next_card_id = 0;
Possibility possibilities[MAX_POSSIBILITIES];
uint64_t possibility_required_masks[MAX_POSSIBILITIES];
int desires_id = -1, extrav_id = -1, upstart_id = -1, prosperity_id = -1, duality_id = -1;
int num_possibilities = 0;
int deck_size;
int hand_size = 5;
int num_trials = 10000000;

int get_card_id(const char* name);
void add_cards_to_deck(Deck* deck, const char* card_name, int copies);
void add_possibility(const char* card_names[], const int counts[], int num_conditions);
void shuffle_partial(Deck* deck, int n);
void add_card_to_hand(Hand* hand, int card_id);
void remove_card_from_hand(Hand* hand, int card_id);
int has_card_in_hand(Hand* hand, int card_id);
int is_one_valid_draw(Hand* hand, Extras* extras, int can_extrav, int can_desires, int can_upstart, int can_prosperity, int can_duality);
void parse_combos_from_file(const char* filename);

int get_card_id(const char* name) {
    for (int i = 0; i < next_card_id; i++) {
        if (strcmp(card_names[i], name) == 0) {
            return i;
        }
    }

    if (next_card_id >= MAX_CARDS) {
        printf("Error: Too many unique cards!\n");
        return -1;
    }

    strcpy(card_names[next_card_id], name);
    return next_card_id++;
}

void add_cards_to_deck(Deck* deck, const char* card_name, int copies) {
    int card_id = get_card_id(card_name);
    if (card_id == -1) {
        return;
    }

    for (int i = 0; i < copies && deck->size < deck_size; i++) {
        deck->cards[deck->size++] = card_id;
    }
}

void add_possibility(const char* card_name[], const int counts[], int num_conditions) {
    if (num_possibilities >= MAX_POSSIBILITIES) {
        printf("Error: Too many possibilities!\n");
        return;
    }

    possibilities[num_possibilities].num_conditions = num_conditions;
    uint64_t required_mask = 0;

    for (int i = 0; i < num_conditions; i++) {
        int card_id = get_card_id(card_name[i]);
        if (card_id == -1) {
            printf("Error: Card '%s' not found!\n", card_name[i]);
            return;
        }
        possibilities[num_possibilities].conditions[i].card_id = card_id;
        possibilities[num_possibilities].conditions[i].count = counts[i];
        required_mask |= (1ULL << card_id);
    }
    possibility_required_masks[num_possibilities] = required_mask;
    num_possibilities++;
}

void shuffle_partial(Deck* deck, int n) {
    for (int i = 0; i < n && i < deck->size; i++) {
        int j = i + rand() % (deck->size - i);
        int temp = deck->cards[i];
        deck->cards[i] = deck->cards[j];
        deck->cards[j] = temp;
    }
}

void add_card_to_hand(Hand* hand, int card_id) {
    if (hand->size < MAX_HAND_SIZE && card_id >= 0 && card_id < MAX_CARDS) {
        hand->cards[hand->size++] = card_id;
        hand->presence_mask |= (1ULL << card_id);
        hand->card_counts[card_id]++;
    }
}

void remove_card_from_hand(Hand* hand, int card_id) {
    for (int i = 0; i < hand->size; i++) {
        if (hand->cards[i] == card_id) {
            hand->cards[i] = hand->cards[--hand->size];
            hand->card_counts[card_id]--;

            if (hand->card_counts[card_id] == 0) {
                hand->presence_mask &= ~(1ULL << card_id);
            }
            return;
        }
    }
}

int has_card_in_hand(Hand* hand, int card_id) {
    return (hand->presence_mask & (1ULL << card_id)) != 0;
}

int is_one_valid_draw(Hand* hand, Extras* extras, int can_extrav, int can_desires, int can_upstart, int can_prosperity, int can_duality) {
    for (int i = 0; i < num_possibilities; i++) {
        if ((hand->presence_mask & possibility_required_masks[i]) != possibility_required_masks[i]) {
            continue;
        }

        int possibility_valid = 1;
        for (int j = 0; j < possibilities[i].num_conditions; j++) {
            if (hand->card_counts[possibilities[i].conditions[j].card_id] < possibilities[i].conditions[j].count) {
                possibility_valid = 0;
                break;
            }
        }

        if (possibility_valid) {
            return 1;
        }
    }

    if (can_desires && desires_id >= 0 && has_card_in_hand(hand, desires_id) && extras->size >= 2) {
        Hand temp_hand;
        Extras temp_extras;
        temp_hand = *hand;
        temp_extras = *extras;

        add_card_to_hand(&temp_hand, temp_extras.cards[temp_extras.size - 1]);
        temp_extras.size--;
        add_card_to_hand(&temp_hand, temp_extras.cards[temp_extras.size - 1]);
        temp_extras.size--;

        if (is_one_valid_draw(&temp_hand, &temp_extras, 0, 0, can_upstart, 0, can_duality)) {
            return 1;
        }
    }

    if (can_extrav && extrav_id >= 0 && has_card_in_hand(hand, extrav_id) && extras->size >= 2) {
        Hand temp_hand;
        Extras temp_extras;
        temp_hand = *hand;
        temp_extras = *extras;

        add_card_to_hand(&temp_hand, temp_extras.cards[temp_extras.size - 1]);
        temp_extras.size--;
        add_card_to_hand(&temp_hand, temp_extras.cards[temp_extras.size - 1]);
        temp_extras.size--;

        if (is_one_valid_draw(&temp_hand, &temp_extras, 0, 0, 0, 0, can_duality)) {
            return 1;
        }
    }

    if (can_prosperity && prosperity_id >= 0 && has_card_in_hand(hand, prosperity_id) && extras->size >= 6) {
        for (int i = 0; i < 6 && i < extras->size; i++) {
            Hand temp_hand;
            Extras temp_extras;
            temp_hand = *hand;
            temp_extras = *extras;

            add_card_to_hand(&temp_hand, temp_extras.cards[i]);

            for (int j = 0; j < temp_extras.size - 6; j++) {
                temp_extras.cards[j] = temp_extras.cards[j + 6];
            }
            temp_extras.size -= 6;

            if (is_one_valid_draw(&temp_hand, &temp_extras, 0, 0, 0, 0, can_duality)) {
                return 1;
            }
        }
    }

    if (can_upstart && upstart_id >= 0 && has_card_in_hand(hand, upstart_id) && extras->size >= 1) {
        Hand temp_hand;
        Extras temp_extras;
        temp_hand = *hand;
        temp_extras = *extras;

        remove_card_from_hand(&temp_hand, upstart_id);
        add_card_to_hand(&temp_hand, temp_extras.cards[temp_extras.size - 1]);
        temp_extras.size--;

        if (is_one_valid_draw(&temp_hand, &temp_extras, 0, can_desires, can_upstart, 0, can_duality)) {
            return 1;
        }
    }

    if (can_duality && duality_id >= 0 && has_card_in_hand(hand, duality_id) && extras->size >= 3) {
        for (int i = 0; i < 3 && i < extras->size; i++) {
            Hand temp_hand;
            Extras temp_extras;
            temp_hand = *hand;
            temp_extras = *extras;

            add_card_to_hand(&temp_hand, temp_extras.cards[i]);

            for (int j = 0; j < temp_extras.size - 3; j++) {
                temp_extras.cards[j] = temp_extras.cards[j + 3];
            }
            temp_extras.size -= 3;

            if (is_one_valid_draw(&temp_hand, &temp_extras, 0, can_desires, can_upstart, can_prosperity, 0)) {
                return 1;
            }
        }
    }
    return 0;
}

void parse_combos_from_file(const char* filename) {
    FILE *pCombos = fopen(filename, "r");
    if (pCombos == NULL) {
        printf("Combos file '%s' not found!\n", filename);
        return;
    }

    char line[256];
    while (fgets(line, sizeof(line), pCombos)) {
        if (line[0] == '\n' || line[0] == '#') continue;

        const char* card_names[MAX_CONDITIONS];
        int counts[MAX_CONDITIONS];
        int num_conditions = 0;

        char* token = strtok(line, " \t\n");
        while (token != NULL && num_conditions < MAX_CONDITIONS) {
            card_names[num_conditions] = strdup(token);

            token = strtok(NULL, " \t\n");
            if (token == NULL) {
                printf("Error: Missing count for card '%s'\n", card_names[num_conditions]);
                free((void*)card_names[num_conditions]);
                break;
            }
            counts[num_conditions] = atoi(token);

            num_conditions++;
            token = strtok(NULL, " \t\n");
        }

        if (num_conditions > 0) {
            add_possibility(card_names, counts, num_conditions);

            for (int i = 0; i < num_conditions; i++) {
                free((void*)card_names[i]);
            }
        }
    }

    fclose(pCombos);
}

int main() {
    srand(time(NULL));

    Deck deck;
    deck.size = 0;

    printf("Enter deck count: ");
    scanf("%d", &deck_size);

    FILE *pDeck = fopen("deck.txt", "r");
    if (pDeck == NULL) {
        printf("Decklist not found!");
        return 1;
    }
    char card[50];
    int copy;
    while (fscanf(pDeck, "%49s", card) != EOF) {
        fscanf(pDeck, "%d", &copy);
        add_cards_to_deck(&deck, card, copy);
    }
    fclose (pDeck);

    add_cards_to_deck(&deck, "blank", deck_size-deck.size);

    desires_id = get_card_id("Desires");
    extrav_id = get_card_id("Extravagance");
    upstart_id = get_card_id("Upstart");
    prosperity_id = get_card_id("Prosperity");
    duality_id = get_card_id("Duality");

    parse_combos_from_file("combo.txt");

    int successes = 0;
    clock_t start = clock();

    Hand hand;
    Extras extras;

    for (int i = 0; i < num_trials; i++) {
        shuffle_partial(&deck, hand_size + MAX_EXTRAS_SIZE);

        hand.presence_mask = 0;
        hand.size = 0;
        memset(hand.card_counts, 0, sizeof(hand.card_counts));

        extras.size = 0;

        for (int j = 0; j < hand_size && j < deck.size; j++) {
            add_card_to_hand(&hand, deck.cards[j]);
        }

        for (int j = hand_size; j < hand_size + MAX_EXTRAS_SIZE && j < deck.size; j++) {
            extras.cards[extras.size++] = deck.cards[j];
        }

        if (is_one_valid_draw(&hand, &extras, 1, 1, 1, 1, 1)) {
            successes++;
        }
    }

    clock_t end = clock();
    double time_taken = ((double)(end - start)) / CLOCKS_PER_SEC;
    double success_rate = (double)successes / num_trials * 100.0;
    printf("Total simulations: %d\n", num_trials);
    printf("Success rate: %.2f%%\n", success_rate);
    printf("Time taken: %.2f seconds\n", time_taken);

    return 0;
}