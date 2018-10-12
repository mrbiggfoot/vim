#ifndef COMPL_H
#define COMPL_H

#ifdef __cplusplus
typedef unsigned char char_u;

// XXX FIXME ------------------------------------------------------------------
typedef enum
{
    VAR_UNKNOWN = 0,
} vartype_T;

typedef struct
{
    vartype_T	v_type;
    char	v_lock;	    // see below: VAR_LOCKED, VAR_FIXED
    union
    {
	int64_t	v_number;	// number value
//#ifdef FEAT_FLOAT
	double		v_float;	// floating number value
//#endif
	char_u		*v_string;	// string value (can be NULL!)
	void		*v_list;	// list value (can be NULL!)
	void		*v_dict;	// dict value (can be NULL!)
	void	*v_partial;	// closure: function with args
//#ifdef FEAT_JOB_CHANNEL
	void		*v_job;		// job value (can be NULL!)
	void	*v_channel;	// channel value (can be NULL!)
//#endif
	void		*v_blob;	// blob value (can be NULL!)
    }		vval;
} typval_T;
//-----------------------------------------------------------------------------
#endif // __cplusplus

/*
 * Array indexes used for cp_text[].
 */
#define CPT_ABBR        0  // "abbr"
#define CPT_MENU        1  // "menu"
#define CPT_KIND        2  // "kind"
#define CPT_INFO        3  // "info"
#define CPT_COUNT       4  // Number of entries

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
//#ifdef FEAT_EVAL // XXX FIXME
    typval_T	cp_user_data;
//#endif
    char_u   *cp_fname;   // file containing the match, allocated when
                          // cp_flags has CP_FREE_FNAME
    int      cp_flags;    // CP_ values
    int      cp_number;   // sequence number
    int      has_match;   // used in fuzzy matching
};

#endif // COMPL_H
