#ifndef COMPL_H
#define COMPL_H

/*
 * Array indexes used for cp_text[].
 */
#define CPT_ABBR        0  // "abbr"
#define CPT_MENU        1  // "menu"
#define CPT_KIND        2  // "kind"
#define CPT_INFO        3  // "info"
#define CPT_USER_DATA   4  // "user data"
#define CPT_COUNT       5  // Number of entries

/*
 * Structure used to store one match for insert completion.
 */
typedef struct compl_S compl_T;
struct compl_S
{
    compl_T  *cp_next;
    compl_T  *cp_prev;
    char_u   *cp_str;     // matched text
    char_u   *(cp_text[CPT_COUNT]);  // text for the menu
    char_u   *cp_fname;   // file containing the match, allocated when
                          // cp_flags has CP_FREE_FNAME
    int      cp_flags;    // CP_ values
    int      cp_number;   // sequence number
    int      has_match;   // used in fuzzy matching
};

#endif // COMPL_H
