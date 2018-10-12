
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <fstream>
#include <stdint.h>
#include <string>
#include <sys/stat.h>
#include <unordered_map>
#include <unordered_set>

typedef unsigned char char_u;
#include "match.hpp"
#include "tags_cache.hpp"
extern "C" {
#include "fzy/match.h"

// Hack to avoid inclusion of "misc1.h" which is not compatible with C++.
void preserve_exit(void);

// Another hack.
int semsg(const char *s, ...);
}

using namespace std;

// Number of patterns to cache match results for.
#define NUM_CACHED_PTN  64
static string g_ptns[NUM_CACHED_PTN];
static int g_ptn_idx = 0;
static bool g_reset_ptn_cache;

struct HasMatches {
  // A set bit at 'idx' in this array means that g_ptns[idx] matches the name.
  int32_t   has_matches[(NUM_CACHED_PTN + 31) / 32];

  HasMatches() {
    memset(has_matches, 0, sizeof(has_matches));
  }
};

typedef unordered_map<string, HasMatches> TagNames;

struct TagsCache {
  bool      exists;
  time_t    mtime;
  TagNames  names;

  TagsCache() : exists(false), mtime(0) {}
};

// Maps tags file name to the tags cache.
typedef unordered_map<string, TagsCache> TagFiles;
static TagFiles g_tag_files;

void update_tags_cache_start() {
  g_reset_ptn_cache = false;
  for (TagFiles::iterator it = g_tag_files.begin(); it != g_tag_files.end();
       ++it) {
    it->second.exists = false;
  }
}

void update_tags_cache_finish() {
  for (TagFiles::iterator it = g_tag_files.begin(); it != g_tag_files.end();) {
    if (it->second.exists) {
      ++it;
    } else {
      it = g_tag_files.erase(it);
    }
  }
  if (g_reset_ptn_cache) {
    g_ptn_idx = 0;
    for (int i = 0; i < NUM_CACHED_PTN; ++i) {
      g_ptns[i].clear();
    }
  }
}

void update_tags_cache(const char *fname) try {
  pair<TagFiles::iterator, bool> pr =
    g_tag_files.insert(make_pair(string(fname), TagsCache()));
  TagsCache &c = pr.first->second;

  struct stat attr;
  if (stat(fname, &attr) != 0) {
    // File probably does not exist.
    semsg("no tags '%s' found", fname);
    return;
  }

  c.exists = true;
  if (c.mtime == attr.st_mtime) {
    // The cache is still valid, no need to reread it.
    return;
  }

  ifstream in(fname);
  if (in.is_open()) {
    g_reset_ptn_cache = true;
    c.names.clear();
    c.mtime = attr.st_mtime;

    string line;
    while (getline(in, line)) {
      size_t pos = line.find_first_of(":. \t\n");
      if (pos != string::npos) {
        line.resize(pos);
      }
      if (line.length() >= MIN_TAG_NAME_LENTH) {
        c.names.insert(make_pair(line, HasMatches()));
      }
    }
  }
} catch (...) {
  preserve_exit();
}

void get_tags_cache_matches(const char *pattern,
                            const char ***matches,
                            int *num_matches,
                            int ignore_case) try {
  int mask_idx = 0;
  int32_t mask = 0;
  bool ptn_cached = false;

  for (int i = 0; i < NUM_CACHED_PTN; ++i) {
    if (strcmp(g_ptns[i].c_str(), pattern) == 0) {
      ptn_cached = true;
      mask_idx = i / 32;
      mask = (1 << (i & 31));
      break;
    }
  }
  if (!ptn_cached) {
    g_ptns[g_ptn_idx] = string(pattern);
    mask_idx = g_ptn_idx / 32;
    mask = (1 << (g_ptn_idx & 31));
    ++g_ptn_idx;
    if (g_ptn_idx == NUM_CACHED_PTN) {
      g_ptn_idx = 0;
    }
  }
  const int32_t unset_mask = ~mask;

  unordered_set<const char *> mnames;
  for (TagFiles::iterator it = g_tag_files.begin();
       it != g_tag_files.end(); ++it) {
    TagNames &names = it->second.names;
    if (ptn_cached) {
      for (TagNames::const_iterator nit = names.begin();
           nit != names.end(); ++nit) {
        if (nit->second.has_matches[mask_idx] & mask) {
          mnames.insert(nit->first.c_str());
        }
      }
    } else {
      for (TagNames::iterator nit = names.begin();
           nit != names.end(); ++nit) {
        if (has_custom_match(pattern, nit->first.c_str(), ignore_case)) {
          nit->second.has_matches[mask_idx] |= mask;
          mnames.insert(nit->first.c_str());
        } else {
          nit->second.has_matches[mask_idx] &= unset_mask;
        }
      }
    }
  }

  *num_matches = mnames.size();
  if (*num_matches) {
    *matches = (const char **)calloc(*num_matches, sizeof(const char *));
    int i = 0;
    for (unordered_set<const char *>::const_iterator it = mnames.begin();
         it != mnames.end(); ++it) {
      (*matches)[i++] = *it;
    }
  }
} catch (...) {
  preserve_exit();
}
