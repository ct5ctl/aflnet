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
                          *queue _top, /* Top of the list                  */
                          *q_prev100; /* Previous 100 marker              */

// EXP_ST u8  skip_deterministic,        /* Skip deterministic stages?       */
//            force_deterministic,       /* Force deterministic stages?      */
//            use_splicing,              /* Recombine input files?           */
//            dumb_mode,                 /* Run in non-instrumented mode?    */
//            score_changed,             /* Scoring for favorites changed?   */
//            kill_signal,               /* Signal that killed the child     */
//            resuming_fuzz,             /* Resuming an older fuzzing job?   */
//            timeout_given,             /* Specific timeout given?          */
//            not_on_tty,                /* stdout is not a tty              */
//            term_too_small,            /* terminal dimensions too small    */
//            uses_asan,                 /* Target uses ASAN?                */
//            no_forkserver,             /* Disable forkserver?              */
//            crash_mode,                /* Crash mode! Yeah!                */
//            in_place_resume,           /* Attempt in-place resume?         */
//            auto_changed,              /* Auto-generated tokens changed?   */
//            no_cpu_meter_red,          /* Feng shui on the status screen   */
//            no_arith,                  /* Skip most arithmetic ops         */
//            shuffle_queue,             /* Shuffle input queue?             */
//            bitmap_changed = 1,        /* Time to update bitmap?           */
//            qemu_mode,                 /* Running in QEMU mode?            */
//            skip_requested,            /* Skip request, via SIGUSR1        */
//            run_over10m,               /* Run time over 10 minutes?        */
//            persistent_mode,           /* Running in persistent mode?      */
//            deferred_mode,             /* Deferred forkserver mode?        */
//            fast_cal;                  /* Try to calibrate faster?         */

#endif