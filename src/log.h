/**
 * In-memory cache of the persistent raft log stored on disk.
 */

#ifndef RAFT_LOG_H
#define RAFT_LOG_H

#include "../include/raft.h"

/**
 * Initial size of the entry reference count hash table.
 */
#define LOG__REFS_INITIAL_SIZE 256

/**
 * Initialize an empty in-memory log of raft entries.
 */
void log__init(struct raft_log *l);

/**
 * Release all memory used by the given log object.
 */
void log__close(struct raft_log *l);

/**
 * Get the current number of outstanding entries in the log, i.e. the number of
 * entries that are not included in the most recent snapshot (if any). Return #0
 * if there are no outstanding entries.
 */
size_t log__n_outstanding(struct raft_log *l);

/**
 * Get the index of the last entry in the log. Return #0 if the log is empty.
 */
raft_index log__last_index(struct raft_log *l);

/**
 * Get the term of the last entry in the log. Return #0 if the log is empty.
 */
raft_term log__last_term(struct raft_log *l);

/**
 * Get the term of the entry with the given index. Return #0 if @index is
 * greater than the last index of the log, or if it's lower than oldest index we
 * know the term of (either because it's outstanding or because it's the last
 * entry in the most recent snapshot).
 */
raft_term log__term_of(struct raft_log *l, raft_index index);

/**
 * Get the last index of the most recent snapshot. Return #0 if there are no
 * snapshots.
 */
raft_index log__snapshot_index(struct raft_log *l);

/**
 * Get the entry with the given index.
 *
 * The returned pointer remains valid only as long as no API that might delete
 * the entry with the given index is invoked. Return #NULL if there is no such
 * entry.
 */
const struct raft_entry *log__get(struct raft_log *l, const raft_index index);

/**
 * Append the an entry to the log.
 */
int log__append(struct raft_log *l,
                const raft_term term,
                const int type,
                const struct raft_buffer *buf,
                void *batch);

/**
 * Convenience to append a series of #RAFT_COMMAND entries.
 */
int log__append_commands(struct raft_log *l,
                         const raft_term term,
                         const struct raft_buffer bufs[],
                         const unsigned n);

/**
 * Convenience to encode and append a single #RAFT_CHANGE entry.
 */
int log__append_configuration(struct raft_log *l,
                              const raft_term term,
                              const struct raft_configuration *configuration);

/**
 * Acquire an array of entries from the given index onwards.
 *
 * The payload memory referenced by the @buf attribute of the returned entries
 * is guaranteed to be valid until log__release() is called.
 */
int log__acquire(struct raft_log *l,
                 const raft_index index,
                 struct raft_entry *entries[],
                 unsigned *n);

/**
 * Release a previously acquired array of entries.
 */
void log__release(struct raft_log *l,
                  const raft_index index,
                  struct raft_entry entries[],
                  const size_t n);

/**
 * Delete all entries from the given index (included) onwards. If the log is
 * empty this is a no-op. If @index is lower than or equal to the index of the
 * first entry in the log, then the log will become empty.
 */
void log__truncate(struct raft_log *l, const raft_index index);

/**
 * Discard all entries from the given index (included) onwards. This is exactly
 * the same as truncate, but the memory of the entries does not gets released.
 */
void log__discard(struct raft_log *l, const raft_index index);

/**
 * To be called when taking a new snapshot.
 *
 * The log must contain an entry at @last_index, which is the index of the last
 * entry included in the snapshot. The function will update the last snapshot
 * information and delete all entries up @last_index - @trailing. If the log
 * contains no entry a @last_index - @trailing, then no entry will be deleted.
 */
void log__snapshot(struct raft_log *l, raft_index last_index, unsigned trailing);

/**
 * To be called when restoring a new snapshot.
 *
 * The log can be in any state. All outstanding entries will be discarded, the
 * last index and last term of the most recent snapshot will be set to the given
 * values, and the offset adjusted accordingly.
 */
void log__restore(struct raft_log *l,
                  raft_index last_index,
                  raft_term last_term);

/**
 * Change the current offset of outstanding entries. This is called at startup
 * when populating the log with entries loaded from disk.
 */
void log__seek(struct raft_log *l, raft_index start_index);

#endif /* RAFT_LOG_H */
