// afl-fuzz-global.h
// afl-fuzz.c与afl-fuzz-mutation-rule.c之间的全局变量声明和定义
#ifndef AFL_FUZZ_GLOBAL_H
#define AFL_FUZZ_GLOBAL_H

//afl-fuzz
#define AFL_MAIN
#include "android-ashmem.h"
#define MESSAGES_TO_STDOUT

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#define _FILE_OFFSET_BITS 64

#include "config.h"
#include "types.h"
#include "debug.h"
#include "alloc-inl.h"
#include "hash.h"
#include "libs/cJSON.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <signal.h>
#include <dirent.h>
#include <ctype.h>
#include <fcntl.h>
#include <termios.h>
#include <dlfcn.h>
#include <sched.h>

#include <sys/wait.h>
#include <sys/time.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <sys/capability.h>

#include "aflnet.h"
#include <graphviz/gvc.h>
#include <math.h>


#include <sys/types.h>

extern u8 *stage_name,       /* Name of the current fuzz stage   */
          *stage_short,               /* Short stage name                 */
          *syncing_party;             /* Currently syncing with...        */

extern s32 stage_cur, stage_max;      /* Stage progression                */
extern s32 splicing_with;        /* Splicing with which test case?   */

extern u8  stage_val_type;            /* Value type (STAGE_VAL_*)         */

extern u64 stage_finds[32],           /* Patterns found per fuzz stage    */
           stage_cycles[32];          /* Execs per fuzz stage             */

extern enum {
  /* 00 */ STAGE_FLIP1,
  /* 01 */ STAGE_FLIP2,
  /* 02 */ STAGE_FLIP4,
  /* 03 */ STAGE_FLIP8,
  /* 04 */ STAGE_FLIP16,
  /* 05 */ STAGE_FLIP32,
  /* 06 */ STAGE_ARITH8,
  /* 07 */ STAGE_ARITH16,
  /* 08 */ STAGE_ARITH32,
  /* 09 */ STAGE_INTEREST8,
  /* 10 */ STAGE_INTEREST16,
  /* 11 */ STAGE_INTEREST32,
  /* 12 */ STAGE_EXTRAS_UO,
  /* 13 */ STAGE_EXTRAS_UI,
  /* 14 */ STAGE_EXTRAS_AO,
  /* 15 */ STAGE_HAVOC,
  /* 16 */ STAGE_SPLICE
};

extern u8  skip_deterministic;        /* Skip deterministic stages?       */

extern struct queue_entry *queue,     /* Fuzzing queue (linked list)      */
                          *queue_cur, /* Current offset within the queue  */
                          *queue_top, /* Top of the list                  */
                          *q_prev100; /* Previous 100 marker              */

extern u8  dumb_mode,                 /* Run in non-instrumented mode?    */
            use_splicing,              /* Recombine input files?           */
            no_arith;                  /* Skip most arithmetic ops         */

extern u32 master_id, master_max;     /* Master instance job splitting    */

extern u32 queued_paths,              /* Total number of queued testcases */
            havoc_div,             /* Cycle count divisor for havoc    */
            pending_not_fuzzed,        /* Queued but not done yet          */
            pending_favored,           /* Pending favored paths            */
            current_entry;             /* Current queue entry ID           */
extern u64 unique_crashes;            /* Crashes with unique signatures   */

extern s32 stage_cur_byte;            /* Byte offset of current stage op  */
extern s32 stage_cur_val;             /* Value used for stage op          */

extern u8* trace_bits;                /* SHM with instrumentation bitmap  */

extern u64 blocks_eff_total,          /* Blocks subject to effector maps  */
            blocks_eff_select;         /* Blocks selected as fuzzable      */

extern struct queue_entry {

  u8* fname;                          /* File name for the test case      */
  u32 len;                            /* Input length                     */

  u8  cal_failed,                     /* Calibration failed?              */
      trim_done,                      /* Trimmed?                         */
      was_fuzzed,                     /* Had any fuzzing done yet?        */
      passed_det,                     /* Deterministic stages passed?     */
      has_new_cov,                    /* Triggers new coverage?           */
      var_behavior,                   /* Variable behavior?               */
      favored,                        /* Currently favored?               */
      fs_redundant;                   /* Marked as redundant in the fs?   */

  u32 bitmap_size,                    /* Number of bits set in bitmap     */
      exec_cksum;                     /* Checksum of the execution trace  */

  u64 exec_us,                        /* Execution time (us)              */
      handicap,                       /* Number of queue cycles behind    */
      depth;                          /* Path depth                       */

  u8* trace_mini;                     /* Trace bytes, if kept             */
  u32 tc_ref;                         /* Trace bytes ref count            */

  struct queue_entry *next,           /* Next element, if any             */
                     *next_100;       /* 100 elements ahead               */

  region_t *regions;                  /* Regions keeping information of message(s) sent to the server under test */
  u32 region_count;                   /* Total number of regions in this seed */
  u32 index;                          /* Index of this queue entry in the whole queue */
  u32 generating_state_id;            /* ID of the start at which the new seed was generated */
  u8 is_initial_seed;                 /* Is this an initial seed */
  u32 unique_state_count;             /* Unique number of states traversed by this queue entry */

};

extern enum {
  /* 00 */ STAGE_VAL_NONE,
  /* 01 */ STAGE_VAL_LE,
  /* 02 */ STAGE_VAL_BE
};

extern s8  interesting_8[]  = { INTERESTING_8 };
extern s16 interesting_16[] = { INTERESTING_8, INTERESTING_16 };
extern s32 interesting_32[] = { INTERESTING_8, INTERESTING_16, INTERESTING_32 };

extern struct extra_data* extras;     /* Extra tokens to fuzz with        */
extern u32 extras_cnt;                /* Total number of tokens read      */

extern struct extra_data* a_extras;   /* Automatically selected extras    */
extern u32 a_extras_cnt;              /* Total number of tokens available */

/* flags */
extern u8 use_net;
extern u8 poll_wait;
extern u8 server_wait;
extern u8 socket_timeout;
extern u8 protocol_selected;
extern u8 terminate_child;
extern u8 corpus_read_or_sync;
extern u8 state_aware_mode;
extern u8 region_level_mutation;
extern u8 state_selection_algo, seed_selection_algo;
extern u8 false_negative_reduction;

extern volatile u8 stop_soon,         /* Ctrl-C pressed?                  */
                    clear_screen,  /* Window resized?                  */
                    child_timed_out;   /* Traced process timed out?        */

extern char **was_fuzzed_map; /* A 2D array keeping state-specific was_fuzzed information */
extern u32 target_state_id;

extern klist_t(lms) *kl_messages;

#endif