// This is an open source non-commercial project. Dear PVS-Studio, please check
// it. PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/*
 * match.cpp: functions for custom completion matching
 */

#include <algorithm>
#include <cstdlib>
#include <thread>
#include <vector>

typedef unsigned char char_u;
#include "match.hpp"
extern "C" {
#include "fzy/match.h"

// Hack to avoid inclusion of "misc1.h" which is not compatible with C++.
void preserve_exit(void);
}

using namespace std;

/*
 * Returns true if needle can be found in haystack. Both are null-terminated.
 */
int has_custom_match(const char *needle, const char *haystack,
                     int ignore_case) {
  (void)ignore_case; // not used in fuzzy matching
  return has_match(needle, haystack);
}

struct ComplScore {
  compl_T   *cs_compl;
  score_t   cs_score;

  ComplScore(compl_T *c) : cs_compl(c) {}
};

/* Comparator for compl_score_T based on fuzzy match score. */
bool compl_score_cmp(const ComplScore &l, const ComplScore &r) {
  return (l.cs_score > r.cs_score);
}

static void calc_score_and_sort(const char *pattern,
                                ComplScore *cs,
                                const int num,
                                const int chunk_sz) {
  if (num <= chunk_sz) {
    for (int i = 0; i < num; ++i) {
      const compl_T *c = cs[i].cs_compl;
      if (c->has_match) {
        const char *text = (const char *)
          (c->cp_text[CPT_ABBR] ? c->cp_text[CPT_ABBR] : c->cp_str);
        cs[i].cs_score = match(pattern, text);
      } else {
        cs[i].cs_score = SCORE_MIN;
      }
    }
    sort(cs, cs + num, compl_score_cmp);
  } else {
    thread chunk_thread(calc_score_and_sort, pattern, cs, num / 2, chunk_sz);
    calc_score_and_sort(pattern, cs + num / 2, num - num / 2, chunk_sz);
    chunk_thread.join();
    inplace_merge(cs, cs + num / 2, cs + num, compl_score_cmp);
  }
}

/*
 * Resort match list based on fuzzy match score. Modifies *first_match to
 * point to the new list head.
 */
void sort_custom_matches(const char *pattern, compl_T **first_match) try {
  if (!first_match || !*first_match) {
    return;
  }
  compl_T *compl_first_match = *first_match;
  const bool cyclic = (compl_first_match->cp_prev != NULL);
  vector<ComplScore> cs_vec;

  compl_T *c;
  for (c = compl_first_match; c != NULL;
       c = (c->cp_next == compl_first_match) ? NULL : c->cp_next) {
    cs_vec.push_back(ComplScore(c));
  }

  const int chunk_sz = max(128, (int)cs_vec.size() / MAX_NUM_MATCH_THREADS);
  calc_score_and_sort(pattern, cs_vec.data(), (int)cs_vec.size(), chunk_sz);

  for (size_t i = 1; i < cs_vec.size(); ++i) {
    compl_T *prev = cs_vec[i - 1].cs_compl;
    compl_T *cur = cs_vec[i].cs_compl;
    prev->cp_next = cur;
    cur->cp_prev = prev;
  }
  compl_T *first = cs_vec[0].cs_compl;
  compl_T *last = cs_vec[cs_vec.size() - 1].cs_compl;
  first->cp_prev = cyclic ? last : NULL;
  last->cp_next = cyclic ? first : NULL;

  *first_match = cs_vec[0].cs_compl;
} catch (...) {
  preserve_exit();
}
