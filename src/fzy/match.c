#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <float.h>
#include <math.h>

#include "match.h"
#include "bonus.h"
#include "upper_lower.h"

char *strcasechr(const char *s, char c) {
  const char accept[3] = {c, TOUPPER(c), 0};
  return strpbrk(s, accept);
}

int has_match(const char *needle, const char *haystack) {
  while (*needle) {
    char nch = *needle++;

    if (!(haystack = strcasechr(haystack, nch))) {
      return 0;
    }
    haystack++;
  }
  return 1;
}

#define max(a, b) (((a) > (b)) ? (a) : (b))

static void precompute_bonus(const char *haystack, score_t *match_bonus) {
  /* Which positions are beginning of words */
  int m = (int)strlen(haystack);
  char last_ch = '/';
  for (int i = 0; i < m; i++) {
    char ch = haystack[i];
    match_bonus[i] = COMPUTE_BONUS(last_ch, ch);
    last_ch = ch;
  }
}

score_t match(const char *needle, const char *haystack) {
  if (!*needle)
    return SCORE_MIN;

  int n = (int)strlen(needle);
  if (n > MAX_NEEDLE_SIZE) {
    n = MAX_NEEDLE_SIZE;
  }
  const int m = (int)strlen(haystack);

  if (m > MAX_HAYSTACK_SIZE) {
    /*
     * Unreasonably large candidate: return no score
     * If it is a valid match it will still be returned, it will
     * just be ranked below any reasonably sized candidates
     */
    return SCORE_MIN;
  }

  score_t match_bonus[MAX_HAYSTACK_SIZE];
  score_t D[MAX_NEEDLE_SIZE][MAX_HAYSTACK_SIZE];
  score_t M[MAX_NEEDLE_SIZE][MAX_HAYSTACK_SIZE];

  /*
   * D[][] Stores the best score for this position ending with a match.
   * M[][] Stores the best possible score at this position.
   */
  precompute_bonus(haystack, match_bonus);

  char tolower_haystack[MAX_HAYSTACK_SIZE];
  for (int j = 0; j < m; ++j) {
    tolower_haystack[j] = TOLOWER(haystack[j]);
  }

  for (int i = 0; i < n; i++) {
    score_t prev_score = SCORE_MIN;
    score_t gap_score = i == n - 1 ? SCORE_GAP_TRAILING : SCORE_GAP_INNER;

    const char tolower_needle_i = TOLOWER(needle[i]);

    for (int j = 0; j < m; j++) {
      if (tolower_needle_i == tolower_haystack[j]) {
        score_t score = SCORE_MIN;
        if (!i) {
          score = (j * SCORE_GAP_LEADING) + match_bonus[j];
        } else if (j) { /* i > 0 && j > 0 */
          score = max(
              M[i - 1][j - 1] + match_bonus[j],

              /* consecutive match, doesn't stack with match_bonus */
              D[i - 1][j - 1] + SCORE_MATCH_CONSECUTIVE);
        }
        if (needle[i] == haystack[j]) {
          score += SCORE_MATCH_CASE;
        }
        D[i][j] = score;
        M[i][j] = prev_score = max(score, prev_score + gap_score);
      } else {
        D[i][j] = SCORE_MIN;
        M[i][j] = prev_score = prev_score + gap_score;
      }
    }
  }

  return M[n - 1][m - 1];
}
