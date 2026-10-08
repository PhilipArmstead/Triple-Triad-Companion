// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "solver.h"

#include "game.h"

#include <stdlib.h>
#include <stdint.h>

typedef struct {
	CachedEntry hit; // Move, bound and score encoded in 16 bits
	int32_t alpha;
	int32_t beta;
} CacheProbe;
typedef struct {
	uint64_t key;
	CachedEntry value;
	UT_hash_handle hh;
} MoveMap;
typedef struct {
	Move move;
	GameState nextState;
	uint16_t priority;
} Candidate;
typedef struct {
	Candidate candidates[MAXIUMUM_POSSIBLE_MOVES];
	uint8_t length;
} CandidateList;
static CachedEntry search(GameState state, int32_t alpha, int32_t beta, MoveMap **map);
static uint8_t classifyBound(uint8_t score, int32_t alpha, int32_t beta);
static uint64_t getStateKey(GameState state);
static CacheProbe probeCache(const MoveMap *cache, int alpha, int beta);
static CandidateList getOrderedCandidates(GameState state);
static uint16_t getMovePriority(GameState state, GameState nextState, Move move);
static void sortCandidateIndices(const CandidateList *candidateList, uint8_t order[]);

/**
 * Whether a cached score is the true value of a node, or only a bound on it.
 * A node that pruned never saw all its children, so its score is only a bound
 * and may only be reused when the current window makes that bound decisive.
 */
#define EXACT (0)
#define LOWER_BOUND (1)
#define UPPER_BOUND (2)


CachedEntry solver_getOptimalMove(GameState state) {
	MoveMap *moveMap = NULL;
	const CachedEntry result = search(state, INT32_MIN, INT32_MAX, &moveMap);

	MoveMap *entry;
	MoveMap *temporary;
	HASH_ITER(hh, moveMap, entry, temporary) {
		HASH_DEL(moveMap, entry);
		free(entry);
	}

	return result;
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
static CacheProbe probeCache(const MoveMap *cache, const int alpha, const int beta) {
	if (cache == NULL) {
		return (CacheProbe){.hit = 0xFFFF, .alpha = alpha, .beta = beta};
	}

	const CachedEntry cached = cache->value;

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

static CachedEntry search(GameState state, int32_t alpha, int32_t beta, MoveMap **map) {
	if (state.moveCount == BOARD_SIZE) {
		const uint8_t score = game_getPlayerScore(state);
		const uint8_t bound = classifyBound(score, alpha, beta);
		return encodeHint(0xFF, bound, score);
	}

	const uint64_t key = getStateKey(state);
	MoveMap *cache;
	HASH_FIND(hh, *map, &key, sizeof key, cache);
	const CacheProbe probe = probeCache(cache, alpha, beta);
	if (probe.hit != 0xFFFF) {
		return probe.hit;
	}

	int32_t nextAlpha = probe.alpha;
	int32_t nextBeta = probe.beta;

	const CandidateList candidates = getOrderedCandidates(state);

	const bool maximising = state.currentPlayer == PLAYER_1;
	uint8_t bestScore = maximising ? 0x00 : 0xFF;
	Move bestMove = {.cardId = 0xFF, .position = 0xFF};

	for (uint8_t i = 0; i < candidates.length; ++i) {
		const Candidate candidate = candidates.candidates[i];
		const CachedEntry entry = search(candidate.nextState, nextAlpha, nextBeta, map);
		const uint8_t score = entry & 0x0Fu;
		if (maximising ? score > bestScore : score < bestScore) {
			bestScore = score;
			bestMove = candidate.move;
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
	if (cache != NULL) {
		cache->value = result;
	} else {
		MoveMap *entry = malloc(sizeof *entry);
		if (entry != NULL) {
			entry->key = key;
			entry->value = result;
			HASH_ADD(hh, *map, key, sizeof entry->key, entry);
		}
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

static uint64_t getStateKey(GameState state) {
	uint64_t key = (uint64_t)state.owner1 * 2 + (state.currentPlayer - 1);
	for (int position = 0; position < BOARD_SIZE; position++) {
		key = key * CELL_STATES + (state.grid[position] != 0xFF ? state.grid[position] : EMPTY_CELL);
	}
	return key;
}

/**
 * Legal moves paired with their successor state, best first. Priorities are
 * scored from the mover's own perspective, so the strongest candidate is the
 * highest one for both players, and searching it first maximises cutoffs.
 */
static CandidateList getOrderedCandidates(GameState state) {
	CandidateList candidates = {0};
	const uint16_t hand = state.currentPlayer == PLAYER_1 ? state.hand1 : state.hand2;
	for (uint8_t card = 0; card < CARDS_IN_HAND * 2; ++card) {
		if ((hand & (1 << card)) == 0) {
			continue;
		}

		for (uint8_t cell = 0; cell < BOARD_SIZE; ++cell) {
			if (state.grid[cell] == 0xFF) {
				Move move = {.cardId = card, .position = cell};
				const GameState nextState = game_placeCard(state, move);
				const uint16_t movePriority = getMovePriority(state, nextState, move);
				candidates.candidates[candidates.length++] = (Candidate){
					.nextState = nextState,
					.move = move,
					.priority = movePriority,
				};
			}
		}
	}
	uint8_t order[MAXIUMUM_POSSIBLE_MOVES];
	sortCandidateIndices(&candidates, order);
	Candidate sorted[MAXIUMUM_POSSIBLE_MOVES];
	for (uint8_t i = 0; i < candidates.length; ++i) {
		sorted[i] = candidates.candidates[order[i]];
	}
	for (uint8_t i = 0; i < candidates.length; ++i) {
		candidates.candidates[i] = sorted[i];
	}

	return candidates;
}

static uint16_t getMovePriority(const GameState state, const GameState nextState, const Move move) {
	const uint16_t ownerBefore = state.currentPlayer == PLAYER_1 ? state.owner1 : state.owner2;
	const uint16_t ownerAfter = state.currentPlayer == PLAYER_1 ? nextState.owner1 : nextState.owner2;

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
