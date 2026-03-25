#ifndef M_NOTICE_H
#define M_NOTICE_H

#include "types.h"
#include "m_mail.h"
#include "lb_rtc.h"

#ifdef __cplusplus
extern "C" {
#endif

#define mNtc_BOARD_POST_COUNT 15

/* sizeof(mNtc_board_post) == 0xC8 */
typedef struct notice_board_post_s {
  /* 0x00 */ u8 message[MAIL_BODY_LEN]; /* post contents */
  /* 0xC0 */ lbRTC_time_c post_time; /* date-time of post */
} mNtc_board_post_c;

extern void mNtc_SetInitData();
extern int mNtc_notice_write_num();
extern void mNtc_notice_write(mNtc_board_post_c* new_post);
extern void mNtc_auto_nwrite_time_ct();
extern void mNtc_set_auto_nwrite_data();

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

extern int mNtc_get_message_of_the_week();

/**
 * @brief Base bulletin board selection list.
 *
 * Manages parallel arrays of item values and index mappings.
 * Used as the foundation for randomized selection of noticeboard messages.
 */
class BBSList {
public:
    int mCount;     /* total number of entries */
    int mNumSet;    /* number of entries currently set */
    int* mItems;    /* array of item values */
    int* mIndices;  /* array of index mappings */

    virtual ~BBSList();

    int GetIdx(int i) const;
    int GetItem(int i) const;
};

/**
 * @brief Randomized range list template.
 *
 * Extends BBSList with random selection, add/clear/check operations.
 * Template parameter T is the element type (instantiated with int).
 */
template <typename T>
class RngList : public BBSList {
public:
    virtual ~RngList();

    void Reset();
    void AddIdx(int idx);
    void ClearIdx(int idx);
    BOOL IsSet(int idx) const;
    BOOL IsItemSet(int item) const;
    int GetN(int* out, int n);
    T Get();
    T GetAndCrossOff();
};

/**
 * @brief Inverse randomized range list template.
 *
 * Like RngList but with inverted set/unset logic: entries start as "set"
 * and are removed as they're used. Useful for tracking which messages
 * haven't been shown yet.
 */
template <typename T>
class InverseRngList : public RngList<T> {
public:
    virtual ~InverseRngList();

    void Reset();
    void AddIdx(int idx);
    void ClearIdx(int idx);
    BOOL IsSet(int idx) const;
    int GetN(int* out, int n);
    T Get();
    T GetAndCrossOff();
};

/**
 * @brief Backed inverse range list template.
 *
 * Extends InverseRngList with external u32* backing storage that persists
 * the set/unset state across sessions (e.g., in save data).
 */
template <typename T>
class BackedInverseRngList : public InverseRngList<T> {
public:
    u32* mBackingData; /* external persistent storage */

    BackedInverseRngList(u32* data, int count);
    virtual ~BackedInverseRngList();
};

#endif /* __cplusplus */

#endif /* M_NOTICE_H */
