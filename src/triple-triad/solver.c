// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "solver.h"

#include "game.h"

#include <intrin.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct {
	CachedEntry hit; // Move, bound and score encoded in 16 bits
	int32_t alpha;
	int32_t beta;
} CacheProbe;
/**
 * A transposition table slot. The full key is stored so a slot can only ever
 * answer for its own state, and the generation marks which solve wrote it, so
 * the table never needs clearing between solves.
 */
typedef struct {
	uint64_t key;
	uint32_t generation;
	CachedEntry value;
} CacheSlot;
typedef struct {
	Move move;
	GameState nextState;
	uint16_t priority;
} Candidate;
typedef struct {
	Candidate candidates[MAXIUMUM_POSSIBLE_MOVES];
	uint8_t length;
} CandidateList;
static CachedEntry search(const GameState *state, int32_t alpha, int32_t beta);
static CacheSlot *getCacheSlot(uint64_t key);
static uint8_t classifyBound(uint8_t score, int32_t alpha, int32_t beta);
static uint64_t getStateKey(const GameState *state);
static CacheProbe probeCache(const CachedEntry *cached, int alpha, int beta);
static void
getOrderedCandidates(const GameState *state, CandidateList *candidates, uint8_t order[]);
static uint16_t getMovePriority(const GameState *state, const GameState *nextState, Move move);
static void sortCandidateIndices(const CandidateList *candidateList, uint8_t order[]);
static bool hasEarlierDuplicate(const GameState *state, uint16_t hand, uint8_t card);

/**
 * Whether a cached score is the true value of a node, or only a bound on it.
 * A node that pruned never saw all its children, so its score is only a bound
 * and may only be reused when the current window makes that bound decisive.
 */
#define EXACT (0)
#define LOWER_BOUND (1)
#define UPPER_BOUND (2)

// 2^21 slots of 16 bytes each (32 MB), allocated once and reused by every solve
#define CACHE_BITS (21)
#define CACHE_SLOTS ((size_t)1 << CACHE_BITS)
// Positions with this many moves made or more skip the table
#define CACHE_MAX_MOVE_COUNT (BOARD_SIZE - 2)

static CacheSlot *cache;
static uint32_t generation;


CachedEntry solver_getOptimalMove(GameState state) {
	if (cache == NULL) {
		// Without a table the search is slower but still correct
		cache = calloc(CACHE_SLOTS, sizeof *cache);
	}
	// Slots start at generation 0, so the first solve must use 1
	++generation;
	return search(&state, INT32_MIN, INT32_MAX);
}

static CacheSlot *getCacheSlot(const uint64_t key) {
	if (cache == NULL) {
		return NULL;
	}
	// Fibonacci hashing spreads the structured keys evenly over the table
	return &cache[(key * 0x9E3779B97F4A7C15ull) >> (64 - CACHE_BITS)];
}

/**
 * How much a searched score tells us. A node that never improved on the window
 * it was given only yields a bound, because its children were cut short.
 */
static uint8_t classifyBound(uint8_t score, int32_t alpha, int32_t beta) {
	return score <= alpha ? UPPER_BOUND : score >= beta ? LOWER_BOUND : EXACT;
}

/**
 * Decides whether a cached entry settles the current node outright, or merely
 * narrows the window it must be searched with. An exact entry always settles it;
 * a bound only does so when it already falls outside the current window.
 */
static CacheProbe probeCache(const CachedEntry *entry, const int alpha, const int beta) {
	if (entry == NULL) {
		return (CacheProbe){.hit = 0xFFFF, .alpha = alpha, .beta = beta};
	}

	const CachedEntry cached = *entry;

	const uint8_t bound = (cached >> 4) & 0x03u;
	if (bound == EXACT) {
		return (CacheProbe){.hit = cached, .alpha = alpha, .beta = beta};
	}

	const uint8_t score = cached & 0x0Fu;
	if (bound == LOWER_BOUND) {
		return score >= beta
						 ? (CacheProbe){.hit = cached, .alpha = alpha, .beta = beta}
						 : (CacheProbe){.hit = 0xFFFF, .alpha = (alpha > score ? alpha : score), .beta = beta};
	}
	return score <= alpha
					 ? (CacheProbe){.hit = cached, .alpha = alpha, .beta = beta}
					 : (CacheProbe){.hit = 0xFFFF, .alpha = alpha, .beta = (beta < score ? beta : score)};
}

static CachedEntry encodeHint(uint8_t move, uint8_t bound, uint8_t score) {
	return (CachedEntry)(((uint16_t)(move & 0xFFu) << 6) | ((uint16_t)(bound & 0x03u) << 4) |
											 ((uint16_t)(score & 0x0Fu)));
}

/**
 * The last move leads straight to the final score, so there's nothing to prune
 * or cache. In a real game there's one card and one cell left, but test boards
 * can leave more, so every remaining move is still tried.
 */
static CachedEntry searchLastMove(const GameState *state, const int32_t alpha, const int32_t beta) {
	const bool maximising = state->currentPlayer == PLAYER_1;
	const uint16_t hand = maximising ? state->hand1 : state->hand2;
	uint8_t bestScore = 0;
	uint8_t bestMove = 0xFF;
	for (uint8_t cell = 0; cell < BOARD_SIZE; ++cell) {
		if (state->grid[cell] != 0xFF) {
			continue;
		}
		for (uint8_t card = 0; card < CARDS_IN_HAND * 2; ++card) {
			if ((hand & (1 << card)) == 0) {
				continue;
			}
			const Move move = {.cardId = card, .position = cell};
			const uint8_t score = game_getPlayerScore(game_placeCard(*state, move));
			if (bestMove == 0xFF || (maximising ? score > bestScore : score < bestScore)) {
				bestScore = score;
				bestMove = (uint8_t)((card << 4) | cell);
			}
		}
	}
	return encodeHint(bestMove, classifyBound(bestScore, alpha, beta), bestScore);
}

static CachedEntry search(const GameState *state, int32_t alpha, int32_t beta) {
	if (state->moveCount == BOARD_SIZE) {
		const uint8_t score = game_getPlayerScore(*state);
		const uint8_t bound = classifyBound(score, alpha, beta);
		return encodeHint(0xFF, bound, score);
	}
	if (state->moveCount == BOARD_SIZE - 1) {
		return searchLastMove(state, alpha, beta);
	}

	const uint64_t key = getStateKey(state);
	// Positions this close to the end are cheaper to re-search than to cache
	CacheSlot *slot = state->moveCount < CACHE_MAX_MOVE_COUNT ? getCacheSlot(key) : NULL;
	const bool isCached = slot != NULL && slot->generation == generation && slot->key == key;
	const CacheProbe probe = probeCache(isCached ? &slot->value : NULL, alpha, beta);
	if (probe.hit != 0xFFFF) {
		return probe.hit;
	}

	int32_t nextAlpha = probe.alpha;
	int32_t nextBeta = probe.beta;

	CandidateList candidates;
	uint8_t order[MAXIUMUM_POSSIBLE_MOVES];
	getOrderedCandidates(state, &candidates, order);

	const bool maximising = state->currentPlayer == PLAYER_1;
	uint8_t bestScore = maximising ? 0x00 : 0xFF;
	Move bestMove = {.cardId = 0xFF, .position = 0xFF};

	for (uint8_t i = 0; i < candidates.length; ++i) {
		const Candidate *candidate = &candidates.candidates[order[i]];
		const CachedEntry entry = search(&candidate->nextState, nextAlpha, nextBeta);
		const uint8_t score = entry & 0x0Fu;
		// Always take the first candidate, so there's a move even when none beats the initial score
		if (i == 0 || (maximising ? score > bestScore : score < bestScore)) {
			bestScore = score;
			bestMove = candidate->move;
		}

		if (maximising) {
			nextAlpha = nextAlpha > bestScore ? nextAlpha : bestScore;
		} else {
			nextBeta = nextBeta < bestScore ? nextBeta : bestScore;
		}
		if (nextBeta <= nextAlpha) {
			break;
		}
	}

	const uint8_t moveMask = (bestMove.cardId << 4) | bestMove.position;
	const CachedEntry result = encodeHint(moveMask, classifyBound(bestScore, alpha, beta), bestScore);
	// Always replace: losing an older entry only costs a re-search, never correctness
	if (slot != NULL) {
		*slot = (CacheSlot){.key = key, .generation = generation, .value = result};
	}
	return result;
}

/**
 * Exact cache key for a state within a single search.
 *
 * Only the grid contents and player one's ownership are encoded.
 * Both hands and the move count are implied by which cards are already
 * on the grid, and player two's ownership is every occupied cell player one does
 * not hold, so including them would be redundant.
 *
 * Each cell holds a card id or an empty marker, giving a base-21 digit that
 * covers the largest possible deck, so the packed key stays a safe integer and
 * distinct states can never collide.
 */
#define EMPTY_CELL (20)
#define CELL_STATES (EMPTY_CELL + 1)

static uint64_t getStateKey(const GameState *state) {
	uint64_t key = (uint64_t)state->owner1 * 2 + (state->currentPlayer - 1);
	for (int position = 0; position < BOARD_SIZE; position++) {
		key = key * CELL_STATES + (state->grid[position] != 0xFF ? state->grid[position] : EMPTY_CELL);
	}
	return key;
}

/**
 * Legal moves paired with their successor state, and the order to search them
 * in, best first. Priorities are scored from the mover's own perspective, so
 * the strongest candidate is the highest one for both players, and searching it
 * first maximises cutoffs.
 */
static void
getOrderedCandidates(const GameState *state, CandidateList *candidates, uint8_t order[]) {
	candidates->length = 0;
	const uint16_t hand = state->currentPlayer == PLAYER_1 ? state->hand1 : state->hand2;
	for (uint8_t card = 0; card < CARDS_IN_HAND * 2; ++card) {
		if ((hand & (1 << card)) == 0 || hasEarlierDuplicate(state, hand, card)) {
			continue;
		}

		for (uint8_t cell = 0; cell < BOARD_SIZE; ++cell) {
			if (state->grid[cell] == 0xFF) {
				Move move = {.cardId = card, .position = cell};
				Candidate *candidate = &candidates->candidates[candidates->length++];
				candidate->move = move;
				candidate->nextState = game_placeCard(*state, move);
				candidate->priority = getMovePriority(state, &candidate->nextState, move);
			}
		}
	}
	sortCandidateIndices(candidates, order);
}

static uint16_t
getMovePriority(const GameState *state, const GameState *nextState, const Move move) {
	const uint16_t ownerBefore = state->currentPlayer == PLAYER_1 ? state->owner1 : state->owner2;
	const uint16_t ownerAfter =
		state->currentPlayer == PLAYER_1 ? nextState->owner1 : nextState->owner2;

	const uint8_t captured = (uint8_t)__popcnt(ownerAfter ^ ownerBefore);
	// Corners are tie-breakers because they're harder to capture.
	// So add a small bonus for cards here (positions 0, 2, 6 and 8).
	uint8_t cornerBonus = 0;
	if ((move.position & 1) == 0 && move.position != 4) {
		cornerBonus = 1;
	}

	return captured * 100 + cornerBonus;
}

// Insertion sort
// This sorts the indices of the array rather than the array itself
// (because of the cost of reorganising this struct).
static void sortCandidateIndices(const CandidateList *candidateList, uint8_t order[]) {
	for (uint8_t i = 0; i < candidateList->length; ++i) {
		order[i] = (uint8_t)i;
	}

	for (uint8_t i = 1; i < candidateList->length; ++i) {
		uint8_t current = order[i];
		uint8_t j = i;

		while (j > 0 && candidateList->candidates[order[j - 1]].priority <
											candidateList->candidates[current].priority) {
			order[j] = order[j - 1];
			--j;
		}

		order[j] = current;
	}
}

/**
 * Whether the hand holds an identical card (same sides and element) with a
 * lower index. Identical cards lead to identical outcomes, so only the first
 * needs searching.
 */
static bool hasEarlierDuplicate(const GameState *state, const uint16_t hand, const uint8_t card) {
	const Card *current = &state->cards[card];
	for (uint8_t other = 0; other < card; ++other) {
		const Card *earlier = &state->cards[other];
		if ((hand & (1 << other)) != 0 && earlier->element == current->element &&
				earlier->attributes.north == current->attributes.north &&
				earlier->attributes.south == current->attributes.south &&
				earlier->attributes.west == current->attributes.west &&
				earlier->attributes.east == current->attributes.east) {
			return true;
		}
	}
	return false;
}
