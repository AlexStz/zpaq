/* zpaq.cpp v6.35 - Journaling incremental deduplicating archiver

  Copyright (C) 2013, Dell Inc. Written by Matt Mahoney.

    LICENSE

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 3 of
    the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details at
    Visit <http://www.gnu.org/copyleft/gpl.html>.

zpaq is for creating journaling compressed archives for incremental
backups of files and directory trees. Incremental update of an entire
disk is fast because only those files whose last-modified date has
changed are added. zpaq is journaling, which means that the archive
is (mostly) append-only, and both the old and new versions are saved.
You can extract the old versions by rolling back the archive to an
earlier version. zpaq deduplicates: it saves identical files or file
fragments only once by comparing SHA-1 hashes to those stored in the
archive.

zpaq compresses in the open-standard ZPAQ level 2 (v2.02) journaling
format specified at http://mattmahoney.net/zpaq/
The format is self describing, meaning that new versions of the
archiver that improve compression will still produce archives that
older decompressers can read, because the decompression instructions
are stored in the archive.

Usage: command archive.zpaq [file|dir]... -options...
Commands:
  a  add               Add changed files to archive.zpaq
  x  extract           Extract latest versions of files
  l  list              List contents
  d  delete            Mark as deleted in a new version of archive
  t  test              Test archive integrity
Options (may be abbreviated):
  -not <file|dir>...   Exclude
  -to <file|dir>...    Rename external files or specify prefix
  -until N|YYYYMMDD[HH[MM[SS]]]    Revert to version number or date
  -force               a: Add even if unchanged. x: output clobbers
  -quiet [N]           Don't show files smaller than N (default none)
  -threads N           Use N threads (default: cores detected)
  -fragile             Don't add or verify checksums or recovery info
  -method 0...6        Compress faster...better (default: 1)
list options:
  -summary [N]         Show top N files and types (default: 20)
  -since N             List from N'th update or last -N updates
  -all                 List all versions

The archive file name must end with ".zpaq" or the extension will be
assumed. Commands and options may be abbreviated as long as it is not
ambiguous, like -th or -thr (but not -t) for -threads.
It is recommended that if zpaq is run from a script that abbreviations
not be used because future versions might add options that would make
existing abbreviations ambiguous. Commands:

  a or -add

Add files and directory trees to the archive. Directories are scanned
recursively to add subdirectories and their contents. Only ordinary
files and directories are added, not special types (devices, symbolic
links, etc). Symbolic links are not followed. File names and directories
may be specified with absolute or relative paths or no paths, and will
be saved that way. The last-modified date is saved, rounded to the nearest
second. Windows attributes or Linux permissions are saved. However,
additional metadata such as owner, group, extended attributes (xattrs,
ACLs, alternate streams) are not saved.

Before adding files, the last-modifed date is compared with the
date stored in the archive. Files are added only if the dates
differ or if the file is not already in the archive, or if -force
is specified.

When a file is changed, both the old and new versions are saved to
the archive. When a directory is added, any files that existed in
the old version of the directory but not the new version are marked
as deleted in the current version but remain in the archive.

For all commands, file names and directories may be specified with
wildcards. A * matches any string of length 0 or more up to the
next slash. A ? matches exactly one character. In Linux, names with
wildcards must be quoted to protect them from the shell. In Windows,
the forward (/) and back (\) slash are equivalent and stored as (/).
When adding directories, a trailing slash like c:\ is eqivalent
to all of the files in that directory but not the directory itself,
as in c:\*

If the archive is "" (an empty string), then zpaq will test compression
(report size and time) without writing to disk.

  x or -extract

Extract files and directores (default: all) from the archive.
Files will be extracted by creating filenames as saved in the archive.
If any of the external files already exist then zpaq will exit with
an error and not extract any files unless -force is given to allow
files to be overwritten. Last-modified dates and attributes will be
restored as saved. Attributes saved in Windows will not be restored
in Linux and vice versa. If there are multiple versions of a file
saved, then the latest version will be extracted unless an earlier
version is specified with -until.

  l or -list

List the archive contents. Each file or directory is shown with a
version number, date, attributes, uncompressed size, and file name.
There may be multiple versions of a single file with different version
numbers. A second table lists for each version number the number of
fragments, date, number of files added and deleted, and the number
of MB added before and after compression. Attributes are shown as
a string of characters like "DASHRI" in Windows (directory, archive,
system, hidden, read-only, indexed), or an octal number like "100644"
(as passed to chmod(), meaning rw-r--r--) in Linux. If any special
Windows attributes are set, then the value is displayed in hex.

  d or -delete

Mark files and directories in the archive as deleted. This actually
makes the archive slightly larger. The files can still be extracted
by rolling back to an earlier version using -until.

  t or -test

Print statistics about the archive. Return an exit status of 0 if
no errors are detected, or 1 if the archive is corrupted in any way that
would prevent any version of any file from being extracted properly.
Filename arguments are ignored.

Options:

  -not

Exclude files and directories (before renaming) from being added,
extracted, listed, or deleted. For example
"zpaq a arc calgary -not calgary/book?"
will add all the files in calgary except book1 and book2.

  -to

When extracting an entire archive, specify a prefix to append to all
output files. For example, "zpaq x arc -to out/" will extract
calgary/book1 to out/calgary/book1. When there are filename arguments,
replace them with the arguments of -to in order. For example,
"zpaq x arc a b c -to d e" will extract a to d, b to e, and c without
renaming, where a, b, c, d, e can be files or directory trees.
With add, -to renames the external files. For example,
"zpaq a arc a b c -to d e" compresses the files (or directories)
d, e, c, saving the names as a, b, c respectively.

  -until N
  -until YYYYMMDD[HH[MM[SS]]]

Roll back the archive to an earlier version. With -list, -extract, and -test,
versions later than N will be ignored. With -add and -delete, the archive
will be truncated at N, discarding any subsquently added contents
before updating. The version can also be specified as a date and time
(UCT time zone) as an 8, 10, 12, or 14 digit number in the range
19000101 to 29991231235959 with the last 6 digits defaulting to 235959.
The default is the latest version. For backward compatibility, -version
is equivalent to -until.

  -force

Add files even if the dates match. If a file really is identical,
then it will not be added. When extracting, output files will be
overwritten.

  -quiet [N]

With N, show only files or blocks of size at least N bytes but
otherwise display normally. The default is to suppress all output
except errors.

  -threads N

Set the number of threads to N for parallel compression and decompression.
The default is to detect the number of processor cores and use that value
or the limit according to -method, whichever is less. The number of cores
is detected from the environment variable %NUMBER_OF_PROCESSORS% in
Windows or /proc/cpuinfo in Linux.

  -fragile

With -add, don't add header locator tags, checksums, or a redundant
list of fragment sizes to data blocks. This can produce slightly
smaller archives and compress faster, but make it more difficult
(usually impossible) to detect and recover from any damage to the archive.
It is not possible to completely test fragile archives. With -extract,
checksums (if present) are not verified during extraction, which can
be faster. Recommended only for testing.

  -method 0..6

Compress faster..better. The default is -method 1. (See below for advanced
compression options).

List options:

  -summary N

When listing contents, show only the top N files, directories, and
filename extensions by total size. The default is 20. Also show
a table of deduplication statistics and a table of version dates.

  -since N

List only versions N and later. If N is negative then list only the
last -N updates. Default is 0 (all).

  -all

List all versions of each file, including deletions. The default is to
list only the latest version, or not list it if it is deleted.


Advanced compression options:

  -method {0..6|x|s}[N1[,N2]...][[f<cfg>|c|i|a|m|s|t|w][N1][,N2]...]...

With a single digit, select compression speed, where low values are
faster and higher values compress smaller. The default is 1.
N1, if present, selects a maximum block size of 2^(N1+20) - 4096 bytes.
The default for methods 0, 1, x, s is 4 (16 MB, e.g. -method 14).
The default for methods 2..5 is 6 (64 MB, e.g. -method 26).

When the first character is x, select experimental compression methods.
These are for advanced uses only. The method is described
by a group of commands beginning with a letter and followed by
numbers separated by commas or periods without spaces. The first
group starts with x and subsequent groups must start with f<cfg>, c, i,
a, m, s, t, or w, where <cfg> is the name of a configuration file
without the trailing .cfg extension, for example, "-method x4,0fmax".
The numeric arguments in each group have the following meanings depending
on the initial letter. The default values of all numeric arguments is 0
unless specified otherwise.

  x

N1 selects a maximum block size of 2^(20+N1) - 4096 bytes. Fragments
are grouped into blocks which are compressed or decompressed in
parallely by separate threads. Each thread requires 2 times the block
size in memory per thread to temporarily store the input and output for
compression. For decompression, it requires storing only the block size.

N2 selects the preprocessing step as follows:

  0 = no preprocessing.
  1 = LZ77 with variable length codes.
  2 = LZ77 with byte-aligned codes.
  3 = BWT.
  4..7 = 0..3 with E8E9 transform applied first.

The E8E9 tranform scans the input block from back to front for
5 byte sequences of the form (E8|E9 xx xx xx 00|FF) and adds
the block offset of the sequence start (0..n-4) to the middle 3 bytes,
interpreted LSB first. It improves the compression of x86 .exe and
.dll files.

LZ77 with variable length codes is designed for compression with no
further context modeling. Bit codes are packed LSB first and are
interpreted as follows:

  00,n,L[n] = n literal bytes
  mm,mmm,n,ll,r,q (mm > 00) = match of length 4n+ll, offset ((q-1)<<rb)+r-1

where q is written in 8mm+mmm-8 (0..23) bits with an implied leading 1 bit
and n is written using interleaved Elias Gamma coding, i.e. the leading
1 bit is implied, remaining bits are preceded by a 1 and terminated by
a 0. e.g. abc is written 1,b,1,c,0. Codes are packed LSB first and
padded with leading 0 bits in the last byte. r is written in
rb = max(0, N1-4) bits, where N1 is the block size parameter.

Byte oriented LZ77 with minimum match length m = N5 with m in 1..64:

  00xxxxxx   x+1 (1..64) literals follow
  yyxxxxxx   match of length x+m (m..m+63), with y+1 (2..4) bytes of
             offset-1 to follow, MSB first.

BWT sorts the block by suffix with an implied terminator of -1 encoded
as 255. The 4 byte offset of this terminator is appended to the end
of the block LSB first, thus increasing the block size by 5. The maximum
allowed block size is 16 MB (N1 = 4). Memory required is 5x block size
for both compression and decompression.

N3 through N8 affect only LZ77 compression as follows:

N3 = minimum match length. If the longest match found is less than N3,
then literals are coded instead.

N4 = context length to search first (normally N4 > N3), or 0 to skip
this search.

N5 = search for 2^N5 matches of length N4 (unless 0), then 2^N5 more
matches of length N3. The longest match is chosen, breaking ties by
choosing the closer one.

N6 = use a hash table of 2^N6 elements to store the location of context
hashes. It requires 4 x 2^N6 bytes of memory for compression only.

For example, -method x4,5,4,0,3,24 specifies a 2^4 =  16 MB block size,
E8E9 + LZ77 (4+1) with variable length codes, minimum match length 4, no
secondary search (0), search length of 2^3 = 8, hash table size of 2^24
elements. This gives fast and reasonable compression, requiring 96 MB per
thread to compress and 16 MB per thread to decompress.

The default is method -x4,1,4,0,3,24

  s

Store in streaming format rather than journaling. Each file is stored
in a separate block. Files larger than the block size specified by
N1 (2^(N1+20) - 4096) are split into blocks of this size. No deduplication
is performed. Directories are not stored. Block sizes, dates, and attributes
are stored in the first segment comment header of the first block
as "size YYYYMMDDHHMMSS wN" (or "uN") all as decimal numbers. Subsequent
blocks of the same file store only the size and leave the filename blank
as well. Compression is single threaded. Options are the same as x.

Subsequent groups beginning with a letter specify context models. zpaq
compresses by predicting bits one at a time and arithmetic coding.
Predictions are performed by a chain of context models, each taking
a context (some set of past input bits or their hash) and possibly
the predictions of other components as input. The final component
in the chain is used to assign optimal length codes. The model, along
with code to reverse any preprocessing steps, is saved in the archive
as a program written in ZPAQL, as described in the ZPAQ specification
and in libzpaq.h. Each letter describes a context model as follows:

  c - Context model (CM or ICM).
  i - ISSE chain.
  a - MATCH.
  m - MIX.
  t - MIX2.
  s - SSE
  w - Word model ISSE chain.

Memory usage per component is generally limited to the block size, but
may be less for small contexts. The numeric arguments to each model
are as follows:

  c

N1 is 0 for an ICM and 1..256 to specify a CM with limit argument N1-1.
Higher values are better for stationary sources. A CM maps a context
has directly to a prediction and has an update rate that decreases with
higher N1. An ICM maps a context to a bit history and then to a prediction.
An ICM adapts rapidly to different data types or to sorted data.

N2 in 1..255 includes (offset mod N2) in the context hash. N2 in 1000..1255
includes the distance to the last occurrence of N2-1000 in the context
hash.

N3,... in 0..255 specifies a list of byte context masks reading back from
the most recently coded byte. Each mask is ANDed with the context byte
before hashing. a value of 1000 or more is equivalent to a sequence
of N-1000 zeros. Only the last 65536 bytes of context are saved.
For byte aligned LZ77 only, N3,... in 256..511 specifies either the LZ77
parse state if a match/literal code or match offset byte is expected,
or else a literal byte ANDed with the low 8 bits as with 0..255. For example,
-method x4,6,8,0,4,24,12,16c0,0,511 specifies E8E9 + byte aligned LZ77 (6),
and an order 1 ICM context model that includes the LZ77 parse state.

  i

ISSE chain. Each ISSE takes a prediction from the previous component
as input and adjusts it according to the bit history of the specified
context. There must be a previous component. The arguments N,...
specify an increase of N in the context order from previous component's
context, which is hashed together with the most recently coded bytes.
For example, -method x4,3ci1,1,2 specifies a 16 MB block (4), BWT (3),
followed by an order 0 ICM (c) and an ISSE chain of 3 components with
context orders 1, 2, and 4.

  a

Specifies a MATCH. A MATCH maintains a history buffer and a hash table
index to quickly find the most recent occurrence of the current context
and predict whatever bit follows. If a prediction fails, then it makes
no further predictions for the rest of the byte and then it looks up
the context hash on the next boundary. The hash is computed as the
xN1 + 18 - N3 low bits of hash := hash*N1+c+1 where c is the most recently
coded byte and xN1 is the block size as specified by x. N2 and N3 specify
how many times to halve the memory usage of the buffer and hash table,
respectively. For example, -method x4,0a24,0,1 specifies a 16 MB block
for compression (x4) with no preprocessing (0). The match model uses
a multiplier of 24, a history buffer the same size as the compression
buffer (0), and a hash table using half as much memory (1). This means
that the hash table index is 4 + 18 - 1 = 21 bits. The multiplier of 24
when written in binary has 3 trailing 0 bits. Thus, it computes a hash
of order 21/3 = 7. A high order match is most useful when mixed with
other low-order context models. Default is a24,0,0

  m

Specifies a MIX. A MIX does weighted averaging of all previously specified
components and adapts the weights in favor of the most accurate ones.
The mixing weights can be selected by a (usually small) context for
better compression.

N1 is the context size in bits. The context is not hashed. If N1 is not a
multiple of 8 then the low bits of the oldest byte are discarded.
A good value is 0, 8, or 16. N2 specifies the update rate. A good value
is around 16 to 32. For example, -method x6,0ci1,1,1m8,24 specifies
64 MB blocks, no preprocessing, an order 0-1-2-3 ICM-ISSE chain with a
final mixer taking all other components as input, order 0 (bitwise order 8)
context, and a learning rate of 24. Default is m8,24.

  t

Specifies a MIX2. The input is only the previous 2 components. N1 and N2
are the same as MIX. For example,
-method x6,0ci1,1,1m8,10m16,32t0,24 specifies the previous model with
2 mixers taking different contexts with different learning rates, and
then mixing both of them together with no context and a learning rate of 24.
Default is t8,24.

  s

Specifies an SSE. An SSE, like an ICM, uses a context to adjust the
prediction of the previous component, but does so using the direct
context (via a 2-D lookup table of the quantized and intertolated
prediction) rather than a linear adjustment based on the bit history.
Thus, it has more parameters, making a smaller context more appropriate.

The input is the previous component. N1 is the context
size in bits as in a MIX or MIX2. N2 and N3 are the start and limit
arguments, where higher values specify lower initial and final learning
rates. Default is s8,32,255.

  w

Word-model ICM-ISSE chain for modeling text. N1 is the length of the
chain, with word-level context orders of 0..N1-1. A word is defined
as a sequence of characters in the range N2..N2+N3-1 after ANDing
with N4. For Default is w1,65,26,223,20 represents the set [A-Za-z]
using a context hash multiplier of 20.

  f<cfg>

Specifies a custom configuration file containing ZPAQL code.
The ZPAQL language is described in libzpaq.h. N1..N9 are passed
as arguments $1..$9, except that $1 may be reduced such that
the actual block size is at most 2^$1 bytes.
If there is a post-processor (PCOMP) section, then
it must correcty invert any preprocessing steps specified by the N2
argument to x. The PCOMP argument, normally the name of an external
preprocessor command, is ignored. The .cfg filename extension is optional.
Component specifications other than x are ignored.

For example, -method x6,0fmax specifies 64 MB blocks, no preprocessing,
and modeling using the external file max.cfg with $1 and $2 passed
as 6 and 0 respectively.


TO COMPILE:

This program needs libzpaq from http://mattmahoney.net/zpaq/ and
libdivsufsort-lite from above or http://code.google.com/p/libdivsufsort/
Recommended compile for Windows with MinGW:

  g++ -O3 -s -msse2 zpaq.cpp libzpaq.cpp divsufsort.c -o zpaq -DNDEBUG

With Visual C++:

  cl /O2 /EHsc /arch:SSE2 /DNDEBUG zpaq.cpp libzpaq.cpp divsufsort.c

For Linux:

  g++ -O3 -s -Dunix -DNDEBUG zpaq.cpp libzpaq.cpp divsufsort.c \
  -fopenmp -o zpaq

Possible options:

  -o         Name of output executable.
  -O3 or /O2 Optimize (faster).
  /EHsc      Enable exception handing in VC++ (required).
  -s         Strip debugging symbols. Smaller executable.
  /arch:SSE2 Assume x86 processor with SSE2. Otherwise use -DNOJIT.
  -msse2     Same. Implied by -m64 for a x86-64 target.
  -DNOJIT    Don't assume x86 with SSE2 for libzpaq. Slower (disables JIT).
  -static    Don't assume C++ runtime on target. Bigger executable but safer.
  -Dunix     Not Windows. Sometimes automatic in Linux. Needed for Mac OS/X.
  -fopenmp   Parallel divsufsort (faster, implies -pthread, broken in MinGW).
  -pthread   Required in Linux, implied by -fopenmp.
  -DNDEBUG   Turn off debugging checks in divsufsort (faster).
  -DDEBUG    Turn on debugging checks in libzpaq, zpaq (slower).
  -DPTHREAD  Use Pthreads instead of Windows threads. Requires pthreadGC2.dll
             or pthreadVC2.dll from http://sourceware.org/pthreads-win32/
  -Dunixtest To make -Dunix work in Windows with MinGW.

*/
#define _FILE_OFFSET_BITS 64  // In Linux make sizeof(off_t) == 8
#include "libzpaq.h"
#include "divsufsort.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <stdint.h>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <stdexcept>

#ifndef DEBUG
#define NDEBUG 1
#endif
#include <assert.h>

#ifdef unix
#define PTHREAD 1
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <dirent.h>
#include <utime.h>

#else  // Assume Windows
#define UNICODE
#include <windows.h>
#include <io.h>
#endif

using std::string;
using std::vector;
using std::map;

// Handle errors in libzpaq and elsewhere
void libzpaq::error(const char* msg) {
  fprintf(stderr, "zpaq error: %s\n", msg);
  throw std::runtime_error(msg);
}
using libzpaq::error;

// Portable thread types and functions for Windows and Linux. Use like this:
//
// // Create mutex for locking thread-unsafe code
// Mutex mutex;            // shared by all threads
// init_mutex(mutex);      // initialize in unlocked state
// Semaphore sem(n);       // n >= 0 is initial state
//
// // Declare a thread function
// ThreadReturn thread(void *arg) {  // arg points to in/out parameters
//   lock(mutex);          // wait if another thread has it first
//   release(mutex);       // allow another waiting thread to continue
//   sem.wait();           // wait until n>0, then --n
//   sem.signal();         // ++n to allow waiting threads to continue
//   return 0;             // must return 0 to exit thread
// }
//
// // Start a thread
// ThreadID tid;
// run(tid, thread, &arg); // runs in parallel
// join(tid);              // wait for thread to return
// destroy_mutex(mutex);   // deallocate resources used by mutex
// sem.destroy();          // deallocate resources used by semaphore

#ifdef PTHREAD
#include <pthread.h>
typedef void* ThreadReturn;                                // job return type
typedef pthread_t ThreadID;                                // job ID type
void run(ThreadID& tid, ThreadReturn(*f)(void*), void* arg)// start job
  {pthread_create(&tid, NULL, f, arg);}
void join(ThreadID tid) {pthread_join(tid, NULL);}         // wait for job
typedef pthread_mutex_t Mutex;                             // mutex type
void init_mutex(Mutex& m) {pthread_mutex_init(&m, 0);}     // init mutex
void lock(Mutex& m) {pthread_mutex_lock(&m);}              // wait for mutex
void release(Mutex& m) {pthread_mutex_unlock(&m);}         // release mutex
void destroy_mutex(Mutex& m) {pthread_mutex_destroy(&m);}  // destroy mutex

class Semaphore {
public:
  Semaphore() {sem=-1;}
  void init(int n) {
    assert(n>=0);
    assert(sem==-1);
    pthread_cond_init(&cv, 0);
    pthread_mutex_init(&mutex, 0);
    sem=n;
  }
  void destroy() {
    assert(sem>=0);
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cv);
  }
  int wait() {
    assert(sem>=0);
    pthread_mutex_lock(&mutex);
    int r=0;
    if (sem==0) r=pthread_cond_wait(&cv, &mutex);
    assert(sem>0);
    --sem;
    pthread_mutex_unlock(&mutex);
    return r;
  }
  void signal() {
    assert(sem>=0);
    pthread_mutex_lock(&mutex);
    ++sem;
    pthread_cond_signal(&cv);
    pthread_mutex_unlock(&mutex);
  }
private:
  pthread_cond_t cv;  // to signal FINISHED
  pthread_mutex_t mutex; // protects cv
  int sem;  // semaphore count
};

#else  // Windows
typedef DWORD ThreadReturn;
typedef HANDLE ThreadID;
void run(ThreadID& tid, ThreadReturn(*f)(void*), void* arg) {
  tid=CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)f, arg, 0, NULL);
  if (tid==NULL) error("CreateThread failed");
}
void join(ThreadID& tid) {WaitForSingleObject(tid, INFINITE);}
typedef HANDLE Mutex;
void init_mutex(Mutex& m) {m=CreateMutex(NULL, FALSE, NULL);}
void lock(Mutex& m) {WaitForSingleObject(m, INFINITE);}
void release(Mutex& m) {ReleaseMutex(m);}
void destroy_mutex(Mutex& m) {CloseHandle(m);}

class Semaphore {
public:
  enum {MAXCOUNT=2000000000};
  Semaphore(): h(NULL) {}
  void init(int n) {assert(!h); h=CreateSemaphore(NULL, n, MAXCOUNT, NULL);}
  void destroy() {assert(h); CloseHandle(h);}
  int wait() {assert(h); return WaitForSingleObject(h, INFINITE);}
  void signal() {assert(h); ReleaseSemaphore(h, 1, NULL);}
private:
  HANDLE h;  // Windows semaphore
};

#endif

#ifdef _MSC_VER  // Microsoft C++
#define fseeko(a,b,c) _fseeki64(a,b,c)
#define ftello(a) _ftelli64(a)
#else
#ifndef unix
#ifndef fseeko
#define fseeko(a,b,c) fseeko64(a,b,c)
#endif
#ifndef ftello
#define ftello(a) ftello64(a)
#endif
#endif
#endif

// For testing -Dunix in Windows
#ifdef unixtest
#define lstat(a,b) stat(a,b)
#define mkdir(a,b) mkdir(a)
#define fseeko(a,b,c) fseeko64(a,b,c)
#define ftello(a) ftello64(a)
#endif

// signed size of a string or vector
template <typename T> int size(const T& x) {
  return x.size();
}

// In Windows, convert 16-bit wide string to UTF-8 and \ to /
#ifndef unix
string wtou(const wchar_t* s) {
  assert(sizeof(wchar_t)==2);  // Not true in Linux
  assert((wchar_t)(-1)==65535);
  string r;
  if (!s) return r;
  for (; *s; ++s) {
    if (*s=='\\') r+='/';
    else if (*s<128) r+=*s;
    else if (*s<2048) r+=192+*s/64, r+=128+*s%64;
    else r+=224+*s/4096, r+=128+*s/64%64, r+=128+*s%64;
  }
  return r;
}

// In Windows, convert UTF-8 string to wide string ignoring
// invalid UTF-8 or >64K. If doslash then convert "/" to "\".
std::wstring utow(const char* ss, bool doslash=false) {
  assert(sizeof(wchar_t)==2);
  assert((wchar_t)(-1)==65535);
  std::wstring r;
  if (!ss) return r;
  const unsigned char* s=(const unsigned char*)ss;
  for (; s && *s; ++s) {
    if (s[0]=='/' && doslash) r+='\\';
    else if (s[0]<128) r+=s[0];
    else if (s[0]>=192 && s[0]<224 && s[1]>=128 && s[1]<192)
      r+=(s[0]-192)*64+s[1]-128, ++s;
    else if (s[0]>=224 && s[0]<240 && s[1]>=128 && s[1]<192
             && s[2]>=128 && s[2]<192)
      r+=(s[0]-224)*4096+(s[1]-128)*64+s[2]-128, s+=2;
  }
  return r;
}
#endif

// Print a UTF-8 string so it displays properly
void printUTF8(const char* s, FILE* f=stdout) {
  assert(f);
  assert(s);
#ifdef unix
  fprintf(f, "%s", s);
#else
  const HANDLE h=(HANDLE)_get_osfhandle(_fileno(f));
  DWORD ft=GetFileType(h);
  if (ft==FILE_TYPE_CHAR) {
    fflush(f);
    std::wstring w=utow(s);  // Windows console: convert to UTF-16
    DWORD n=0;
    WriteConsole(h, w.c_str(), w.size(), &n, 0);
  }
  else  // stdout redirected to file
    fprintf(f, "%s", s);
#endif
}

// Return relative time in milliseconds
int64_t mtime() {
#ifdef unix
  timeval tv;
  gettimeofday(&tv, 0);
  return tv.tv_sec*1000LL+tv.tv_usec/1000;
#else
  return clock()*1000.0/CLOCKS_PER_SEC;
#endif
}

// Convert 64 bit decimal YYYYMMDDHHMMSS to "YYYY-MM-DD HH:MM:SS"
// where -1 = unknown date, 0 = deleted.
string dateToString(int64_t date) {
  if (date<=0) return "                   ";
  string s="0000-00-00 00:00:00";
  static const int t[]={18,17,15,14,12,11,9,8,6,5,3,2,1,0};
  for (int i=0; i<14; ++i) s[t[i]]+=int(date%10), date/=10;
  return s;
}

// Convert 'u'+(N*256) to octal N or 'w'+(N*256) to hex N or "DRASHI"
string attrToString(int64_t attrib) {
  string r="      ";
  if ((attrib&255)=='u') {
    for (int i=0; i<6; ++i)
      r[5-i]=(attrib>>(8+3*i))%8+'0';
  }
  else if ((attrib&255)=='w') {
    attrib>>=8;
    if (attrib&~0x20b7) {  // non-standard flags set?
      r="0x    ";
      for (int i=0; i<4; ++i)
        r[5-i]="0123456789abcdef"[attrib>>(4*i)&15];
      if (attrib>0x10000) {
        r="0x        ";
        for (int i=0; i<8; ++i)
          r[9-i]="0123456789abcdef"[attrib>>(4*i)&15];
      }
    }
    else {
      r="......";
      if (attrib&0x10) r[0]='D';  // directory
      if (attrib&0x20) r[1]='A';  // archive
      if (attrib&0x04) r[2]='S';  // system
      if (attrib&0x02) r[3]='H';  // hidden
      if (attrib&0x01) r[4]='R';  // read only
      if (attrib&0x2000) r[5]='I';  // index
    }
  }
  return r;
}

// Convert seconds since 0000 1/1/1970 to 64 bit decimal YYYYMMDDHHMMSS
// Valid from 1970 to 2099.
int64_t decimal_time(time_t t) {
  if (t<=0) return -1;
  const int second=t%60;
  const int minute=t/60%60;
  const int hour=t/3600%24;
  t/=86400;  // days since Jan 1 1970
  const int term=t/1461;  // 4 year terms since 1970
  t%=1461;
  t+=(t>=59);  // insert Feb 29 on non leap years
  t+=(t>=425);
  t+=(t>=1157);
  const int year=term*4+t/366+1970;  // actual year
  t%=366;
  t+=(t>=60)*2;  // make Feb. 31 days
  t+=(t>=123);   // insert Apr 31
  t+=(t>=185);   // insert June 31
  t+=(t>=278);   // insert Sept 31
  t+=(t>=340);   // insert Nov 31
  const int month=t/31+1;
  const int day=t%31+1;
  return year*10000000000LL+month*100000000+day*1000000
         +hour*10000+minute*100+second;
}

// Convert decimal date to time_t - inverse of decimal_time()
time_t unix_time(int64_t date) {
  if (date<=0) return -1;
  static const int days[12]={0,31,59,90,120,151,181,212,243,273,304,334};
  const int year=date/10000000000LL%10000;
  const int month=(date/100000000%100-1)%12;
  const int day=date/1000000%100;
  const int hour=date/10000%100;
  const int min=date/100%100;
  const int sec=date%100;
  return (day-1+days[month]+(year%4==0 && month>1)+((year-1970)*1461+1)/4)
    *86400+hour*3600+min*60+sec;
}

/////////////////////////////// File //////////////////////////////////

// Return true if a file or directory (UTF-8 without trailing /) exists
bool exists(string filename) {
  int len=filename.size();
  if (len<1) return false;
  if (filename[len-1]=='/') filename=filename.substr(0, len-1);
#ifdef unix
  struct stat sb;
  return !lstat(filename.c_str(), &sb);
#else
  return GetFileAttributes(utow(filename.c_str()).c_str())
         !=INVALID_FILE_ATTRIBUTES;
#endif
}

// Base class of InputFile and OutputFile (OS independent)
class File {
protected:
  enum {BUFSIZE=1<<16};  // buffer size
  int ptr;  // next byte to read or write in buf
  libzpaq::Array<char> buf;  // I/O buffer
  File(): ptr(0), buf(BUFSIZE) {}
};

// File types accepting UTF-8 filenames
#ifdef unix

class InputFile: public File, public libzpaq::Reader {
  FILE* in;
  int n;  // number of bytes in buf
public:
  InputFile(): in(0), n(0) {}

  // Open file for reading. Return true if successful
  bool open(const char* filename) {
    in=fopen(filename, "rb");
    if (!in) perror(filename);
    n=ptr=0;
    return in!=0;
  }

  // True if open
  bool isopen() {return in!=0;}

  // Read and return 1 byte (0..255) or EOF
  int get() {
    assert(in);
    if (ptr>=n) {
      assert(ptr==n);
      n=fread(&buf[0], 1, BUFSIZE, in);
      ptr=0;
      if (!n) return EOF;
    }
    assert(ptr<n);
    return buf[ptr++]&255;
  }

  // Return file position
  int64_t tell() {
    return ftello(in)-n+ptr;
  }

  // Set file position
  void seek(int64_t pos, int whence) {
    if (whence==SEEK_CUR) {
      whence=SEEK_SET;
      pos+=tell();
    }
    fseeko(in, pos, whence);
    n=ptr=0;
  }

  // Close file if open
  void close() {if (in) fclose(in), in=0;}
  ~InputFile() {close();}
};

class OutputFile: public File, public libzpaq::Writer {
  FILE* out;
  string filename;
public:
  OutputFile(): out(0) {}

  // Return true if file is open
  bool isopen() {return out!=0;}

  // Open for append/update or create if needed.
  bool open(const char* filename) {
    assert(!isopen());
    ptr=0;
    this->filename=filename;
    out=fopen(filename, "rb+");
    if (out) fseeko(out, 0, SEEK_END);
    else out=fopen(filename, "wb+");
    if (!out) perror(filename);
    return isopen();
  }

  // Flush pending output
  void flush() {
    if (ptr) {
      assert(isopen());
      assert(ptr>0 && ptr<=BUFSIZE);
      int n=fwrite(&buf[0], 1, ptr, out);
      if (n!=ptr) {
        perror(filename.c_str());
        error("write failed");
      }
      ptr=0;
    }
  }

  // Write 1 byte
  void put(int c) {
    assert(isopen());
    if (ptr>=BUFSIZE) {
      assert(ptr==BUFSIZE);
      flush();
    }
    assert(ptr>=0 && ptr<BUFSIZE);
    buf[ptr++]=c;
  }

  // Write bufp[0..size-1]
  void write(const char* bufp, int size);

  // Write size bytes at offset
  void write(const char* bufp, int64_t pos, int size) {
    assert(isopen());
    flush();
    fseeko(out, pos, SEEK_SET);
    write(bufp, size);
  }

  // Seek to pos. whence is SEEK_SET, SEEK_CUR, or SEEK_END
  void seek(int64_t pos, int whence) {
    assert(isopen());
    flush();
    fseeko(out, pos, whence);
  }

  // return position
  int64_t tell() {
    assert(isopen());
    return ftello(out)+ptr;
  }

  // Truncate file and move file pointer to end
  void truncate(int64_t newsize=0) {
    assert(isopen());
    seek(newsize, SEEK_SET);
    if (ftruncate(fileno(out), newsize)) perror("ftruncate");
  }

  // Close file and set date if not 0. Set permissions if attr low byte is 'u'
  void close(int64_t date=0, int64_t attr=0) {
    if (out) {
      flush();
      fclose(out);
    }
    out=0;
    if (date>0) {
      struct utimbuf ub;
      ub.actime=time(NULL);
      ub.modtime=unix_time(date);
      utime(filename.c_str(), &ub);
    }
    if ((attr&255)=='u')
      chmod(filename.c_str(), attr>>8);
  }

  ~OutputFile() {close();}
};

#else  // Windows

// Print error message
void winError(const char* filename) {
  int err=GetLastError();
  printUTF8(filename, stderr);
  if (err==ERROR_FILE_NOT_FOUND)
    fprintf(stderr, ": file not found\n");
  else if (err==ERROR_PATH_NOT_FOUND)
    fprintf(stderr, ": path not found\n");
  else if (err==ERROR_ACCESS_DENIED)
    fprintf(stderr, ": access denied\n");
  else if (err==ERROR_SHARING_VIOLATION)
    fprintf(stderr, ": sharing violation\n");
  else if (err==ERROR_BAD_PATHNAME)
    fprintf(stderr, ": bad pathname\n");
  else
    fprintf(stderr, ": Windows error %d\n", err);
}

// Set the last-modified date of an open file handle
void setDate(HANDLE out, int64_t date) {
  if (date>0) {
    SYSTEMTIME st;
    FILETIME ft;
    st.wYear=date/10000000000LL%10000;
    st.wMonth=date/100000000%100;
    st.wDayOfWeek=0;  // ignored
    st.wDay=date/1000000%100;
    st.wHour=date/10000%100;
    st.wMinute=date/100%100;
    st.wSecond=date%100;
    st.wMilliseconds=0;
    SystemTimeToFileTime(&st, &ft);
    if (!SetFileTime(out, NULL, NULL, &ft))
      fprintf(stderr, "SetFileTime error %d\n", int(GetLastError()));
  }
}

class InputFile: public File, public libzpaq::Reader {
  HANDLE in;  // input file handle
  DWORD n;    // buffer size
public:
  InputFile():
    in(INVALID_HANDLE_VALUE), n(0) {}

  // Open for reading. Return true if successful
  bool open(const char* filename) {
    assert(in==INVALID_HANDLE_VALUE);
    n=ptr=0;
    std::wstring w=utow(filename, true);
    in=CreateFile(w.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL,
                  OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (in==INVALID_HANDLE_VALUE) winError(filename);
    return in!=INVALID_HANDLE_VALUE;
  }

  bool isopen() {return in!=INVALID_HANDLE_VALUE;}

  // Read 1 byte
  int get() {
    if (ptr>=int(n)) {
      assert(ptr==int(n));
      ptr=0;
      ReadFile(in, &buf[0], BUFSIZE, &n, NULL);
      if (n==0) return EOF;
    }
    assert(ptr<int(n));
    return buf[ptr++]&255;
  }

  // set file pointer
  void seek(int64_t pos, int whence) {
    if (whence==SEEK_SET) whence=FILE_BEGIN;
    else if (whence==SEEK_END) whence=FILE_END;
    else if (whence==SEEK_CUR) {
      whence=FILE_BEGIN;
      pos+=tell();
    }
    LONG offhigh=pos>>32;
    SetFilePointer(in, pos, &offhigh, whence);
    n=ptr=0;
  }

  // get file pointer
  int64_t tell() {
    LONG offhigh=0;
    DWORD r=SetFilePointer(in, 0, &offhigh, FILE_CURRENT);
    return (int64_t(offhigh)<<32)+r+ptr-n;
  }

  // Close handle if open
  void close() {
    if (in!=INVALID_HANDLE_VALUE) {
      CloseHandle(in);
      in=INVALID_HANDLE_VALUE;
    }
  }
  ~InputFile() {close();}
};

class OutputFile: public File, public libzpaq::Writer {
  HANDLE out;               // output file handle
  std::wstring filename;    // filename as wide string
public:
  OutputFile(): out(INVALID_HANDLE_VALUE) {}

  // Return true if file is open
  bool isopen() {
    return out!=INVALID_HANDLE_VALUE;
  }

  // Open file ready to update or append, create if needed.
  bool open(const char* filename_) {
    assert(!isopen());
    ptr=0;
    filename=utow(filename_, true);
    out=CreateFile(filename.c_str(), GENERIC_WRITE, 0, NULL, OPEN_ALWAYS,
                   FILE_ATTRIBUTE_NORMAL, NULL);
    if (out==INVALID_HANDLE_VALUE) winError(filename_);
    else {
      LONG hi=0;
      SetFilePointer(out, 0, &hi, FILE_END);
    }
    return isopen();
  }

  // Write pending output
  void flush() {
    assert(isopen());
    if (ptr) {
      DWORD n=0;
      WriteFile(out, &buf[0], ptr, &n, NULL);
      if (ptr!=int(n)) {
        fprintf(stderr, "%s: error %d: wrote %d of %d bytes\n",
                wtou(filename.c_str()).c_str(), int(GetLastError()),
                int(n), ptr);
        error("write failed");
      }
      ptr=0;
    }
  }

  // Write 1 byte
  void put(int c) {
    assert(isopen());
    if (ptr>=BUFSIZE) {
      assert(ptr==BUFSIZE);
      flush();
    }
    buf[ptr++]=c;
  }

  // Write bufp[0..size-1]
  void write(const char* bufp, int size);

  // Write size bytes at offset
  void write(const char* bufp, int64_t pos, int size) {
    assert(isopen());
    flush();
    if (pos!=tell()) seek(pos, SEEK_SET);
    write(bufp, size);
  }

  // set file pointer
  void seek(int64_t pos, int whence) {
    if (whence==SEEK_SET) whence=FILE_BEGIN;
    else if (whence==SEEK_CUR) whence=FILE_CURRENT;
    else if (whence==SEEK_END) whence=FILE_END;
    flush();
    LONG offhigh=pos>>32;
    SetFilePointer(out, pos, &offhigh, whence);
  }

  // get file pointer
  int64_t tell() {
    LONG offhigh=0;
    DWORD r=SetFilePointer(out, 0, &offhigh, FILE_CURRENT);
    return (int64_t(offhigh)<<32)+r+ptr;
  }

  // Truncate file and move file pointer to end
  void truncate(int64_t newsize=0) {
    seek(newsize, SEEK_SET);
    SetEndOfFile(out);
  }

  // Close file and set date if not 0. Set attr if low byte is 'w'.
  void close(int64_t date=0, int64_t attr=0) {
    if (isopen()) {
      flush();
      setDate(out, date);
      CloseHandle(out);
      out=INVALID_HANDLE_VALUE;
      if ((attr&255)=='w')
        SetFileAttributes(filename.c_str(), attr>>8);
      filename=L"";
    }
  }
  ~OutputFile() {close();}
};

#endif

// Write bufp[0..size-1]
void OutputFile::write(const char* bufp, int size) {
  if (ptr==BUFSIZE) flush();
  while (size>0) {
    assert(ptr>=0 && ptr<BUFSIZE);
    int n=BUFSIZE-ptr;  // number of bytes to copy to buf
    if (n>size) n=size;
    memcpy(&buf[ptr], bufp, n);
    size-=n;
    bufp+=n;
    ptr+=n;
    if (ptr==BUFSIZE) flush();
  }
}

// Count bytes written and discard them
struct Counter: public libzpaq::Writer {
  int64_t pos;  // count of written bytes
  Counter(): pos(0) {}
  void put(int c) {++pos;}
  void write(const char* bufp, int size) {pos+=size;}
};

///////////////////////// NumberOfProcessors ///////////////////////////

// Guess number of cores
int numberOfProcessors() {
  int rc=0;  // result
#ifdef unix

  // Count lines of the form "processor\t: %d\n" in /proc/cpuinfo
  // where %d is 0, 1, 2,..., rc-1
  FILE *in=fopen("/proc/cpuinfo", "r");
  if (!in) return 1;
  std::string s;
  int c;
  while ((c=getc(in))!=EOF) {
    if (c>='A' && c<='Z') c+='a'-'A';  // convert to lowercase
    if (c>' ') s+=c;  // remove white space
    if (c=='\n') {  // end of line?
      if (size(s)>10 && s.substr(0, 10)=="processor:") {
        c=atoi(s.c_str()+10);
        if (c==rc) ++rc;
      }
      s="";
    }
  }
  fclose(in);
#else

  // In Windows return %NUMBER_OF_PROCESSORS%
  const char* p=getenv("NUMBER_OF_PROCESSORS");
  if (p) rc=atoi(p);
#endif
  if (rc<1) rc=1;
  return rc;
}

////////////////////////////// StringBuffer //////////////////////////

Mutex global_mutex;  // lock for large realloc()

// For libzpaq output to a string
struct StringWriter: public libzpaq::Writer {
  string s;
  void put(int c) {s+=char(c);}
};

// For (de)compressing to/from a string. Writing appends bytes
// which can be later read.
class StringBuffer: public libzpaq::Reader, public libzpaq::Writer {
  unsigned char* p;  // allocated memory, not NUL terminated
  size_t al;         // number of bytes allocated, al > 0
  size_t wpos;       // index of next byte to write, wpos < al
  size_t rpos;       // index of next byte to read, rpos < wpos or return EOF.
  size_t limit;      // max size, default = -1

  // Increase capacity to a without changing size
  void reserve(size_t a) {
    if (a<=al) return;
    if (a>=(1u<<26)) lock(global_mutex);
    unsigned char* q=(unsigned char*)realloc(p, a);
    if (a>=(1u<<26)) release(global_mutex);
    if (!q) {
      fprintf(stderr, "StringBuffer realloc %1.0f to %1.0f at %p failed\n",
          double(al), double(a), p);
      error("Out of memory");
    }
    p=q;
    al=a;
  }

  // Enlarge al to make room to write at least n bytes.
  void lengthen(unsigned n) {
    assert(p);
    assert(wpos<al);
    if (wpos+n>limit) error("StringBuffer overflow");
    if (wpos+n<al) return;
    size_t a=al;
    while (wpos+n>=a) a=a*2+128;
    reserve(a);
  }

  // No assignment or copy
  void operator=(const StringBuffer&);
  StringBuffer(const StringBuffer&);

public:

  // Direct access to data
  unsigned char* data() {return p;}

  // Allocate n bytes initially and make the size 0. More memory will
  // be allocated later if needed.
  StringBuffer(size_t n=0): al(n), wpos(0), rpos(0), limit(size_t(-1)) {
    if (al<128) al=128;
    p=(unsigned char*)malloc(al);
    if (!p) {
      fprintf(stderr, "StringBuffer malloc(%1.0f) failed\n", double(al));
      error("Out of memory");
    }
  }

  // Set output limit
  void setLimit(size_t n) {limit=n;}

  // Free memory
  ~StringBuffer() {free(p);}

  // Return number of bytes written.
  size_t size() const {return wpos;}

  // Return number of bytes left to read
  size_t remaining() const {return wpos-rpos;}

  // Reset size to 0.
  void reset() {rpos=wpos=0;}

  // Write a single byte.
  void put(int c) {  // write 1 byte
    lengthen(1);
    p[wpos++]=c;
  }

  // Write buf[0..n-1]
  void write(const char* buf, int n) {
    lengthen(n);
    memcpy(p+wpos, buf, n);
    wpos+=n;
  }

  // Read a single byte. Return EOF (-1) and reset at end of string.
  int get() {return rpos<wpos ? p[rpos++] : (reset(),-1);}

  // Read up to n bytes into buf[0..] or fewer if EOF is first.
  // Return the number of bytes actually read.
  int read(char* buf, int n) {
    if (rpos+n>wpos) n=wpos-rpos;
    if (n>0) memcpy(buf, p+rpos, n);
    rpos+=n;
    return n;
  }

  // Return the entire string as a read-only array.
  const char* c_str() const {return (const char*)p;}

  // Truncate the string to size i.
  void resize(size_t i) {wpos=i;}

  // Write a string.
  void operator+=(const string& t) {write(t.data(), t.size());}

  // Swap efficiently
  void swap(StringBuffer& s) {
    std::swap(p, s.p);
    std::swap(al, s.al);
    std::swap(wpos, s.wpos);
    std::swap(rpos, s.rpos);
    std::swap(limit, s.limit);
  }
};

// In Windows convert upper case to lower case.
inline int tolowerW(int c) {
#ifndef unix
  if (c>='A' && c<='Z') return c-'A'+'a';
#endif
  return c;
}

// Return true if path a is a prefix of path b.
// Match ? in a to any char in b except / or NUL
// Match * in a to any string in b up to / or NUL
// Otherwise a must match b up to a / or NUL in b.
// In Windows, not case sensitive.
bool ispath(const char* a, const char* b) {
  for (;*a && *b; ++a, ++b) {
    const int ca=tolowerW(*a);
    const int cb=tolowerW(*b);
    if (ca==0 || (ca=='/' && !a[1]))
      return cb==0 || cb=='/';
    else if (ca=='?') {
      if (cb==0 || cb=='/') return false;
    }
    else if (ca=='*') {
      while (true) {
        if (ispath(a+1, b)) return true;
        if (!*b) return false;
        if (*b=='/') return false;
        ++b;
      }
    }
    else if (ca!=cb)
      return false;
  }
  if (*a) return false;
  return *b==0 || *b=='/';
}

// Convert string to lower case
string lowercase(string s) {
  for (unsigned i=0; i<s.size(); ++i)
    if (s[i]>='A' && s[i]<='Z') s[i]+='a'-'A';
  return s;
}

// Read 4 byte little-endian int and advance s
int btoi(const char* &s) {
  s+=4;
  return (s[-4]&255)|((s[-3]&255)<<8)|((s[-2]&255)<<16)|((s[-1]&255)<<24);
}

// Read 8 byte little-endian int and advance s
int64_t btol(const char* &s) {
  int64_t r=unsigned(btoi(s));
  return r+(int64_t(btoi(s))<<32);
}

// Convert x to 4 byte little-endian string
string itob(unsigned x) {
  string s(4, '\0');
  s[0]=x, s[1]=x>>8, s[2]=x>>16, s[3]=x>>24;
  return s;
}

// convert to 8 byte little-endian string
string ltob(int64_t x) {
  string s(8, '\0');
  s[0]=x,     s[1]=x>>8,  s[2]=x>>16, s[3]=x>>24;
  s[4]=x>>32, s[5]=x>>40, s[6]=x>>48, s[7]=x>>56;
  return s;
}

// Convert decimal, octal (leading o) or hex (leading x) string to int
int ntoi(const char* s) {
  int n=0, base=10, sign=1;
  for (; *s; ++s) {
    int c=*s;
    if (isupper(c)) c=tolower(c);
    if (!n && c=='x') base=16;
    else if (!n && c=='o') base=8;
    else if (!n && c=='-') sign=-1;
    else if (c>='0' && c<='9') n=n*base+c-'0';
    else if (base==16 && c>='a' && c<='f') n=n*base+c-'a'+10;
    else break;
  }
  return n*sign;
}

// Convert non-negative decimal number x to string of at least n digits
string itos(int64_t x, int n=1) {
  assert(x>=0);
  assert(n>=0);
  string r;
  for (; x || n>0; x/=10, --n) r=char('0'+x%10)+r;
  return r;
}

/////////////////////////////// Jidac /////////////////////////////////

// A Jidac object represents an archive contents: a list of file
// fragments with hash, size, and archive offset, and a list of
// files with date, attributes, and list of fragment pointers.
// Methods add to, extract from, compare, and list the archive.

bool fragile=false;  // -fragile option
int64_t quiet=0;  // -quiet option
static const int64_t MAX_QUIET=0x7FFFFFFFFFFFFFFFLL;  // no output but errors

// enum for HT::csize
static const int64_t EXTRACTED= 0x7FFFFFFFFFFFFFFELL;  // decompressed?
static const int64_t HT_BAD=    0x7FFFFFFFFFFFFFFALL;  // no such frag

// fragment hash table entry
struct HT {
  unsigned char sha1[20];  // fragment hash
  int usize;      // uncompressed size, -1 if unknown
  int64_t csize;  // if >=0 then block offset else -fragment number
  HT(const char* s=0, int u=-1, int64_t c=HT_BAD) {
    if (s) memcpy(sha1, s, 20);
    else memset(sha1, 0, 20);
    usize=u; csize=c;
  }
};

// filename version entry
struct DTV {
  int64_t date;          // decimal YYYYMMDDHHMMSS (UT) or 0 if deleted
  int64_t size;          // size or -1 if unknown
  int64_t attr;          // first 8 attribute bytes
  vector<unsigned> ptr;  // list of fragment indexes to HT
  int version;           // which transaction was it added?
  DTV(): date(0), size(0), attr(0), version(0) {}
};

// filename entry
struct DT {
  int64_t edate;         // date of external file, 0=not found
  int64_t esize;         // size of external file
  int64_t eattr;         // external file attributes ('u' or 'w' in low byte)
  vector<unsigned> eptr; // fragment list of external file to add
  vector<DTV> dtv;       // list of versions
  int written;           // 0..ptr.size() = fragments output. -1=ignore
  DT(): edate(0), esize(0), eattr(0), written(-1) {}
};

// Version info
struct VER {
  int64_t date;          // 0 if not JIDAC
  int64_t usize;         // uncompressed size of files
  int64_t offset;        // start of transaction
  int updates;           // file updates
  int deletes;           // file deletions
  int firstFragment;     // first fragment ID
  VER() {memset(this, 0, sizeof(*this));}
};

typedef map<string, DT> DTMap;
class CompressJob;

// Do everything
class Jidac {
public:
  int doCommand(int argc, const char** argv);
  friend ThreadReturn decompressThread(void* arg);
  friend ThreadReturn testThread(void* arg);
  friend struct ExtractJob;
private:

  // Command line arguments
  string command;           // "-add", "-extract", "-list", etc.
  string archive;           // archive name
  vector<string> files;     // list of files and directories to add
  vector<string> notfiles;  // list of prefixes to exclude
  vector<string> tofiles;   // files renamed with -to
  int64_t date;             // now as decimal YYYYMMDDHHMMSS (UT)
  int64_t version;          // version number or 14 digit date
  int threads;              // default is number of cores
  int since;                // First version to -list
  int summary;              // Arg to -summary
  string method;            // 0..9, default "1"
  bool force;               // -force option
  bool all;                 // -all option

  // Archive state
  vector<HT> ht;            // list of fragments
  DTMap dt;                 // set of files
  vector<VER> ver;          // version info

  // Commands
  void add();               // add or delete
  int extract();            // extract, return 1 if error else 0
  void list();              // list
  void test();              // test
  void usage();             // help

  // Support functions
  string rename(const string& name);    // replace files prefix with tofiles
  string unrename(const string& name);  // undo rename
  int64_t read_archive(int *errors=0);  // read index block chain
  void read_args(bool scan, bool mark_all=false);  // read args, scan dirs
  void scandir(const char* filename);   // scan dirs and add args to dt
  void addfile(const char* filename, int64_t edate, int64_t esize,
               int64_t eattr);          // add external file to dt
  void list_versions(int64_t csize);    // print ver. csize=archive size
};

// Print help message
void Jidac::usage() {
  printf(
  "zpaq 6.35 - Journaling incremental deduplicating archiving compressor\n"
  "(C) " __DATE__ ", Dell Inc. This is free software under GPL v3.\n"
#ifndef NDEBUG
  "DEBUG version\n"
#endif
  "\n"
  "Usage: command archive.zpaq [file|dir]... -options...\n"
  "Commands:\n"
  "  a  add               Add changed files to archive.zpaq\n"
  "  x  extract           Extract latest versions of files\n"
  "  l  list              List contents\n"
  "  d  delete            Mark as deleted in a new version of archive\n"
  "  t  test              Test archive integrity\n"
  "Options (may be abbreviated):\n"
  "  -not <file|dir>...   Exclude\n"
  "  -to <file|dir>...    Rename external files or specify prefix\n"
  "  -until N|YYYYMMDD[HH[MM[SS]]]    Revert to version number or date\n"
  "  -force               a: Add even if unchanged. x: output clobbers\n"
  "  -quiet [N]           Don't show files smaller than N (default none)\n"
  "  -threads N           Use N threads (default: %d detected)\n"
  "  -method 0...6        Compress faster...better (default: 1)\n"
  "list options:\n"
  "  -summary [N]         Show top N files and types (default: 20)\n"
  "  -since N             List from N'th update or last -N updates\n"
  "  -all                 List all versions\n"
  "See zpaq.cpp for more options and complete documentation.\n",
  threads);
  exit(1);
}

// Rename name by matching it to a prefix of files[i] and replacing
// the prefix with tofiles[i]. If files but not tofiles is empty
// then append prefix tofiles[0].
string Jidac::rename(const string& name) {
  if (!files.size() && tofiles.size())
    return tofiles[0]+name;
  for (unsigned i=0; i<files.size() && i<tofiles.size(); ++i) {
    const unsigned len=files[i].size();
    if (name.size()>=len && name.substr(0, len)==files[i])
      return tofiles[i]+name.substr(files[i].size());
  }
  return name;
}

// Rename name by matching it to a prefix of tofiles[i] and replacing
// the prefix with files[i]. If files but not tofiles is empty and
// prefix matches tofiles[0] then remove prefix.
string Jidac::unrename(const string& name) {
  if (!files.size() && tofiles.size() && name.size()>=tofiles[0].size()
      && tofiles[0]==name.substr(0, tofiles[0].size()))
    return name.substr(tofiles[0].size());
  for (unsigned i=0; i<files.size() && i<tofiles.size(); ++i) {
    const unsigned len=tofiles[i].size();
    if (name.size()>=len && name.substr(0, len)==tofiles[i])
      return files[i]+name.substr(tofiles[i].size());
  }
  return name;
}

// Expand an abbreviated option (with or without a leading "-")
// or report error if not exactly 1 match. Always expand commands.
string expandOption(const char* opt) {
  const char* opts[]={"list","add","extract","delete","test",
    "method","force","quiet","summary","since","compare",
    "to","not","version","until","threads","all","fragile",0};
  assert(opt);
  if (opt[0]=='-') ++opt;
  const int n=strlen(opt);
  if (n==1 && opt[0]=='x') return "-extract";
  string result;
  for (unsigned i=0; opts[i]; ++i) {
    if (!strncmp(opt, opts[i], n)) {
      if (result!="")
        fprintf(stderr, "Ambiguous: %s\n", opt), exit(1);
      result=string("-")+opts[i];
      if (i<5 && result!="") return result;
    }
  }
  if (result=="")
    fprintf(stderr, "No such option: %s\n", opt), exit(1);
  return result;
}

// Parse the command line. Return 1 if error else 0.
int Jidac::doCommand(int argc, const char** argv) {

  // initialize to default values
  command="";
  quiet=0;
  force=all=fragile=false;
  since=0;
  summary=0;
  version=9999999999999LL;
  threads=0; // 0 = auto-detect
  method="1";  // 0..9
  ht.resize(1);  // element 0 not used
  ver.resize(1); // version 0

  // Get optional options
  for (int i=1; i<argc; ++i) {
    const string opt=expandOption(argv[i]);
    if ((opt=="-add" || opt=="-extract" || opt=="-list" || opt=="-delete"
        || opt=="-test") && i<argc-1 && argv[i+1][0]!='-' && command=="") {
      command=opt;
      archive=argv[++i];
      while (++i<argc && argv[i][0]!='-')
        files.push_back(argv[i]);
      --i;
    }
    else if (opt=="-quiet") {
      quiet=MAX_QUIET;
      if (i<argc-1 && isdigit(argv[i+1][0])) quiet=int64_t(atof(argv[++i]));
    }
    else if (opt=="-force") force=true;
    else if (opt=="-all") all=true;
    else if (opt=="-fragile") fragile=true;
    else if (opt=="-since" && i<argc-1)
      since=atoi(argv[++i]);
    else if (opt=="-summary") {
      summary=20;
      if (i<argc-1 && isdigit(argv[i+1][0])) summary=atoi(argv[++i]);
    }
    else if (opt=="-threads" && i<argc-1) {
      threads=atoi(argv[++i]);
      if (threads<1) threads=1;
    }
    else if (opt=="-to") {
      while (++i<argc && argv[i][0]!='-')
        tofiles.push_back(argv[i]);
      --i;
    }
    else if (opt=="-not") {
      while (++i<argc && argv[i][0]!='-')
        notfiles.push_back(argv[i]);
      --i;
    }
    else if ((opt=="-version" || opt=="-until") && i<argc-1) {
      version=int64_t(atof(argv[++i]));
      if (version>=19000000LL     && version<=29991231LL)
        version=version*100+23;
      if (version>=1900000000LL   && version<=2999123123LL)
        version=version*100+59;
      if (version>=190000000000LL && version<=299912312359LL)
        version=version*100+59;
      if (version>9999999
          && (version<19000101000000LL || version>29991231235959LL)) {
        fprintf(stderr,
           "Version date %1.0f must be 19000101000000 to 29991231235959\n",
            double(version));
        exit(1);
      }
    }
    else if (opt=="-method" && i<argc-1) {
      method=argv[++i];
      if (method=="") usage();
    }
    else usage();
  }

  // Set threads
  if (!threads) threads=numberOfProcessors();

  // Add .zpaq extension to archive
  if (archive!="" && 
      (size(archive)<5 || archive.substr(archive.size()-5)!=".zpaq"))
    archive+=".zpaq";

  // Execute command
  if (command=="-add" || command=="-delete") {
    if (size(files)==0) usage();
    add();
  }
  else if (command=="-list") list();
  else if (command=="-extract") return extract();
  else if (command=="-test") test();
  else usage();
  return 0;
}

// Read archive up to -date into ht, dt, ver. Return place to append.
// If errors is not NULL then set it to number of errors found.
int64_t Jidac::read_archive(int *errors) {
  if (errors) *errors=0;

  // Open archive or archive.zpaq
  InputFile in;
  if (!in.open(archive.c_str()))
    return 0;
  if (quiet<MAX_QUIET) {
    printf("Reading archive ");
    printUTF8(archive.c_str());
    printf("\n");
  }

  // Scan archive contents
  string lastfile=archive; // last named file in streaming format
  if (size(lastfile)>5)
    lastfile=lastfile.substr(0, size(lastfile)-5); // drop .zpaq
  int64_t block_offset=0;  // start of last block of any type
  int64_t data_offset=0;   // start of last block of fragments (type d)
  bool found_data=false;   // exit if nothing found
  bool first=true;         // first segment in archive?
  enum {NORMAL, ERR, RECOVER} pass=NORMAL;  // recover ht from data blocks?
  StringBuffer os(32832);  // decompressed block

  // Detect archive format and read the filenames, fragment sizes,
  // and hashes. In JIDAC format, these are in the index blocks, allowing
  // data to be skipped. Otherwise the whole archive is scanned to get
  // this information from the segment headers and trailers.
  while (true) {
    try {

      // If there is an error in the h blocks, scan a second time in RECOVER
      // mode to recover the redundant fragment data from the d blocks.
      libzpaq::Decompresser d;
      d.setInput(&in);
      if (d.findBlock())
        found_data=true;
      else if (pass==ERR) {
        in.seek(0, SEEK_SET);
        block_offset=0;
        if (!d.findBlock()) break;
        pass=RECOVER;
        if (quiet<MAX_QUIET)
          printf("Attempting to recover fragment tables...\n");
      }
      else
        break;

      // Read the segments in the current block
      StringWriter filename, comment;
      int segs=0;
      while (d.findFilename(&filename)) {
        if (filename.s.size()) {
          for (unsigned i=0; i<filename.s.size(); ++i)
            if (filename.s[i]=='\\') filename.s[i]='/';
          lastfile=filename.s.c_str();
        }
        comment.s="";
        d.readComment(&comment);
        if (quiet<MAX_QUIET && pass!=NORMAL)
          printf("Reading %s %s at %1.0f\n", filename.s.c_str(),
                 comment.s.c_str(), double(block_offset));
        int64_t usize=0;  // read uncompressed size from comment or -1
        int64_t fdate=0;  // read date from filename or -1
        int64_t fattr=0;  // read attributes from comment as wN or uN
        unsigned num=0;   // read fragment ID from filename
        const char* p=comment.s.c_str();
        for (; isdigit(*p); ++p)  // read size
          usize=usize*10+*p-'0';
        if (p==comment.s.c_str()) usize=-1;  // size not found
        for (; *p && fdate<19000000000000LL; ++p)  // read date
          if (isdigit(*p)) fdate=fdate*10+*p-'0';
        if (fdate<19000000000000LL || fdate>=30000000000000LL) fdate=-1;

        // Read the comment attribute wN or uN where N is a number
        int attrchar=0;
        for (; true; ++p) {
          if (*p=='u' || *p=='w') {
            attrchar=*p;
            fattr=0;
          }
          else if (isdigit(*p) && (attrchar=='u' || attrchar=='w'))
            fattr=fattr*10+*p-'0';
          else if (attrchar) {
            fattr=fattr*256+attrchar;
            attrchar=0;
          }
          if (!*p) break;
        }

        // Test for JIDAC format. Filename is jDC<fdate>[cdhi]<num>
        // and comment ends with " jDC\x01"
        if (comment.s.size()>=4
            && usize>=0
            && comment.s.substr(comment.s.size()-4)=="jDC\x01"
            && filename.s.size()==28
            && filename.s.substr(0, 3)=="jDC"
            && strchr("cdhi", filename.s[17])) {

          // Read the date and number in the filename
          num=0;
          fdate=0;
          for (unsigned i=3; i<17 && isdigit(filename.s[i]); ++i)
            fdate=fdate*10+filename.s[i]-'0';
          for (unsigned i=18; i<filename.s.size() && isdigit(filename.s[i]);
               ++i)
            num=num*10+filename.s[i]-'0';

          // Decompress the block. In recovery mode, only decompress
          // data blocks containing missing HT data.
          os.reset();
          os.setLimit(usize);
          d.setOutput(&os);
          libzpaq::SHA1 sha1;
          d.setSHA1(&sha1);
          if (pass!=RECOVER || (filename.s[17]=='d' && num>0 &&
              num<ht.size() && ht[num].csize==HT_BAD)) {
            d.decompress();
            char sha1result[21]={0};
            d.readSegmentEnd(sha1result);
            if (usize!=int64_t(sha1.usize())) {
              fprintf(stderr, "%s size should be %1.0f, is %1.0f\n",
                      filename.s.c_str(), double(usize),
                      double(sha1.usize()));
              error("incorrect block size");
            }
            if (sha1result[0] && memcmp(sha1result+1, sha1.result(), 20)) {
              fprintf(stderr, "%s checksum error\n", filename.s.c_str());
              error("bad checksum");
            }
          }
          else
            d.readSegmentEnd();

          // Transaction header (type c).
          // If in the future then stop here, else read 8 byte data size
          // from input and jump over it.
          if (filename.s[17]=='c' && fdate>=19000000000000LL
              && fdate<30000000000000LL && pass!=RECOVER) {
            data_offset=in.tell()+1;
            bool isbreak=version<19000000000000LL ? size(ver)>version :
                         version<fdate;
            int64_t jmp=0;
            if (!isbreak && os.size()==8) {  // jump
              const char* s=os.c_str();
              jmp=btol(s);
              if (jmp<0) {
                fprintf(stderr, "Incomplete transaction ignored\n");
                isbreak=true;
              }
              else if (jmp>0)
                in.seek(jmp, SEEK_CUR);
            }
            if (os.size()!=8) {
              fprintf(stderr, "Bad JIDAC header size: %d\n", size(os));
              isbreak=true;
              if (*errors) ++*errors;
            }
            if (isbreak) {
              in.close();
              return block_offset;
            }
            ver.push_back(VER());
            ver.back().firstFragment=size(ht);
            ver.back().offset=block_offset;
            ver.back().date=fdate;
          }

          // Fragment table (type d).
          // Contents is bsize[4] (sha1[20] usize[4])... for fragment N...
          // where bsize is the compressed block size.
          // Store in ht[].{sha1,usize}. Set ht[].csize to block offset
          // assuming N in ascending order.
          else if (filename.s[17]=='h' && num>0 && os.size()>=4
                   && pass!=RECOVER) {
            const char* s=os.c_str();
            const unsigned bsize=btoi(s);
            assert(size(ver)>0);
            const unsigned n=(os.size()-4)/24;
            if (ht.size()!=num) {
              fprintf(stderr,
                "Unordered fragment tables: expected %d found %1.0f\n",
                size(ht), double(num));
              pass=ERR;
            }
            for (unsigned i=0; i<n; ++i) {
              while (ht.size()<=num+i) ht.push_back(HT());
              memcpy(ht[num+i].sha1, s, 20);
              s+=20;
              if (ht[num+i].csize!=HT_BAD) error("duplicate fragment ID");
              ht[num+i].usize=btoi(s);
              ht[num+i].csize=i?-int(i):data_offset;
            }
            data_offset+=bsize;
          }

          // Index (type i)
          // Contents is: 0[8] filename 0 (deletion)
          // or:       date[8] filename 0 na[4] attr[na] ni[4] ptr[ni][4]
          // Read into DT
          else if (filename.s[17]=='i' && pass!=RECOVER) {
            const char* s=os.c_str();
            const char* const end=s+os.size();
            while (s<=end-9) {
              const char* fp=s+8;  // filename
              DT& dtr=dt[fp];
              dtr.dtv.push_back(DTV());
              DTV& dtv=dtr.dtv.back();
              dtv.version=size(ver)-1;
              dtv.date=btol(s);
              assert(size(ver)>0);
              if (dtv.date) ++ver.back().updates;
              else ++ver.back().deletes;
              s+=strlen(fp)+1;  // skip filename
              if (dtv.date && s<=end-8) {
                const unsigned na=btoi(s);
                for (unsigned i=0; i<na && s<end; ++i, ++s)  // read attr
                  if (i<8) dtv.attr+=int64_t(*s&255)<<(i*8);
                if (s<=end-4) {
                  const unsigned ni=btoi(s);
                  dtv.ptr.resize(ni);
                  for (unsigned i=0; i<ni && s<=end-4; ++i) {  // read ptr
                    dtv.ptr[i]=btoi(s);
                    if (dtv.ptr[i]<1 || dtv.ptr[i]>=ht.size()+(1<<24))
                      error("bad fragment ID");
                    while (dtv.ptr[i]>=ht.size()) {
                      pass=ERR;
                      ht.push_back(HT());
                    }
                    dtv.size+=ht[dtv.ptr[i]].usize;
                    ver.back().usize+=ht[dtv.ptr[i]].usize;
                  }
                }
              }
            }
          }

          // Recover fragment sizes and hashes from data block
          else if (pass==RECOVER && filename.s[17]=='d' && num>0
                   && num<ht.size()) {
            if (os.size()>=8 && ht[num].csize==HT_BAD) {
              const char* p=os.c_str()+os.size()-8;
              unsigned n=btoi(p);  // first fragment == num or 0
              if (n==0) n=num;
              unsigned f=btoi(p);  // number of fragments
              if (n==num && f && f*4+8<=os.size()) {
                if (quiet<MAX_QUIET)
                  printf("Recovering fragments %d-%d at %1.0f\n",
                         n, n+f-1, double(block_offset));
                while (ht.size()<=n+f) ht.push_back(HT());
                p=os.c_str()+os.size()-8-4*f;

                // read fragment sizes into ht[n..n+f-1].usize
                unsigned sum=0;
                for (unsigned i=0; i<f; ++i) {
                  sum+=ht[n+i].usize=btoi(p);
                  ht[n+i].csize=i ? -int(i) : block_offset;
                }

                // Compute hashes
                if (sum+f*4+8==os.size()) {
                  if (quiet<MAX_QUIET)
                    printf("Computing hashes for %d bytes\n", sum);
                  libzpaq::SHA1 sha1;
                  p=os.c_str();
                  for (unsigned i=0; i<f; ++i) {
                    for (int j=0; j<ht[n+i].usize; ++j) {
                      assert(p<os.c_str()+os.size());
                      sha1.put(*p++);
                    }
                    memcpy(ht[n+i].sha1, sha1.result(), 20);
                  }
                  assert(p==os.c_str()+sum);
                }
              }
            }

            // Correct bad offsets
            assert(num>0 && num<ht.size());
            if (quiet<MAX_QUIET && ht[num].csize!=block_offset) {
              printf("Changing block %d offset from %1.0f to %1.0f\n",
                     num, double(ht[num].csize), double(block_offset));
              ht[num].csize=block_offset;
            }
          }

          // Bad JIDAC block
          else if (pass!=RECOVER) {
            fprintf(stderr, "Bad JIDAC block ignored: %s %s\n",
                    filename.s.c_str(), comment.s.c_str());
            if (errors) ++*errors;
          }
        }

        // Streaming format
        else if (pass!=RECOVER) {

          // If previous version is dated or does not exist, start a new one
          if (segs==0 && (size(ver)==1 || ver.back().date!=0)) {
            if (size(ver)>version) {
              in.close();
              return block_offset;
            }
            ver.push_back(VER());
            ver.back().firstFragment=size(ht);
            ver.back().offset=block_offset;
          }

          char sha1result[21]={0};
          d.readSegmentEnd(sha1result);
          DT& dtr=dt[lastfile];
          if (filename.s.size()>0 || first) {
            dtr.dtv.push_back(DTV());
            dtr.dtv.back().date=fdate;
            dtr.dtv.back().attr=fattr;
            dtr.dtv.back().version=size(ver)-1;
            ++ver.back().updates;
          }
          assert(dtr.dtv.size()>0);
          dtr.dtv.back().ptr.push_back(size(ht));
          if (usize>=0 && dtr.dtv.back().size>=0) dtr.dtv.back().size+=usize;
          else dtr.dtv.back().size=-1;
          if (usize>=0) ver.back().usize+=usize;
          ht.push_back(HT(sha1result+1, usize>0x7fffffff ? -1 : usize,
                          segs ? -segs : block_offset));
          assert(size(ver)>0);
        }
        ++segs;
        filename.s="";
        first=false;
      }
      block_offset=in.tell();
    }
    catch (std::exception& e) {
      block_offset=in.tell();
      fprintf(stderr, "Skipping block at %1.0f: %s\n", double(block_offset),
              e.what());
      if (errors) ++*errors;
    }
  }
  if (in.tell()>0 && !found_data) error("archive contains no data");
  in.close();

  // Recompute file sizes in recover mode
  if (pass==RECOVER) {
    fprintf(stderr, "Recomputing file sizes\n");
    for (DTMap::iterator p=dt.begin(); p!=dt.end(); ++p) {
      for (unsigned i=0; i<p->second.dtv.size(); ++i) {
        p->second.dtv[i].size=0;
        for (unsigned j=0; j<p->second.dtv[i].ptr.size(); ++j) {
          unsigned k=p->second.dtv[i].ptr[j];
          if (k>0 && k<ht.size())
            p->second.dtv[i].size+=ht[k].usize;
        }
      }
    }
  }
  return block_offset;
}

// Mark each file in dt that matches the command args
// (in files[]) and not matched to -not (in notfiles[])
// using written=0 for each match. Match all files in dt if no args
// (files[] is empty). If mark_all is true, then mark deleted files too.
// If scan is true then recursively scan external directories in args,
// add to dt, and mark them.
void Jidac::read_args(bool scan, bool mark_all) {

  // Match to files[] except notfiles[] or match all if files[] is empty
  if (quiet<MAX_QUIET && scan && size(files)) printf("Scanning files\n");
  for (DTMap::iterator p=dt.begin(); p!=dt.end(); ++p) {
    if (p->second.dtv.size()<1) {
      fprintf(stderr, "Invalid index entry: %s\n", p->first.c_str());
      error("corrupted index");
    }
    bool matched=size(files)==0;
    for (int i=0; !matched && i<size(files); ++i)
      if (ispath(files[i].c_str(), p->first.c_str()))
        matched=true;
    for (int i=0; matched && i<size(notfiles); ++i)
      if (ispath(notfiles[i].c_str(), p->first.c_str()))
        matched=false;
    if (matched &&
        (mark_all || (p->second.dtv.size() && p->second.dtv.back().date)))
      p->second.written=0;
  }

  // Scan external files and directories, insert into dt and mark written=0
  if (scan)
    for (int i=0; i<size(files); ++i)
      scandir(rename(files[i]).c_str());
}

// Return the part of fn up to the last slash
string path(const string& fn) {
  int n=0;
  for (int i=0; fn[i]; ++i)
    if (fn[i]=='/' || fn[i]=='\\') n=i+1;
  return fn.substr(0, n);
}

// Insert filename (UTF-8 with "/") into dt unless in notfiles.
// If filename is a directory then also insert its contents.
// In Windows, filename might have wildcards like "file.*" or "dir/*"
void Jidac::scandir(const char* filename) {

#ifdef unix

  // Omit if in notfiles
  for (int i=0; i<size(notfiles); ++i)
    if (ispath(notfiles[i].c_str(), unrename(filename).c_str())) return;

  // Add regular files and directories
  struct stat sb;
  if (!lstat(filename, &sb)) {
    if (S_ISREG(sb.st_mode))
      addfile(filename, decimal_time(sb.st_mtime), sb.st_size,
              'u'+(sb.st_mode<<8));

    // Traverse directory
    if (S_ISDIR(sb.st_mode)) {
      addfile((string(filename)+"/").c_str(),
              decimal_time(sb.st_mtime), sb.st_size,
              'u'+(sb.st_mode<<8));
      DIR* dirp=opendir(filename);
      if (dirp) {
        for (dirent* dp=readdir(dirp); dp; dp=readdir(dirp)) {
          if (strcmp(".", dp->d_name) && strcmp("..", dp->d_name)) {
            string s=filename;
            int len=s.size();
            if (len>0 && s[len-1]!='/' && s[len-1]!='\\') s+="/";
            s+=dp->d_name;
            scandir(s.c_str());
          }
        }
        closedir(dirp);
      }
      else
        perror(filename);
    }
  }
  else
    perror(filename);

#else  // Windows: expand wildcards in filename

  // Expand wildcards
  WIN32_FIND_DATA ffd;
  string t=filename;
  if (t.size()>0 && t[t.size()-1]=='/') t+="*";
  HANDLE h=FindFirstFile(utow(t.c_str()).c_str(), &ffd);
  if (h==INVALID_HANDLE_VALUE)
    winError(t.c_str());
  while (h!=INVALID_HANDLE_VALUE) {

    // For each file, get name, date, size, attributes
    SYSTEMTIME st;
    int64_t edate=0;
    if (FileTimeToSystemTime(&ffd.ftLastWriteTime, &st))
      edate=st.wYear*10000000000LL+st.wMonth*100000000LL+st.wDay*1000000
            +st.wHour*10000+st.wMinute*100+st.wSecond;
    const int64_t esize=ffd.nFileSizeLow+(int64_t(ffd.nFileSizeHigh)<<32);
    const int64_t eattr='w'+(int64_t(ffd.dwFileAttributes)<<8);

    // Ignore the names "." and ".." or any path/name in notfiles
    t=wtou(ffd.cFileName);
    if (t=="." || t=="..") edate=0;  // don't add
    string fn=path(filename)+t;
    for (int i=0; i<size(notfiles); ++i)
      if (ispath(notfiles[i].c_str(), unrename(fn).c_str()))
        edate=0;

    // Save directory names with a trailing / and scan their contents
    // Otherwise, save plain files
    if (edate) {
      if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        fn+="/";
      addfile(fn.c_str(), edate, esize, eattr);
      if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        fn+="*";
        scandir(fn.c_str());
      }
    }
    if (!FindNextFile(h, &ffd)) {
      if (GetLastError()!=ERROR_NO_MORE_FILES)
        winError(fn.c_str());
      break;
    }
  }
  FindClose(h);
#endif
}

// Add external file and its date, size, and attributes to dt
// if eattr satisfies -attribute args.
void Jidac::addfile(const char* filename, int64_t edate,
                    int64_t esize, int64_t eattr) {
  DT& d=dt[unrename(filename)];
  d.edate=edate;
  d.esize=esize;
  d.eattr=eattr;
  d.written=0;
}

/////////////////////////////// add ///////////////////////////////////

// E8E9 transform of buf[0..n-1] to improve compression of .exe and .dll.
// Patterns (E8|E9 xx xx xx 00|FF) at offset i replace the 3 middle
// bytes with x+i mod 2^24, LSB first, reading backward.
void e8e9(unsigned char* buf, int n) {
  for (int i=n-5; i>=0; --i) {
    if (((buf[i]&254)==0xe8) && ((buf[i+4]+1)&254)==0) {
      unsigned a=(buf[i+1]|buf[i+2]<<8|buf[i+3]<<16)+i;
      buf[i+1]=a;
      buf[i+2]=a>>8;
      buf[i+3]=a>>16;
    }
  }
}

// BWT Preprocessor preprocessed with e8e9 if doE8 is true.
// Format is a Burrows-Wheeler transform with the terminal symbol -1
// included and encoded as 255. The location of this symbol (idx) is
// encoded by appending it as 4 bytes LSB first.

class BWTBuffer: public libzpaq::Reader {
  StringBuffer* inp;
public:
  int get() {return inp->get();}
  BWTBuffer(StringBuffer& in, bool doE8);
};

BWTBuffer::BWTBuffer(StringBuffer& in, bool doE8): inp(&in) {
  const int n=in.size();
  libzpaq::Array<int> w(n+1);
  if (doE8) e8e9(in.data(), n);
  int idx=divbwt(in.data(), in.data(), &w[0], n);
  assert(idx>=0 && idx<=n);
  in.put(0);
  memmove(in.data()+idx+1, in.data()+idx, n-idx);
  in.data()[idx]=255;
  for (int i=0; i<4; ++i) in.put(idx>>(i*8));
}

// LZ preprocessor for level 1 or 2 compression and e8e9 filter.
// Level 1 uses variable length LZ77 codes like in the lazy compressor:
//
//   00,n,L[n] = n literal bytes
//   mm,mmm,n,ll,r,q (mm > 00) = match 4*n+ll at offset (q<<rb)+r-1
//
// where q is written in 8mm+mmm-8 (0..23) bits with an implied leading 1 bit
// and n is written using interleaved Elias Gamma coding, i.e. the leading
// 1 bit is implied, remaining bits are preceded by a 1 and terminated by
// a 0. e.g. abc is written 1,b,1,c,0. Codes are packed LSB first and
// padded with leading 0 bits in the last byte. r is a number with rb bits,
// where rb = log2(blocksize) - 24.
//
// Level 2 is byte oriented LZ77 with minimum match length m = $4 = args[3]
// with m in 1..64. Lengths and offsets are MSB first:
// 00xxxxxx   x+1 (1..64) literals follow
// yyxxxxxx   y+1 (2..4) offset bytes follow, match length x+m (m..m+63)

// floor(log2(x)) + 1 = number of bits excluding leading zeros (0..32)
int lg(unsigned x) {
  unsigned r=0;
  if (x>=65536) r=16, x>>=16;
  if (x>=256) r+=8, x>>=8;
  if (x>=16) r+=4, x>>=4;
  assert(x>=0 && x<16);
  return
    "\x00\x01\x02\x02\x03\x03\x03\x03\x04\x04\x04\x04\x04\x04\x04\x04"[x]+r;
}

// return number of 1 bits in x
int nbits(unsigned x) {
  int r;
  for (r=0; x; x>>=1) r+=x&1;
  return r;
}

// Encode inbuf to buf using LZ77. args are as follows:
// args[0] is log2 buffer size in MB.
// args[1] is level (1=variable length, 2=byte aligned lz77) + 4 if E8E9.
// args[2] is the minimum match length and context order.
// args[3] is the higher context order to search first, or else 0.
// args[4] is the log2 hash bucket size (number of searches).
// args[5] is the log2 hash table size.
// args[6] is the log2 min offset to increment min match after a literal.
// args[7] is the log2 min offset to increment min match after a match.

class LZBuffer: public libzpaq::Reader {
  libzpaq::Array<unsigned> ht;// hash table, confirmation in low bits
  const unsigned char* in;    // input pointer
  const int checkbits;        // hash confirmation size
  const int level;            // 1=var length LZ77, 2=byte aligned LZ77
  const unsigned htsize;      // size of hash table
  const unsigned n;           // input length
  unsigned i;                 // current location in in (0 <= i < n)
  const unsigned minMatch;    // minimum match length
  const unsigned minMatch2;   // second context order or 0 if not used
  const unsigned maxMatch;    // longest match length allowed
  const unsigned maxLiteral;  // longest literal length allowed
  unsigned h1, h2;            // low, high order context hashes of in[i..]
  const unsigned bucketbits;  // log bucket
  const unsigned bucket;      // number of matches to search per hash - 1
  const unsigned shift1, shift2;  // how far to shift h1, h2 per hash
  const int minMatchBoth;     // max(minMatch, minMatch2)
  const unsigned rb;          // number of level 1 r bits in match code
  unsigned bits;              // pending output bits (level 1)
  unsigned nbits;             // number of bits in bits
  unsigned rpos, wpos;        // read, write pointers
  enum {BUFSIZE=1<<14};       // output buffer size
  unsigned char buf[BUFSIZE]; // output buffer

  void write_literal(unsigned i, unsigned& lit);
  void write_match(unsigned len, unsigned off);
  void fill();  // encode to buf

  // write k bits of x
  void putb(unsigned x, int k) {
    x&=(1<<k)-1;
    bits|=x<<nbits;
    nbits+=k;
    while (nbits>7) {
      assert(wpos<BUFSIZE);
      buf[wpos++]=bits, bits>>=8, nbits-=8;
    }
  }

  // write last byte
  void flush() {
    assert(wpos<BUFSIZE);
    if (nbits>0) buf[wpos++]=bits;
    bits=nbits=0;
  }

  // write 1 byte
  void put(int c) {
    assert(wpos<BUFSIZE);
    buf[wpos++]=c;
  }

public:
  LZBuffer(StringBuffer& inbuf, int args[]); // input to compress

  // return 1 byte of compressed output (overrides Reader)
  int get() {
    int c=-1;
    if (rpos==wpos) fill();
    if (rpos<wpos) c=buf[rpos++];
    if (rpos==wpos) rpos=wpos=0;
    return c;
  }

  // Read up to p[0..n-1] and return bytes read.
  int read(char* p, int n);
};

// Read n bytes of compressed output into p and return number of
// bytes read in 0..n. 0 signals EOF (overrides Reader).
int LZBuffer::read(char* p, int n) {
  if (rpos==wpos) fill();
  int nr=n;
  if (nr>int(wpos-rpos)) nr=wpos-rpos;
  if (nr) memcpy(p, buf+rpos, nr);
  rpos+=nr;
  assert(rpos<=wpos);
  if (rpos==wpos) rpos=wpos=0;
  return nr;
}

LZBuffer::LZBuffer(StringBuffer& inbuf, int args[]):
    ht(1<<args[5]),
    in(inbuf.data()),
    checkbits(12-args[0]),
    level(args[1]&3),
    htsize(ht.size()),
    n(inbuf.size()),
    i(0),
    minMatch(std::max(args[2], (level==1 ? 4 : 1))),
    minMatch2(args[3]),
    maxMatch(BUFSIZE*3),
    maxLiteral(BUFSIZE/4),
    h1(0), h2(0),
    bucketbits(args[4]), bucket((1<<args[4])-1), 
    shift1(minMatch>0 ? (args[5]-1)/minMatch+1 : 1),
    shift2(minMatch2>0 ? (args[5]-1)/minMatch2+1 : 0),
    minMatchBoth(std::max(minMatch, minMatch2)+4),
    rb(args[0]>4 ? args[0]-4 : 0),
    bits(0),
    nbits(0),
    rpos(0),
    wpos(0) {
  assert(args[0]>=0);
  assert(args[1]==1 || args[1]==2 || args[1]==5 || args[1]==6);
  assert(level==1 || level==2);

  // e8e9 transform
  if (args[1]>4) e8e9(inbuf.data(), n);
}

// Encode from in to buf until end of input or buf is not empty
void LZBuffer::fill() {

  // Scan the input
  unsigned lit=0;  // number of output literals pending
  const unsigned mask=(1<<checkbits)-1;
  while (i<n && wpos*2<BUFSIZE) {

    // Search for longest match, or pick closest in case of tie
    // Try the longest context orders first. If a match is found, then
    // skip the lower order as a speed optimization.
    unsigned blen=minMatch-1;  // best match length
    unsigned bp=0;  // pointer to best match
    unsigned blit=0;  // literals before best match
    int bscore=0;  // best cost
    if (level==1 || minMatch<=64) {
      if (minMatch2>0) {
        for (unsigned k=0; k<=bucket; ++k) {
          unsigned p=ht[h2^k];
          if (p && (p&mask)==(in[i+3]&mask)) {
            p>>=checkbits;
            if (p<i && i+blen<=n && in[p+blen-1]==in[i+blen-1]) {
              unsigned l, l1=(in[p]!=in[i]);
              for (l=1; i+l<n && l<maxMatch && in[p+l]==in[i+l]; ++l);
              if (l>=blen-1) {
                int score=l*8-lg(i-p)-2*(lit>0)-11-l1*(lit?8:11);
                if (score>bscore) blen=l, bp=p, blit=l1, bscore=score;
              }
            }
          }
          if (blen>=128) break;
        }
      }

      // Search the lower order context
      if (!minMatch2 || blen<minMatch2) {
        for (unsigned k=0; k<=bucket; ++k) {
          unsigned p=ht[h1^k];
          if (p && (p&mask)==(in[i+3]&mask)) {
            p>>=checkbits;
            if (p<i && i+blen<=n && in[p+blen-1]==in[i+blen-1]) {
              unsigned l;
              for (l=0; i+l<n && l<maxMatch && in[p+l]==in[i+l]; ++l);
              if (l>=blen-1) {
                int score=l*8-lg(i-p)-2*(lit>0)-11;
                if (score>bscore) blen=l, bp=p, blit=0, bscore=score;
              }
            }
          }
          if (blen>=128) break;
        }
      }
    }

    // If match is long enough, then output any pending literals first,
    // and then the match. blen is the length of the match.
    assert(i>=bp);
    const unsigned off=i-bp;  // offset
    if (off>0 && bscore>0
        && blen-blit>=minMatch+(level==2)*((off>=(1<<16))+(off>=(1<<24)))) {
      lit+=blit;
      write_literal(i+blit, lit);
      write_match(blen-blit, off);
    }

    // Otherwise add to literal length
    else {
      blen=1;
      ++lit;
    }

    // Update index, advance blen bytes
    while (blen--) {
      if (i+minMatchBoth<n) {
        unsigned ih=i&bucket;
        const unsigned p=(i<<checkbits)|(in[i+3]&mask);
        assert(ih<=bucket);
        if (minMatch2) {
          ht[h2^ih]=p;
          h2=(((h2*3)<<shift2)+(in[i+minMatch2+1]+1)*23456789)&(htsize-1);
        }
        ht[h1^ih]=p;
        h1=(((h1*3)<<shift1)+(in[i+minMatch]+1)*123456791)&(htsize-1);
      }
      ++i;
    }

    // Write long literals to keep buf from filling up
    if (lit>=maxLiteral)
      write_literal(i, lit);
  }

  // Write pending literals at end of input
  assert(i<=n);
  if (i==n) {
    write_literal(n, lit);
    flush();
  }
}

// Write literal sequence in[i-lit..i-1], set lit=0
void LZBuffer::write_literal(unsigned i, unsigned& lit) {
  assert(lit>=0);
  assert(i>=0 && i<=n);
  assert(i>=lit);
  if (level==1) {
    if (lit<1) return;
    int ll=lg(lit);
    assert(ll>=1 && ll<=24);
    putb(0, 2);
    --ll;
    while (--ll>=0) {
      putb(1, 1);
      putb((lit>>ll)&1, 1);
    }
    putb(0, 1);
    while (lit) putb(in[i-lit--], 8);
  }
  else {
    assert(level==2);
    while (lit>0) {
      unsigned lit1=lit;
      if (lit1>64) lit1=64;
      put(lit1-1);
      for (unsigned j=i-lit; j<i-lit+lit1; ++j) put(in[j]);
      lit-=lit1;
    }
  }
}

// Write match sequence of given length and offset
void LZBuffer::write_match(unsigned len, unsigned off) {

  // mm,mmm,n,ll,r,q[mmmmm-8] = match n*4+ll, offset ((q-1)<<rb)+r+1
  if (level==1) {
    if (len<minMatch || len>maxMatch) printf("len=%d minMatch=%d maxMatch=%d\n", len, minMatch, maxMatch), exit(1);
    assert(len>=minMatch && len<=maxMatch);
    assert(off>0);
    assert(len>=4);
    assert(rb>=0 && rb<=8);
    int ll=lg(len)-1;
    assert(ll>=2);
    off+=(1<<rb)-1;
    int lo=lg(off)-1-rb;
    assert(lo>=0 && lo<=23);
    putb((lo+8)>>3, 2);// mm
    putb(lo&7, 3);     // mmm
    while (--ll>=2) {  // n
      putb(1, 1);
      putb((len>>ll)&1, 1);
    }
    putb(0, 1);
    putb(len&3, 2);    // ll
    putb(off, rb);     // r
    putb(off>>rb, lo); // q
  }

  // x[2]:len[6] off[x-1] 
  else {
    assert(level==2);
    assert(minMatch>=1 && minMatch<=64);
    --off;
    while (len>0) {  // Split long matches to len1=minMatch..minMatch+63
      const unsigned len1=len>minMatch*2+63 ? minMatch+63 :
          len>minMatch+63 ? len-minMatch : len;
      assert(wpos<BUFSIZE-5);
      assert(len1>=minMatch && len1<minMatch+64);
      if (off<(1<<16)) {
        put(64+len1-minMatch);
        put(off>>8);
        put(off);
      }
      else if (off<(1<<24)) {
        put(128+len1-minMatch);
        put(off>>16);
        put(off>>8);
        put(off);
      }
      else {
        put(192+len1-minMatch);
        put(off>>24);
        put(off>>16);
        put(off>>8);
        put(off);
      }
      len-=len1;
    }
  }
}

// Generate a config file from the method argument with syntax:
// {x|s}[N1[,N2]...][{ciamtswf<cfg>}[N1[,N2]]...]...
// Write the initial args into args[0..8].
string makeConfig(const char* method, int args[]) {
  assert(method);
  assert(method[0]=='x' || method[0]=='s');

  // Read "xN1,N2...N9" into args[0..8] ($1..$9)
  args[0]=4;  // log block size in MB
  args[1]=1;  // lz77 with variable length codes
  args[2]=4;  // minimum match length
  args[3]=0;  // secondary context length
  args[4]=3;  // log searches
  args[5]=24; // lz77 hash table size
  args[6]=16; // offset length to increment min match after literal
  args[7]=18; // offset length to increment min match after match
  args[8]=0;
  if (isdigit(*++method)) args[0]=0;
  for (int i=0; i<9 && (isdigit(*method) || *method==',' || *method=='.');) {
    if (isdigit(*method))
      args[i]=args[i]*10+*method-'0';
    else if (++i<9)
      args[i]=0;
    ++method;
  }

  // Generate the postprocessor
  string hdr, pcomp;
  const int level=args[1]&3;
  const bool doe8=args[1]>=4 && args[1]<=7;

  // LZ77+Huffman, with or without E8E9
  if (level==1) {
    const int rb=args[0]>4 ? args[0]-4 : 0;
    hdr="comp 9 16 0 $1+20 ";
    pcomp=
    "pcomp lazy2 3 ;\n"
    " (r1 = state\n"
    "  r2 = len - match or literal length\n"
    "  r3 = m - number of offset bits expected\n"
    "  r4 = ptr to buf\n"
    "  r5 = r - low bits of offset\n"
    "  c = bits - input buffer\n"
    "  d = n - number of bits in c)\n"
    "\n"
    "  a> 255 if\n";
    if (doe8)
      pcomp+=
      "    b=0 d=r 4 do (for b=0..d-1, d = end of buf)\n"
      "      a=b a==d ifnot\n"
      "        a+= 4 a<d if\n"
      "          a=*b a&= 254 a== 232 if (e8 or e9?)\n"
      "            c=b b++ b++ b++ b++ a=*b a++ a&= 254 a== 0 if (00 or ff)\n"
      "              b-- a=*b\n"
      "              b-- a<<= 8 a+=*b\n"
      "              b-- a<<= 8 a+=*b\n"
      "              a-=b a++\n"
      "              *b=a a>>= 8 b++\n"
      "              *b=a a>>= 8 b++\n"
      "              *b=a b++\n"
      "            endif\n"
      "            b=c\n"
      "          endif\n"
      "        endif\n"
      "        a=*b out b++\n"
      "      forever\n"
      "    endif\n"
      "\n";
    pcomp+=
    "    (reset state)\n"
    "    a=0 b=0 c=0 d=0 r=a 1 r=a 2 r=a 3 r=a 4\n"
    "    halt\n"
    "  endif\n"
    "\n"
    "  a<<=d a+=c c=a               (bits+=a<<n)\n"
    "  a= 8 a+=d d=a                (n+=8)\n"
    "\n"
    "  (if state==0 (expect new code))\n"
    "  a=r 1 a== 0 if (match code mm,mmm)\n"
    "    a= 1 r=a 2                 (len=1)\n"
    "    a=c a&= 3 a> 0 if          (if (bits&3))\n"
    "      a-- a<<= 3 r=a 3           (m=((bits&3)-1)*8)\n"
    "      a=c a>>= 2 c=a             (bits>>=2)\n"
    "      b=r 3 a&= 7 a+=b r=a 3     (m+=bits&7)\n"
    "      a=c a>>= 3 c=a             (bits>>=3)\n"
    "      a=d a-= 5 d=a              (n-=5)\n"
    "      a= 1 r=a 1                 (state=1)\n"
    "    else (literal, discard 00)\n"
    "      a=c a>>= 2 c=a             (bits>>=2)\n"
    "      d-- d--                    (n-=2)\n"
    "      a= 3 r=a 1                 (state=3)\n"
    "    endif\n"
    "  endif\n"
    "\n"
    "  (while state==1 && n>=3 (expect match length n*4+ll -> r2))\n"
    "  do a=r 1 a== 1 if a=d a> 2 if\n"
    "    a=c a&= 1 a== 1 if         (if bits&1)\n"
    "      a=c a>>= 1 c=a             (bits>>=1)\n"
    "      b=r 2 a=c a&= 1 a+=b a+=b r=a 2 (len+=len+(bits&1))\n"
    "      a=c a>>= 1 c=a             (bits>>=1)\n"
    "      d-- d--                    (n-=2)\n"
    "    else\n"
    "      a=c a>>= 1 c=a             (bits>>=1)\n"
    "      a=r 2 a<<= 2 b=a           (len<<=2)\n"
    "      a=c a&= 3 a+=b r=a 2       (len+=bits&3)\n"
    "      a=c a>>= 2 c=a             (bits>>=2)\n"
    "      d-- d-- d--                (n-=3)\n";
    if (rb)
      pcomp+="      a= 5 r=a 1                 (state=5)\n";
    else
      pcomp+="      a= 2 r=a 1                 (state=2)\n";
    pcomp+=
    "    endif\n"
    "  forever endif endif\n"
    "\n";
    if (rb) pcomp+=  // save r in r5
      "  (if state==5 && n>=8) (expect low bits of offset to put in r5)\n"
      "  a=r 1 a== 5 if a=d a> "+itos(rb-1)+" if\n"
      "    a=c a&= "+itos((1<<rb)-1)+" r=a 5            (save r in r5)\n"
      "    a=c a>>= "+itos(rb)+" c=a\n"
      "    a=d a-= "+itos(rb)+ " d=a\n"
      "    a= 2 r=a 1                   (go to state 2)\n"
      "  endif endif\n"
      "\n";
    pcomp+=
    "  (if state==2 && n>=m) (expect m offset bits)\n"
    "  a=r 1 a== 2 if a=r 3 a>d ifnot\n"
    "    a=c r=a 6 a=d r=a 7          (save c=bits, d=n in r6,r7)\n"
    "    b=r 3 a= 1 a<<=b d=a         (d=1<<m)\n"
    "    a-- a&=c a+=d                (d=offset=bits&((1<<m)-1)|(1<<m))\n";
    if (rb)
      pcomp+=  // insert r into low bits of d
      "    a<<= "+itos(rb)+" d=r 5 a+=d a-= "+itos((1<<rb)-1)+"\n";
    pcomp+=
    "    d=a b=r 4 a=b a-=d c=a       (c=p=(b=ptr)-offset)\n"
    "\n"
    "    (while len-- (copy and output match d bytes from *c to *b))\n"
    "    d=r 2 do a=d a> 0 if d--\n"
    "      a=*c *b=a c++ b++          (buf[ptr++]-buf[p++])\n";
    if (!doe8) pcomp+=" out\n";
    pcomp+=
    "    forever endif\n"
    "    a=b r=a 4\n"
    "\n"
    "    a=r 6 b=r 3 a>>=b c=a        (bits>>=m)\n"
    "    a=r 7 a-=b d=a               (n-=m)\n"
    "    a=0 r=a 1                    (state=0)\n"
    "  endif endif\n"
    "\n"
    "  (while state==3 && n>=2 (expect literal length))\n"
    "  do a=r 1 a== 3 if a=d a> 1 if\n"
    "    a=c a&= 1 a== 1 if         (if bits&1)\n"
    "      a=c a>>= 1 c=a              (bits>>=1)\n"
    "      b=r 2 a&= 1 a+=b a+=b r=a 2 (len+=len+(bits&1))\n"
    "      a=c a>>= 1 c=a              (bits>>=1)\n"
    "      d-- d--                     (n-=2)\n"
    "    else\n"
    "      a=c a>>= 1 c=a              (bits>>=1)\n"
    "      d--                         (--n)\n"
    "      a= 4 r=a 1                  (state=4)\n"
    "    endif\n"
    "  forever endif endif\n"
    "\n"
    "  (if state==4 && n>=8 (expect len literals))\n"
    "  a=r 1 a== 4 if a=d a> 7 if\n"
    "    b=r 4 a=c *b=a\n";
    if (!doe8) pcomp+=" out\n";
    pcomp+=
    "    b++ a=b r=a 4                 (buf[ptr++]=bits)\n"
    "    a=c a>>= 8 c=a                (bits>>=8)\n"
    "    a=d a-= 8 d=a                 (n-=8)\n"
    "    a=r 2 a-- r=a 2 a== 0 if      (if --len<1)\n"
    "      a=0 r=a 1                     (state=0)\n"
    "    endif\n"
    "  endif endif\n"
    "  halt\n"
    "end\n";
  }

  // Byte aligned LZ77, with or without E8E9
  else if (level==2) {
    hdr="comp 9 16 0 $1+20 ";
    pcomp=
    "pcomp lzpre c ;\n"
    "  (Decode LZ77: d=state, M=output buffer, b=size)\n"
    "  a> 255 if (at EOF decode e8e9 and output)\n";
    if (doe8)
      pcomp+=
      "    d=b b=0 do (for b=0..d-1, d = end of buf)\n"
      "      a=b a==d ifnot\n"
      "        a+= 4 a<d if\n"
      "          a=*b a&= 254 a== 232 if (e8 or e9?)\n"
      "            c=b b++ b++ b++ b++ a=*b a++ a&= 254 a== 0 if (00 or ff)\n"
      "              b-- a=*b\n"
      "              b-- a<<= 8 a+=*b\n"
      "              b-- a<<= 8 a+=*b\n"
      "              a-=b a++\n"
      "              *b=a a>>= 8 b++\n"
      "              *b=a a>>= 8 b++\n"
      "              *b=a b++\n"
      "            endif\n"
      "            b=c\n"
      "          endif\n"
      "        endif\n"
      "        a=*b out b++\n"
      "      forever\n"
      "    endif\n";
    pcomp+=
    "    b=0 c=0 d=0 a=0 r=a 1 r=a 2 (reset state)\n"
    "  halt\n"
    "  endif\n"
    "\n"
    "  (in state d==0, expect a new code)\n"
    "  (put length in r1 and inital part of offset in r2)\n"
    "  c=a a=d a== 0 if\n"
    "    a=c a>>= 6 a++ d=a\n"
    "    a== 1 if (literal?)\n"
    "      a+=c r=a 1 a=0 r=a 2\n"
    "    else (3 to 5 byte match)\n"
    "      d++ a=c a&= 63 a+= $3 r=a 1 a=0 r=a 2\n"
    "    endif\n"
    "  else\n"
    "    a== 1 if (writing literal)\n"
    "      a=c *b=a b++\n";
    if (!doe8) pcomp+=" out\n";
    pcomp+=
    "      a=r 1 a-- a== 0 if d=0 endif r=a 1 (if (--len==0) state=0)\n"
    "    else\n"
    "      a> 2 if (reading offset)\n"
    "        a=r 2 a<<= 8 a|=c r=a 2 d-- (off=off<<8|c, --state)\n"
    "      else (state==2, write match)\n"
    "        a=r 2 a<<= 8 a|=c c=a a=b a-=c a-- c=a (c=i-off-1)\n"
    "        d=r 1 (d=len)\n"
    "        do (copy and output d=len bytes)\n"
    "          a=*c *b=a c++ b++\n";
    if (!doe8) pcomp+=" out\n";
    pcomp+=
    "        d-- a=d a> 0 while\n"
    "        (d=state=0. off, len don\'t matter)\n"
    "      endif\n"
    "    endif\n"
    "  endif\n"
    "  halt\n"
    "end\n";
  }

  // BWT with or without E8E9
  else if (level==3) {  // IBWT
    hdr="comp 9 16 $1+20 $1+20 ";  // 2^$1 = block size in MB
    pcomp=
    "pcomp bwtrle c ;\n"
    "\n"
    "  (read BWT, index into M, size in b)\n"
    "  a> 255 ifnot\n"
    "    *b=a b++\n"
    "\n"
    "  (inverse BWT)\n"
    "  elsel\n"
    "\n"
    "    (index in last 4 bytes, put in c and R1)\n"
    "    b-- a=*b\n"
    "    b-- a<<= 8 a+=*b\n"
    "    b-- a<<= 8 a+=*b\n"
    "    b-- a<<= 8 a+=*b c=a r=a 1\n"
    "\n"
    "    (save size in R2)\n"
    "    a=b r=a 2\n"
    "\n"
    "    (count bytes in H[~1..~255, ~0])\n"
    "    do\n"
    "      a=b a> 0 if\n"
    "        b-- a=*b a++ a&= 255 d=a d! *d++\n"
    "      forever\n"
    "    endif\n"
    "\n"
    "    (cumulative counts: H[~i=0..255] = count of bytes before i)\n"
    "    d=0 d! *d= 1 a=0\n"
    "    do\n"
    "      a+=*d *d=a d--\n"
    "    d<>a a! a> 255 a! d<>a until\n"
    "\n"
    "    (build first part of linked list in H[0..idx-1])\n"
    "    b=0 do\n"
    "      a=c a>b if\n"
    "        d=*b d! *d++ d=*d d-- *d=b\n"
    "      b++ forever\n"
    "    endif\n"
    "\n"
    "    (rest of list in H[idx+1..n-1])\n"
    "    b=c b++ c=r 2 do\n"
    "      a=c a>b if\n"
    "        d=*b d! *d++ d=*d d-- *d=b\n"
    "      b++ forever\n"
    "    endif\n"
    "\n";
    if (args[0]<=4) {  // faster IBWT list traversal limited to 16 MB blocks
      pcomp+=
      "    (copy M to low 8 bits of H to reduce cache misses in next loop)\n"
      "    b=0 do\n"
      "      a=c a>b if\n"
      "        d=b a=*d a<<= 8 a+=*b *d=a\n"
      "      b++ forever\n"
      "    endif\n"
      "\n"
      "    (traverse list and output or copy to M)\n"
      "    d=r 1 b=0 do\n"
      "      a=d a== 0 ifnot\n"
      "        a=*d a>>= 8 d=a\n";
      if (doe8) pcomp+=" *b=*d b++\n";
      else      pcomp+=" a=*d out\n";
      pcomp+=
      "      forever\n"
      "    endif\n"
      "\n";
      if (doe8)  // IBWT+E8E9
        pcomp+=
        "    (e8e9 transform to out)\n"
        "    d=b b=0 do (for b=0..d-1, d = end of buf)\n"
        "      a=b a==d ifnot\n"
        "        a+= 4 a<d if\n"
        "          a=*b a&= 254 a== 232 if\n"
        "            c=b b++ b++ b++ b++ a=*b a++ a&= 254 a== 0 if\n"
        "              b-- a=*b\n"
        "              b-- a<<= 8 a+=*b\n"
        "              b-- a<<= 8 a+=*b\n"
        "              a-=b a++\n"
        "              *b=a a>>= 8 b++\n"
        "              *b=a a>>= 8 b++\n"
        "              *b=a b++\n"
        "            endif\n"
        "            b=c\n"
        "          endif\n"
        "        endif\n"
        "        a=*b out b++\n"
        "      forever\n"
        "    endif\n";
      pcomp+=
      "  endif\n"
      "  halt\n"
      "end\n";
    }
    else {  // slower IBWT list traversal for all sized blocks
      if (doe8) {  // E8E9 after IBWT
        pcomp+=
        "    (R2 = output size without EOS)\n"
        "    a=r 2 a-- r=a 2\n"
        "\n"
        "    (traverse list (d = IBWT pointer) and output inverse e8e9)\n"
        "    (C = offset = 0..R2-1)\n"
        "    (R4 = last 4 bytes shifted in from MSB end)\n"
        "    (R5 = temp pending output byte)\n"
        "    c=0 d=r 1 do\n"
        "      a=d a== 0 ifnot\n"
        "        d=*d\n"
        "\n"
        "        (store byte in R4 and shift out to R5)\n"
        "        b=d a=*b a<<= 24 b=a\n"
        "        a=r 4 r=a 5 a>>= 8 a|=b r=a 4\n"
        "\n"
        "        (if E8|E9 xx xx xx 00|FF in R4:R5 then subtract c from x)\n"
        "        a=c a> 3 if\n"
        "          a=r 5 a&= 254 a== 232 if\n"
        "            a=r 4 a>>= 24 b=a a++ a&= 254 a< 2 if\n"
        "              a=r 4 a-=c a+= 4 a<<= 8 a>>= 8 b<>a a<<= 24 a+=b r=a 4\n"
        "            endif\n"
        "          endif\n"
        "        endif\n"
        "\n"
        "        (output buffered byte)\n"
        "        a=c a> 3 if a=r 5 out endif c++\n"
        "\n"
        "      forever\n"
        "    endif\n"
        "\n"
        "    (output up to 4 pending bytes in R4)\n"
        "    b=r 4\n"
        "    a=c a> 3 a=b if out endif a>>= 8 b=a\n"
        "    a=c a> 2 a=b if out endif a>>= 8 b=a\n"
        "    a=c a> 1 a=b if out endif a>>= 8 b=a\n"
        "    a=c a> 0 a=b if out endif\n"
        "\n"
        "  endif\n"
        "  halt\n"
        "end\n";
      }
      else {
        pcomp+=
        "    (traverse list and output)\n"
        "    d=r 1 do\n"
        "      a=d a== 0 ifnot\n"
        "        d=*d\n"
        "        b=d a=*b out\n"
        "      forever\n"
        "    endif\n"
        "  endif\n"
        "  halt\n"
        "end\n";
      }
    }
  }

  // E8E9 or no preprocessing
  else if (level==0) {
    hdr="comp 9 16 0 0 ";
    if (doe8) { // E8E9?
      pcomp=
      "pcomp e8e9 d ;\n"
      "  a> 255 if\n"
      "    a=c a> 4 if\n"
      "      c= 4\n"
      "    else\n"
      "      a! a+= 5 a<<= 3 d=a a=b a>>=d b=a\n"
      "    endif\n"
      "    do a=c a> 0 if\n"
      "      a=b out a>>= 8 b=a c--\n"
      "    forever endif\n"
      "  else\n"
      "    *b=b a<<= 24 d=a a=b a>>= 8 a+=d b=a c++\n"
      "    a=c a> 4 if\n"
      "      a=*b out\n"
      "      a&= 254 a== 232 if\n"
      "        a=b a>>= 24 a++ a&= 254 a== 0 if\n"
      "          a=b a>>= 24 a<<= 24 d=a\n"
      "          a=b a-=c a+= 5\n"
      "          a<<= 8 a>>= 8 a|=d b=a\n"
      "        endif\n"
      "      endif\n"
      "    endif\n"
      "  endif\n"
      "  halt\n"
      "end\n";
    }
    else
      pcomp="end\n";
  }
  else
    error("Unsupported method");
  
  // Build context model (comp, hcomp) assuming:
  // H[0..254] = contexts
  // H[255..511] = location of last byte i-255
  // M = last 64K bytes, filling backward
  // C = pointer to most recent byte
  // R1 = level 2 lz77 1+bytes expected until next code, 0=init
  // R2 = level 2 lz77 first byte of code
  int ncomp=0;  // number of components
  const int membits=args[0]+20;
  int sb=5;  // bits in last context
  string comp;
  string hcomp="hcomp\n"
    "c-- *c=a a+= 255 d=a *d=c\n";
  if (level==2) {  // put level 2 lz77 parse state in R1, R2
    hcomp+=
    "  (decode lz77 into M. Codes:\n"
    "  00xxxxxx = literal length xxxxxx+1\n"
    "  xx......, xx > 0 = match with xx offset bytes to follow)\n"
    "\n"
    "  a=r 1 a== 0 if (init)\n"
    "    a= "+itos(111+57*doe8)+" (skip post code)\n"
    "  else a== 1 if  (new code?)\n"
    "    a=*c r=a 2  (save code in R2)\n"
    "    a> 63 if a>>= 6 a++ a++  (match)\n"
    "    else a++ a++ endif  (literal)\n"
    "  else (read rest of code)\n"
    "    a--\n"
    "  endif endif\n"
    "  r=a 1  (R1 = 1+expected bytes to next code)\n";
  }

  // Generate the context model
  while (*method && ncomp<254) {

    // parse command C[N1[,N2]...] into v = {C, N1, N2...}
    vector<int> v;
    v.push_back(*method++);
    if (isdigit(*method)) {
      v.push_back(*method++-'0');
      while (isdigit(*method) || *method==',' || *method=='.') {
        if (isdigit(*method))
          v.back()=v.back()*10+*method++-'0';
        else {
          v.push_back(0);
          ++method;
        }
      }
    }

    // c: context model
    // N1: 0=ICM 1..256=CM limit N1-1
    // N2: 1..255=offset mod N2. 1000..1255=distance to N2-1000
    // N3...: 0..255=byte mask + 256=lz77 state. 1000+=run of N3-1000 zeros.
    if (v[0]=='c') {
      while (v.size()<3) v.push_back(0);
      comp+=itos(ncomp)+" ";
      sb=11;  // count context bits
      if (v[2]<256) sb+=nbits(v[2]);
      else sb+=6;
      if (args[1]==2) sb+=8;
      for (unsigned i=3; i<v.size(); ++i)
        if (v[i]<512) sb+=nbits(v[i])*3/4;
      if (sb>membits) sb=membits;
      if (v[1]==0) comp+="icm "+itos(sb-6)+"\n";
      else comp+="cm "+itos(sb-2)+" "+itos(v[1]-1)+"\n";

      // special contexts
      hcomp+="d= " +itos(ncomp)+" *d=0\n";
      if (v[2]>1 && v[2]<=255) {  // periodic context
        if (lg(v[2])!=lg(v[2]-1))
          hcomp+="a=c a&= "+itos(v[2]-1)+" hashd\n";
        else
          hcomp+="a=c a%= "+itos(v[2])+" hashd\n";
      }
      else if (v[2]>=1000 && v[2]<=1255)  // distance context
        hcomp+="a= 255 a+= "+itos(v[2]-1000)+
               " d=a a=*d a-=c a> 255 if a= 255 endif d= "+
               itos(ncomp)+" hashd\n";

      // Masked context
      for (unsigned i=3; i<v.size(); ++i) {
        if (i==3) hcomp+="b=c ";
        if (v[i]==255)
          hcomp+="a=*b hashd\n";  // ordinary byte
        else if (v[i]>0 && v[i]<255)
          hcomp+="a=*b a&= "+itos(v[i])+" hashd\n";  // masked byte
        else if (v[i]>=256 && v[i]<512) { // lz77 state or masked literal byte
          hcomp+=
          "a=r 1 a> 1 if\n"  // expect literal or offset
          "  a=r 2 a< 64 if\n"  // expect literal
          "    a=*b ";
          if (v[i]<511) hcomp+="a&= "+itos(v[i]-256);
          hcomp+=" hashd\n"
          "  else\n"  // expect match offset byte
          "    a>>= 6 hashd a=r 1 hashd\n"
          "  endif\n"
          "else\n"  // expect new code
          "  a= 255 hashd a=r 2 hashd\n"
          "endif\n";
        }
        else if (v[i]>=1256)  // skip v[i]-1000 bytes
          hcomp+="a= "+itos(((v[i]-1000)>>8)&255)+" a<<= 8 a+= "
               +itos((v[i]-1000)&255)+
          " a+=b b=a\n";
        else if (v[i]>1000)
          hcomp+="a= "+itos(v[i]-1000)+" a+=b b=a\n";
        if (v[i]<512 && i<v.size()-1)
          hcomp+="b++ ";
      }
      ++ncomp;
    }

    // m,8,24: MIX, size, rate
    // t,8,24: MIX2, size, rate
    // s,8,32,255: SSE, size, start, limit
    if (strchr("mts", v[0]) && ncomp>int(v[0]=='t')) {
      if (v.size()<=1) v.push_back(8);
      if (v.size()<=2) v.push_back(24+8*(v[0]=='s'));
      if (v[0]=='s' && v.size()<=3) v.push_back(255);
      comp+=itos(ncomp);
      sb=5+v[1]*3/4;
      if (v[0]=='m')
        comp+=" mix "+itos(v[1])+" 0 "+itos(ncomp)+" "+itos(v[2])+" 255\n";
      else if (v[0]=='t')
        comp+=" mix2 "+itos(v[1])+" "+itos(ncomp-1)+" "+itos(ncomp-2)
            +" "+itos(v[2])+" 255\n";
      else // s
        comp+=" sse "+itos(v[1])+" "+itos(ncomp-1)+" "+itos(v[2])+" "
            +itos(v[3])+"\n";
      if (v[1]>8) {
        hcomp+="d= "+itos(ncomp)+" *d=0 b=c a=0\n";
        for (; v[1]>=16; v[1]-=8) {
          hcomp+="a<<= 8 a+=*b";
          if (v[1]>16) hcomp+=" b++";
          hcomp+="\n";
        }
        if (v[1]>8)
          hcomp+="a<<= 8 a+=*b a>>= "+itos(16-v[1])+"\n";
        hcomp+="a<<= 8 *d=a\n";
      }
      ++ncomp;
    }

    // i: ISSE chain with order increasing by N1,N2...
    if (v[0]=='i' && ncomp>0) {
      assert(sb>=5);
      hcomp+="d= "+itos(ncomp-1)+" b=c a=*d d++\n";
      for (unsigned i=1; i<v.size() && ncomp<254; ++i) {
        for (int j=0; j<v[i]; ++j) {
          hcomp+="hash ";
          if (i<v.size()-1 || j<v[i]-1) hcomp+="b++ ";
          sb+=6;
        }
        hcomp+="*d=a";
        if (i<v.size()-1) hcomp+=" d++";
        hcomp+="\n";
        if (sb>membits) sb=membits;
        comp+=itos(ncomp)+" isse "+itos(sb-6)+" "+itos(ncomp-1)+"\n";
        ++ncomp;
      }
    }

    // a24,0,0: MATCH. N1=hash multiplier. N2,N3=halve buf, table.
    if (v[0]=='a') {
      if (v.size()<=1) v.push_back(24);
      while (v.size()<4) v.push_back(0);
      comp+=itos(ncomp)+" match "+itos(membits-v[3]-2)+" "
          +itos(membits-v[2])+"\n";
      hcomp+="d= "+itos(ncomp)+" a=*d a*= "+itos(v[1])
           +" a+=*c a++ *d=a\n";
      sb=5+(membits-v[2])*3/4;
      ++ncomp;
    }

    // w1,65,26,223,20: ICM-ISSE chain of length N1 with word contexts,
    // where a word is a sequence of c such that c&N4 is in N2..N2+N3-1.
    // Word is hashed by: hash := hash*N5+c+1
    if (v[0]=='w') {
      if (v.size()<=1) v.push_back(1);
      if (v.size()<=2) v.push_back(65);
      if (v.size()<=3) v.push_back(26);
      if (v.size()<=4) v.push_back(223);
      if (v.size()<=5) v.push_back(20);
      comp+=itos(ncomp)+" icm "+itos(membits-6)+"\n";
      for (int i=1; i<v[1]; ++i)
        comp+=itos(ncomp+i)+" isse "+itos(membits-6)+" "
            +itos(ncomp+i-1)+"\n";
      hcomp+="a=*c a&= "+itos(v[4])+" a-= "+itos(v[2])+" a&= 255 a< "
           +itos(v[3])+" if\n";
      for (int i=0; i<v[1]; ++i) {
        if (i==0) hcomp+="  d= "+itos(ncomp);
        else hcomp+="  d++";
        hcomp+=" a=*d a*= "+itos(v[5])+" a+=*c a++ *d=a\n";
      }
      hcomp+="else\n";
      for (int i=v[1]-1; i>0; --i)
        hcomp+="  d= "+itos(ncomp+i-1)+" a=*d d++ *d=a\n";
      hcomp+="  d= "+itos(ncomp)+" *d=0\n"
           "endif\n";
      ncomp+=v[1]-1;
      sb=membits;
      ++ncomp;
    }

    // Read from config file and ignore rest of command
    if (v[0]=='f') {
      string filename=method;  // append .cfg if not already
      int len=filename.size();
      if (len<=4 || filename.substr(len-4)!=".cfg") filename+=".cfg";
      FILE* in=fopen(filename.c_str(), "r");
      if (!in) {
        perror(filename.c_str());
        error("Config file not found");
      }
      string cfg;
      int c;
      while ((c=getc(in))!=EOF) cfg+=(char)c;
      fclose(in);
      return cfg;
    }
  }
  return hdr+itos(ncomp)+"\n"+comp+hcomp+"halt\n"+pcomp;
}

// Compress from in to out in 1 segment in 1 block using the algorithm
// descried in method. If method begins with a digit then choose
// a method depending on type. Save filename and comment
// in the segment header. If comment is 0 then the default is the input size
// as a decimal string, plus " jDC\x01" for a journaling method (method[0]
// is not 's'). type is set as follows: bits 9-2 estimate compressibility
// where 0 means random. Bit 1 indicates x86 (exe or dll) and bit 0 
// indicates English text.
string compressBlock(StringBuffer* in, libzpaq::Writer* out, string method,
                     const char* filename=0, const char* comment=0,
                     unsigned type=512) {
  assert(in);
  assert(out);
  assert(method!="");
  const unsigned n=in->size();  // input size
  const int arg0=method.size()>1
      ? atoi(method.c_str()+1) : std::max(lg(n+4095)-20, 0);  // block size
  assert((1u<<(arg0+20))>=n+4096);

  // Expand default methods
  if (isdigit(method[0])) {
    const int level=method[0]-'0';
    assert(level>=0 && level<=9);

    // build models
    const int doe8=(type&2)*2;
    method="x"+itos(arg0);
    string htsz=","+itos(19+arg0+(arg0<=6));  // lz77 hash table size param

    // store uncompressed
    if (level==0)
      method+=",0";

    // LZ77, no model. Store if hard to compress
    else if (level==1) {
      if (type<40) method+=",0";
      else {
        method+=","+itos(1+doe8)+",";
        if      (type<80)  method+="4,0,1,15";
        else if (type<128) method+="4,0,2,16";
        else if (type<256) method+="4,0,2"+htsz;
        else               method+="5,0,3"+htsz;
      }
    }

    // LZ77 with longer search
    else if (level==2) {
      if (type<32) method+=",0";
      else {
        method+=","+itos(1+doe8)+",4";
        if      (type<64)  method+=",0,1,16";
        else if (type<96)  method+=",0,2"+htsz;
        else if (type<128) method+=",0,3"+htsz;
        else if (type<256) method+=",8,3"+htsz;
        else               method+=",8,4"+htsz;
      }
    }

    // LZ77 with CM depending on redundancy
    else if (level==3) {
      if (type<16)
        method+=",0";
      else if (type<48)
        method+=","+itos(1+doe8)+",4,0,3"+htsz;
      else
        method+=","+itos(2+doe8)+",8,0,4"+htsz+",c0,0,511";
    }

    // Try LZ77+CM and BWT and pick the smallest. At level 5 also try CM.
    else if (level==4 || level==5) {
      string s=","+itos(20+arg0);
      if (type<12)
        method+=",0";
      else if (type<24)
        method+=","+itos(1+doe8)+",4,0,3"+htsz+",16,18";
      else if (type<48)
        method+=","+itos(2+doe8)+",8,0,4"+htsz+",16,24c0,0,511";
      else {  // try LZ77, BWT, and CM (level 5) and pick the smallest
        StringBuffer in2, out1, out2;
        string method1=method+","+itos(2+doe8)+",8,0,4"+htsz+",16,24c0,0,511";
        string method2=method+","+itos(3+doe8)+"ci1";
        string method3=method+","+itos(doe8)+"ci1,1,1,1,2a"; // level 5
        if (type&1) method3+="w";
        method3+="m";
        string result=method1;
        in2.write(in->c_str(), in->size());
        compressBlock(&in2, &out1, method1, filename, comment, type);
        in2.write(in->c_str(), in->size());
        compressBlock(&in2, &out2, method2, filename, comment, type);
        if (out2.size()<out1.size()) {
          out1.swap(out2);
          result=method2;
        }
        if (level==5) {
          out2.reset();
          compressBlock(in, &out2, method3, filename, comment, type);
          if (out2.size()<out1.size()) {
            out1.swap(out2);
            result=method3;
          }
        }
        out->write(out1.c_str(), out1.size());
        return result;
      }
    }

    // Slow CM with lots of models
    else if (level==6) {

      // Model text files
      method+=","+itos(doe8);
      if (type&1) method+="w2c0,1010,255i1";
      else method+="w1i1";
      method+="c256ci1,1,1,1,1,1,2a";

      // Analyze the data
      const int NR=1<<12;
      int pt[256]={0};  // position of last occurrence
      int r[NR]={0};    // count repetition gaps of length r
      const unsigned char* p=in->data();
      if (level>0) {
        for (unsigned i=0; i<n; ++i) {
          const int k=i-pt[p[i]];
          if (k>0 && k<NR) ++r[k];
          pt[p[i]]=i;
        }
      }

      // Add periodic models
      int n1=n-r[1]-r[2]-r[3];
      for (int i=0; i<2; ++i) {
        int period=0;
        double score=0;
        int t=0;
        for (int j=5; j<NR && t<n1; ++j) {
          const double s=r[j]/(256.0+n1-t);
          if (s>score) score=s, period=j;
          t+=r[j];
        }
        if (period>4 && score>0.1) {
          method+="c0,0,"+itos(999+period)+",255i1";
          if (period<=255)
            method+="c0,"+itos(period)+"i1";
          n1-=r[period];
          r[period]=0;
        }
        else
          break;
      }
      method+="c0,2,0,255i1c0,3,0,0,255i1c0,4,0,0,0,255i1mm16ts19t0";
    }
    else 
      error("method must be 0..6, x, or s");
  }

  // Get hash of input
  string config;
  int args[9]={0};
  try {
    libzpaq::SHA1 sha1;
    const char* sha1ptr=0;
    if (!fragile) {
      for (const char* p=in->c_str(), *end=p+n; p<end; ++p)
        sha1.put(*p);
      sha1ptr=sha1.result();
    }

    // Get config
    config=makeConfig(method.c_str(), args);
    assert(n<=(0x100000u<<args[0])-4096);

    // Compress in to out using config
    libzpaq::Compressor co;
    co.setOutput(out);
#ifdef DEBUG
    if (!fragile) co.setVerify(true);
#endif
    StringBuffer pcomp_cmd;
    if (!fragile) co.writeTag();
    co.startBlock(config.c_str(), args, &pcomp_cmd);
    string cs=itos(n);
    if (method[0]!='s') cs+=" jDC\x01";
    if (comment) cs=comment;
    co.startSegment(filename, cs.c_str());
    if (args[1]==1 || args[1]==2 || args[1]==5 || args[1]==6) {  // LZ77
      LZBuffer lz(*in, args);
      co.setInput(&lz);
      co.compress();
    }
    else if (args[1]==3 || args[1]==7) {  // BWT
      BWTBuffer bwt(*in, args[1]==7);
      co.setInput(&bwt);
      co.compress();
    }
    else {  // compress with e8e9 or no preprocessing
      if (args[1]>=4 && args[1]<=7)
        e8e9(in->data(), in->size());
      co.setInput(in);
      co.compress();
    }
    in->reset();
#ifdef DEBUG  // verify pre-post processing are inverses
    if (fragile)
      co.endSegment(0);
    else {
      int64_t outsize;
      const char* sha1result=co.endSegmentChecksum(&outsize);
      assert(sha1result);
      if (memcmp(sha1result, sha1ptr, 20)!=0) {
        fprintf(stderr, "pre size=%d post size=%1.0f method=%s\n",
                n, double(outsize), method.c_str());
        error("Pre/post-processor test failed");
      }
    }
#else
    co.endSegment(sha1ptr);
#endif
    co.endBlock();
  }
  catch(std::exception& e) {
    printf("Compression error %s\n", e.what());
    printf("\nconfig:\n%s\n", config.c_str());
    printf("\nmethod=%s\n", method.c_str());
    for (int i=0; i<9; ++i)
      printf("args[%d] = $%d = %d\n", i, i+1, args[i]);
    error("compression error");
  }
  return method;
}

// A CompressJob is a queue of blocks to compress and write to the archive.
// Each block cycles through states EMPTY, FILLING, FULL, COMPRESSING,
// COMPRESSED, WRITING. The main thread waits for EMPTY buffers and
// fills them. A set of compressThreads waits for FULL threads and compresses
// them. A writeThread waits for COMPRESSED buffers at the front
// of the queue and writes and removes them.

// Buffer queue element
struct CJ {
  enum {EMPTY, FULL, COMPRESSING, COMPRESSED, WRITING} state;
  StringBuffer in, out;  // uncompressed and compressed data
  string filename;       // to write in filename field
  string method;         // compression level or "" to mark end of data
  int type;              // redundancy*4 + exe*2 + text
  Semaphore full;        // 1 if in is FULL of data ready to compress
  Semaphore compressed;  // 1 if out contains COMPRESSED data
  CJ(): state(EMPTY), type(512) {}
};

// Instructions to a compression job
class CompressJob {
  Mutex mutex;           // protects state changes
  int job;               // number of jobs
  CJ* q;                 // buffer queue
  unsigned qsize;        // number of elements in q
  int front;             // next to remove from queue
  libzpaq::Writer* out;  // archive
  Semaphore empty;       // number of empty buffers ready to fill
public:
  friend ThreadReturn compressThread(void* arg);
  friend ThreadReturn writeThread(void* arg);
  CompressJob(int t, libzpaq::Writer* f):
      job(0), q(0), qsize(t), front(0), out(f) {
    q=new CJ[t];
    if (!q) throw std::bad_alloc();
    init_mutex(mutex);
    empty.init(t);
    for (int i=0; i<t; ++i) {
      q[i].full.init(0);
      q[i].compressed.init(0);
    }
  }
  ~CompressJob() {
    for (int i=qsize-1; i>=0; --i) {
      q[i].compressed.destroy();
      q[i].full.destroy();
    }
    empty.destroy();
    destroy_mutex(mutex);
    delete[] q;
  }      
  void write(StringBuffer& s, const char* filename, string method,
             int hits=-1);
  vector<int> csize;  // compressed block sizes
};

// Write s at the back of the queue. Signal end of input with method=-1
void CompressJob::write(StringBuffer& s, const char* fn, string method,
                        int type) {
  for (unsigned k=(method=="")?qsize:1; k>0; --k) {
    empty.wait();
    lock(mutex);
    unsigned i, j;
    for (i=0; i<qsize; ++i) {
      if (q[j=(i+front)%qsize].state==CJ::EMPTY) {
        q[j].filename=fn?fn:"";
        q[j].method=method;
        q[j].type=type;
        q[j].in.reset();
        q[j].in.swap(s);
        q[j].state=CJ::FULL;
        q[j].full.signal();
        break;
      }
    }
    release(mutex);
    assert(i<qsize);  // queue should not be full
  }
}  

// Compress data in the background, one per buffer
ThreadReturn compressThread(void* arg) {
  CompressJob& job=*(CompressJob*)arg;
  int jobNumber=0;
  try {

    // Get job number = assigned position in queue
    lock(job.mutex);
    jobNumber=job.job++;
    assert(jobNumber>=0 && jobNumber<int(job.qsize));
    CJ& cj=job.q[jobNumber];
    release(job.mutex);

    // Work until done
    while (true) {
      cj.full.wait();
      lock(job.mutex);

      // Check for end of input
      if (cj.method=="") {
        cj.compressed.signal();
        release(job.mutex);
        return 0;
      }

      // Compress
      assert(cj.state==CJ::FULL);
      cj.state=CJ::COMPRESSING;
      int insize=cj.in.size(), start=0, frags=0;
      int64_t now=mtime();
      if (insize>=8) {
        const char* p=cj.in.c_str()+insize-8;
        start=btoi(p);
        frags=btoi(p);
      }
      release(job.mutex);
      string m=compressBlock(&cj.in, &cj.out, cj.method, cj.filename.c_str(),
                             0, cj.type);
      lock(job.mutex);
      if (quiet<=insize)
        printf("Job %d: [%d-%d] %d -> %d (%1.3f s), %d%c -m %s\n",
               jobNumber+1, start, start+frags-1, insize,
               int(cj.out.size()), (mtime()-now)*0.001,
               cj.type/4, " teb"[cj.type&3], m.c_str());
      cj.in.reset();
      cj.state=CJ::COMPRESSED;
      cj.compressed.signal();
      release(job.mutex);
    }
  }
  catch (std::exception& e) {
    fprintf(stderr, "zpaq exiting from job %d: %s\n", jobNumber+1, e.what());
    exit(1);
  }
  return 0;
}

// Write compressed data to the archive in the background
ThreadReturn writeThread(void* arg) {
  CompressJob& job=*(CompressJob*)arg;
  try {

    // work until done
    while (true) {

      // wait for something to write
      CJ& cj=job.q[job.front];  // no other threads move front
      cj.compressed.wait();

      // Quit if end of input
      lock(job.mutex);
      if (cj.method=="") {
        release(job.mutex);
        return 0;
      }

      // Write to archive
      assert(cj.state==CJ::COMPRESSED);
      cj.state=CJ::WRITING;
      job.csize.push_back(cj.out.size());
      int outsize=cj.out.size();
      if (outsize>0) {
        release(job.mutex);
        job.out->write(cj.out.c_str(), outsize);
        lock(job.mutex);
      }
      cj.state=CJ::EMPTY;
      cj.out.reset();
      job.front=(job.front+1)%job.qsize;
      job.empty.signal();
      release(job.mutex);
    }
  }
  catch (std::exception& e) {
    fprintf(stderr, "zpaq exiting from writeThread: %s\n", e.what());
    exit(1);
  }
  return 0;
}

// Write a ZPAQ compressed JIDAC block header. Output size should not
// depend on input data.
void writeJidacHeader(libzpaq::Writer *out, int64_t date,
                      int64_t cdata, unsigned htsize) {
  assert(out);
  assert(date>=19700000000000LL && date<30000000000000LL);
  libzpaq::Compressor co;
  StringBuffer is;
  is+=ltob(cdata);
  compressBlock(&is, out, "0",
                ("jDC"+itos(date, 14)+"c"+itos(htsize, 10)).c_str());
}

// Maps sha1 -> fragment ID in ht with known size
class HTIndex {
  enum {N=1<<22};   // size of hash table t
  vector<HT>& htr;  // reference to ht
  vector<vector<unsigned> > t;  // sha1 prefix -> list of indexes
  unsigned htsize;  // number of IDs in t

  // Compuate a hash index for sha1[20]
  unsigned hash(const unsigned char* sha1) {
    return (sha1[0]|(sha1[1]<<8)|(sha1[2]<<16))&(N-1);
  }

public:
  HTIndex(vector<HT>& r): htr(r), t(N), htsize(0) {
    update();
  }

  // Find sha1 in ht. Return its index or 0 if not found.
  unsigned find(const char* sha1) {
    vector<unsigned>& v=t[hash((const unsigned char*)sha1)];
    for (unsigned i=0; i<v.size(); ++i)
      if (memcmp(sha1, htr[v[i]].sha1, 20)==0)
        return v[i];
    return 0;
  }

  // Update index of ht. Do not index if fragment size is unknown.
  void update() {
    for (; htsize<htr.size(); ++htsize)
      if (htr[htsize].usize>=0)
        t[hash(htr[htsize].sha1)].push_back(htsize);
  }    
};

// Sort by file name extension
bool compareFilename(DTMap::iterator ap, DTMap::iterator bp) {
  const char* as=ap->first.c_str();
  const char* bs=bp->first.c_str();
  const char* ae=strrchr(as, '.');
  if (!ae) ae="";
  const char* be=strrchr(bs, '.');
  if (!be) be="";
  int cmp=strcmp(ae, be);
  if (cmp) return cmp<0;
  return strcmp(as, bs)<0;
}

// Add or delete files from archive
void Jidac::add() {

  // Set block size
  assert(method!="");
  unsigned blocksize=(1<<24)-4096;
  if (isdigit(method[0]) && method[0]>'1')
    blocksize=(1<<26)-4096;
  if (method.size()>1)
    blocksize=(1u<<(20+atoi(method.c_str()+1)))-4096;

  // Read archive index list into ht, dt, ver.
  const int64_t header_pos=
      archive!="" && exists(archive) ? read_archive() : 0;
  if (header_pos==0 && quiet<MAX_QUIET) {
    printf("Creating new archive ");
    printUTF8(archive.c_str());
    printf("\n");
  }

  // Make list of files to add or delete
  read_args(command=="-add");

  // Sort the files to be added by filename extension
  vector<DTMap::iterator> vf;
  unsigned deletions=0;
  for (DTMap::iterator p=dt.begin(); p!=dt.end(); ++p) {
    if (p->second.edate && (force || p->second.dtv.size()==0
       || p->second.edate!=p->second.dtv.back().date))
    vf.push_back(p);
    if (p->second.written==0 && p->second.edate==0)
      ++deletions;
  }
  std::sort(vf.begin(), vf.end(), compareFilename);

  // Test if any files are to be added or deleted
  if (vf.size()==0 && deletions==0) {
    if (quiet<MAX_QUIET)
      printf("Archive %s not updated: nothing to add or delete\n",
          archive.c_str());
    return;
  }  

  // Get transaction date for a journaling update
  time_t now=time(NULL);
  tm* t=gmtime(&now);
  date=(t->tm_year+1900)*10000000000LL+(t->tm_mon+1)*100000000LL
      +t->tm_mday*1000000+t->tm_hour*10000+t->tm_min*100+t->tm_sec;
  if (now==-1 || date<20120000000000LL || date>30000000000000LL)
    error("date is incorrect");

  // Open archive to append
  if (quiet<MAX_QUIET) {
    printf("Updating ");
    printUTF8(archive.c_str());
    printf(" with %u additions and %u deletions at %s\n",
        size(vf), deletions, dateToString(date).c_str());
  }
  OutputFile out;
  Counter counter;
  libzpaq::Writer* outp=0;  // pointer to output
  if (archive=="")
    outp=&counter;
  else {
    if (!out.open(archive.c_str())) error("Archive open failed");
    int64_t archive_size=out.tell();
    if (archive_size!=header_pos) {
      if (quiet<MAX_QUIET)
        printf("Archive truncated from %1.0f to %1.0f bytes\n",
               double(archive_size), double(header_pos));
      out.truncate(header_pos);
    }
    outp=&out;
  }
  int64_t inputsize=0;  // total input size

  // Append in streaming mode. Each file is a separate block. Large files
  // are split into blocks of size blocksize.
  if (method[0]=='s' && command=="-add") {
    StringBuffer sb(blocksize+4096-128);
    int64_t offset=archive=="" ? 0 : out.tell();
    for (unsigned fi=0; fi<vf.size(); ++fi) {
      DTMap::iterator p=vf[fi];
      if (p->first.size()>0 && p->first[p->first.size()-1]!='/') {
        int64_t start=mtime();
        InputFile in;
        if (in.open(p->first.c_str())) {
          int64_t i=0;
          while (true) {
            int c=in.get();
            if (c!=EOF) ++i, sb.put(c);
            if (c==EOF || sb.size()==blocksize) {
              string filename="";
              string comment=itos(sb.size());
              if (i<=blocksize) {
                filename=p->first;
                comment+=" "+itos(p->second.edate);
                if ((p->second.eattr&255)>0) {
                  comment+=" ";
                  comment+=char(p->second.eattr&255);
                  comment+=itos(p->second.eattr>>8);
                }
              }
              compressBlock(&sb, outp, method, filename.c_str(),
                            comment.c_str());
              assert(sb.size()==0);
            }
            if (c==EOF) break;
          }
          in.close();
          inputsize+=i;
          int64_t newoffset=archive=="" ? counter.pos : out.tell();
          if (quiet<=i) {
            printUTF8(p->first.c_str());
            printf(" %1.0f -> %1.0f in %1.3f sec.\n", double(i),
                   double(newoffset-offset), 0.001*(mtime()-start));
          }
          offset=newoffset;
        }
      }
    }
    if (quiet<MAX_QUIET) {
      const int64_t outsize=archive=="" ? counter.pos : out.tell();
      printf("%1.0f + (%1.0f -> %1.0f) = %1.0f\n",
          double(header_pos),
          double(inputsize),
          double(outsize-header_pos),
          double(outsize));
    }
    if (archive!="") out.close();
    return;
  }

  // Adjust date to maintain sequential order
  if (ver.size() && ver.back().date>=date) {
    const int64_t newdate=decimal_time(unix_time(ver.back().date)+1);
    fprintf(stderr, "Warning: adjusting date from %s to %s\n",
      dateToString(date).c_str(), dateToString(newdate).c_str());
    assert(newdate>date);
    date=newdate;
  }

  // Build htinv for fast lookups of sha1 in ht
  HTIndex htinv(ht);

  // reserve space for the header block
  const unsigned htsize=ht.size();  // fragments at start of update
  writeJidacHeader(outp, date, -1, htsize);
  const int64_t header_end=archive=="" ? counter.pos : out.tell();

  // Start compress and write jobs
  vector<ThreadID> tid(threads);
  ThreadID wid;
  CompressJob job(threads, outp);
  if (quiet<MAX_QUIET) printf("Starting %d compression jobs\n", threads);
  for (int i=0; i<threads; ++i) run(tid[i], compressThread, &job);
  run(wid, writeThread, &job);

  // Compress until end of last file
  assert(method!="");
  const unsigned MIN_FRAGMENT=4096;   // fragment size limits
  const unsigned MAX_FRAGMENT=520192;
  unsigned fi=0;       // file number in vf
  unsigned fj=0;       // fragment number in file
  InputFile in;        // currently open input file
  StringBuffer sb;     // block to compress
  unsigned frags=0;    // number of fragments in sb
  unsigned redundancy=0;  // estimated bytes that can be compressed out of sb
  unsigned text=0;     // number of fragents containing text
  unsigned exe=0;      // number of fragments containing x86 (exe, dll)
  const int ON=4;      // number of order-1 tables to save
  unsigned char o1prev[ON*256]={0};  // last ON order 1 predictions
  while (fi<vf.size() || frags>0) {

    // Compress a block if (1) end of input, (2) block is full,
    // (3) EOF, block is almost full, and next file won't fit, or
    // (4) EOF, block is partly full and not compressible.
    // In that case, store it uncompressed.
    if (fi==vf.size()
        || sb.size()>blocksize-MAX_FRAGMENT-80-frags*4
        || (fj==0 && sb.size()>blocksize*3/4
            && sb.size()+vf[fi]->second.esize>blocksize-MAX_FRAGMENT-2048) 
        || (fj==0 && sb.size()>blocksize/8 && redundancy<sb.size()/32)
        || (fj==0 && sb.size()>blocksize/4 && redundancy<sb.size()/16)
        || (fj==0 && sb.size()>blocksize/2 && redundancy<sb.size()/8)) {

      // Pad sb with redundant fragment size list and compress
      if (frags>0) {
        assert(frags<ht.size());
        if (fragile) {
          sb+=itob(0);  // omit first frag ID
          sb+=itob(0);  // omit number of fragments
        }
        else {
          for (unsigned i=ht.size()-frags; i<ht.size(); ++i)
            sb+=itob(ht[i].usize);  // list of frag sizes
          sb+=itob(ht.size()-frags);  // first frag ID
          sb+=itob(frags);  // number of frags
        }
        job.write(sb,
            ("jDC"+itos(date, 14)+"d"+itos(ht.size()-frags, 10)).c_str(),
            method,
            redundancy/(sb.size()/256+1)*4+(exe>frags/8)*2+(text>frags/4));
        assert(sb.size()==0);
        ht[ht.size()-frags].csize=-1;  // compressed size to fill in later
        frags=redundancy=text=exe=0;
      }
      continue;
    }

    // If no file is open then open the next one
    assert(fi<vf.size());
    if (!in.isopen()) {
      assert(fj==0);
  
      // Skip directory
      DTMap::iterator p=vf[fi];
      string filename=rename(p->first);
      if (filename!="" && filename[filename.size()-1]=='/') {
        if (quiet==0) {
          printf("Adding directory ");
          printUTF8(p->first.c_str());
          printf("\n");
        }
        ++fi;
        continue;
      }

      // Open input file
      if (!in.open(filename.c_str())) {  // skip if not found
        p->second.edate=0;
        ++fi;
        continue;
      }
      else if (quiet<=p->second.esize) {
        printf("%6u ", (unsigned)ht.size());
        if (p->second.dtv.size()==0 || p->second.dtv.back().date==0) {
          printf("Adding %1.0f ", double(p->second.esize));
          printUTF8(p->first.c_str());
        }
        else {
          printf("Updating %1.0f ", double(p->second.esize));
          printUTF8(p->first.c_str());
        }
        if (p->first!=filename) {
          printf(" from ");
          printUTF8(filename.c_str());
        }
        printf("\n");
      }
    }

    // Read a fragment
    assert(in.isopen());
    int c=0;  // current byte
    int c1=0;  // previous byte
    unsigned h=0;  // rolling hash for finding fragment boundaries
    unsigned sz=0;  // fragment size;
    libzpaq::SHA1 sha1;
    unsigned char o1[256]={0};
    unsigned hits=0;
    while (true) {
      c=in.get();
      if (c!=EOF) {
        sb.put(c);
        if (c==o1[c1]) h=(h+c+1)*314159265u, ++hits;
        else h=(h+c+1)*271828182u;
        o1[c1]=c;
        c1=c;
        sha1.put(c);
        ++sz;
      }
      if (c==EOF || (h<65536 && sz>=MIN_FRAGMENT) || sz>=MAX_FRAGMENT)
        break;
    }
    inputsize+=sz;

    // Look for matching fragment
    char sh[20];
    assert(sz==sha1.usize());
    memcpy(sh, sha1.result(), 20);
    unsigned j=htinv.find(sh);
    if (j==0) {  // not matched, add 
      j=ht.size();
      ht.push_back(HT(sh, sz, 0));
      ++frags;
      htinv.update();

      // Analyze fragment for redundancy, x86, text.
      // Test for text: letters, digits, '.' and ',' followed by spaces
      //   and no unprintable control chars.
      // Text for exe: 139 (mov reg, r/m) in lots of contexts.
      // 4 tests for redundancy, measured as hits/sz. Take the highest of:
      //   1. Successful prediction count in o1.
      //   2. Non-uniform distribution in o1 (counted in o2).
      //   3. Fraction of zeros in o1 (bytes never seen).
      //   4. Fraction of matches between o1 and previous o1 (o1prev).
      int text1=0, exe1=0;
      int64_t h1=sz;
      unsigned char o1ct[256]={0};  // counts of bytes in o1
      static const unsigned char dt[256]={  // 32768/((i+1)*204)
        160,80,53,40,32,26,22,20,17,16,14,13,12,11,10,10,
          9, 8, 8, 8, 7, 7, 6, 6, 6, 6, 5, 5, 5, 5, 5, 5,
          4, 4, 4, 4, 4, 4, 4, 4, 3, 3, 3, 3, 3, 3, 3, 3,
          3, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
          2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
          1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
          1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
          1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
          1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
          1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
      for (int i=0; i<256; ++i) {
        if (o1ct[o1[i]]<255) h1-=(sz*dt[o1ct[o1[i]]++])>>15;
        if (o1[i]==' ' && (isalnum(i) || i=='.' || i==',')) ++text1;
        if (o1[i]>=1 && o1[i]<32 && o1[i]!=9 && o1[i]!=10 && o1[i]!=13)
          --text1;
        if (o1[i]==139) ++exe1;
      }
      exe+=exe1>=5;
      text+=text1>=5;
      if (sz>0) h1=h1*h1/sz; // Test 2: near 0 if random.
      unsigned h2=h1;
      if (h2>hits) hits=h2;
      h2=o1ct[0]*sz/256;  // Test 3: bytes never seen or that predict 0.
      if (h2>hits) hits=h2;
      h2=0;
      for (int i=0; i<256*ON; ++i)  // Test 4: compare to previous o1.
        h2+=o1prev[i]==o1[i&255];
      h2=h2*sz/(256*ON);
      if (sz>=MIN_FRAGMENT) {
        memmove(o1prev, o1prev+256, 256*(ON-1));
        memcpy(o1prev+256*(ON-1), o1, 256);
      }
      if (h2>hits) hits=h2;
      if (hits>sz) hits=sz;
      redundancy+=hits;
    }
    else { // matched, discard
      assert(sz<=sb.size());
      sb.resize(sb.size()-sz);
    }

    // Point file to this fragment
    vector<unsigned>& eptr=vf[fi]->second.eptr;
    while (eptr.size()<=fj) eptr.push_back(0);
    eptr[fj++]=j;

    // Close file at EOF
    if (c==EOF) {
      in.close();
      ++fi;
      fj=0;
    }
  }

  // Wait for jobs to finish
  assert(frags==0);
  assert(fi==vf.size());
  assert(fj==0);
  assert(sb.size()==0);
  assert(!in.isopen());
  job.write(sb, 0, "");  // signal end of input
  for (int i=0; i<threads; ++i)
    join(tid[i]);
  join(wid);

  // Fill in compressed sizes in ht
  unsigned j=0;
  for (unsigned i=htsize; i<ht.size() && j<job.csize.size(); ++i)
    if (ht[i].csize==-1)
      ht[i].csize=job.csize[j++];
  assert(j==job.csize.size());

  // Append compressed fragment tables to archive
  if (quiet<MAX_QUIET)
    printf("Updating index with %d files, %d blocks, %d fragments\n",
            int(vf.size()), j, int(ht.size()-htsize));
  int64_t cdatasize=(archive=="" ? counter.pos : out.tell())-header_end;
  StringBuffer is;
  unsigned block_start=0;
  for (unsigned i=htsize; i<=ht.size(); ++i) {
    if ((i==ht.size() || ht[i].csize>0) && is.size()>0) {  // write a block
      assert(block_start>=htsize && block_start<i);
      compressBlock(&is, outp, "0",
                    ("jDC"+itos(date, 14)+"h"+itos(block_start, 10)).c_str());
      assert(is.size()==0);
    }
    if (i<ht.size()) {
      if (ht[i].csize) is+=itob(ht[i].csize), block_start=i;
      is+=string(ht[i].sha1, ht[i].sha1+20)+itob(ht[i].usize);
    }
  }
  assert(is.size()==0);

  // Append compressed index to archive
  int dtcount=0;
  for (DTMap::const_iterator p=dt.begin(); p!=dt.end();) {
    const DT& dtr=p->second;

    // Remove file if external does not exist and is currently in archive
    if (dtr.written==0 && !dtr.edate && dtr.dtv.size()
        && dtr.dtv.back().date) {
      is+=ltob(0)+p->first+'\0';
      if (quiet<=dtr.dtv.back().size) {
        printf("Removing ");
        printUTF8(p->first.c_str());
        printf("\n");
      }
    }

    // Update file if compressed and anything differs
    if (p->second.edate && (force || p->second.dtv.size()==0
       || p->second.edate!=p->second.dtv.back().date)) {
      if (dtr.dtv.size()==0 // new file
         || dtr.edate!=dtr.dtv.back().date  // date change
         || dtr.eattr!=dtr.dtv.back().attr  // attr change
         || dtr.eptr!=dtr.dtv.back().ptr) { // content change
        is+=ltob(dtr.edate)+p->first+'\0';
        if ((dtr.eattr&255)=='u') {  // unix attributes
          is+=itob(3);
          is.put('u');
          is.put(dtr.eattr>>8&255);
          is.put(dtr.eattr>>16&255);
        }
        else if ((dtr.eattr&255)=='w') {  // windows attributes
          is+=itob(5);
          is.put('w');
          is+=itob(dtr.eattr>>8);
        }
        else is+=itob(0);
        is+=itob(size(dtr.eptr));  // list of frag pointers
        for (int i=0; i<size(dtr.eptr); ++i)
          is+=itob(dtr.eptr[i]);
      }
    }
    ++p;
    if (is.size()>16000 || (is.size()>0 && p==dt.end())) {
      compressBlock(&is, outp, "1",
                    ("jDC"+itos(date)+"i"+itos(++dtcount, 10)).c_str());
      assert(is.size()==0);
    }
    if (p==dt.end()) break;
  }

  // Back up and write the header
  int64_t archive_end=0;
  if (archive=="")
    archive_end=counter.pos;
  else {
    archive_end=out.tell();
    out.seek(header_pos, SEEK_SET);
    writeJidacHeader(&out, date, cdatasize, htsize);
  }
  if (quiet<MAX_QUIET)
    printf("%1.0f + (%1.0f -> %1.0f + %1.0f + %1.0f = %1.0f) = %1.0f\n",
           double(header_pos),
           double(inputsize),
           double(header_end-header_pos),
           double(cdatasize),
           double(archive_end-header_end-cdatasize),
           double(archive_end-header_pos),
           double(archive_end));
  if (archive!="") {
    assert(header_end==out.tell());
    out.close();
  }
}

/////////////////////////////// extract ///////////////////////////////

// Create directories as needed. For example if path="/tmp/foo/bar"
// then create directories /, /tmp, and /tmp/foo unless they exist.
// Change \ to / in path. Set date and attributes if not 0.
void makepath(string& path, int64_t date=0, int64_t attr=0) {
  for (int i=0; i<size(path); ++i) {
    if (path[i]=='\\' || path[i]=='/') {
      path[i]=0;
#ifdef unix
      int ok=!mkdir(path.c_str(), 0777);
#else
      int ok=CreateDirectory(utow(path.c_str()).c_str(), 0);
#endif
      if (ok && quiet==0) {
        printf("Created directory ");
        printUTF8(path.c_str());
        printf("\n");
      }
      path[i]='/';
    }
  }

  // Set date and attributes
  string filename=path;
  if (filename!="" && filename[filename.size()-1]=='/')
    filename=filename.substr(0, filename.size()-1);  // remove trailing slash
#ifdef unix
  if (date>0) {
    struct utimbuf ub;
    ub.actime=time(NULL);
    ub.modtime=unix_time(date);
    utime(filename.c_str(), &ub);
  }
  if ((attr&255)=='u')
    chmod(filename.c_str(), attr>>8);
#else
  for (int i=0; i<size(filename); ++i)  // change to backslashes
    if (filename[i]=='/') filename[i]='\\';
  if (date>0) {
    HANDLE out=CreateFile(utow(filename.c_str()).c_str(),
                          FILE_WRITE_ATTRIBUTES, 0, NULL, OPEN_EXISTING,
                          FILE_FLAG_BACKUP_SEMANTICS, NULL);
    if (out!=INVALID_HANDLE_VALUE) {
      setDate(out, date);
      CloseHandle(out);
    }
    else winError(filename.c_str());
  }
  if ((attr&255)=='w') {
    SetFileAttributes(utow(filename.c_str()).c_str(), attr>>8);
  }
#endif
}

// An extract job is a set of blocks with at least one file pointing to them.
// Blocks are extracted in separate threads, set READY -> WORKING.
// A block is extracted to memory up to the last fragment that has a file
// pointing to it. Then the checksums are verified. Then for each file
// pointing to the block, each of the fragments that it points to within
// the block are written in order.

struct Block {  // list of fragments
  int64_t offset;       // location in archive
  vector<DTMap::iterator> files;  // list of files pointing here
  unsigned start;       // index in ht of first fragment
  int size;             // number of fragments to decompress
  bool streaming;       // must decompress sequentially?
  enum {READY, WORKING, GOOD, BAD} state;
  Block(unsigned s, int64_t o):
    offset(o), start(s), size(0), state(READY) {}
};

struct ExtractJob {         // list of jobs
  Mutex mutex;              // protects state
  Mutex write_mutex;        // protects writing to disk
  int job;                  // number of jobs started
  vector<Block> block;      // list of blocks to extract
  Jidac& jd;                // what to extract
  OutputFile outf;          // currently open output file
  DTMap::iterator lastdt;   // currently open output file name
  double maxMemory;         // largest memory used by any block (test mode)
  ExtractJob(Jidac& j): job(0), jd(j), lastdt(j.dt.end()), maxMemory(0) {
    init_mutex(mutex);
    init_mutex(write_mutex);
  }
  ~ExtractJob() {
    destroy_mutex(mutex);
    destroy_mutex(write_mutex);
  }
};

// Decompress blocks in a job until none are READY
ThreadReturn decompressThread(void* arg) {
  ExtractJob& job=*(ExtractJob*)arg;
  int jobNumber=0;
  InputFile in;

  // Get job number
  lock(job.mutex);
  jobNumber=++job.job;
  release(job.mutex);

  // Open archive for reading
  if (!in.open(job.jd.archive.c_str())) return 0;
  StringBuffer out;

  // Look for next READY job
  for (unsigned i=0; i<job.block.size(); ++i) {
    Block& b=job.block[i];
    lock(job.mutex);
    if (b.state==Block::READY && b.size>0 && !b.streaming) {
      b.state=Block::WORKING;
      release(job.mutex);
    }
    else {
      release(job.mutex);
      continue;
    }

    // Get uncompressed size of block
    unsigned output_size=0;  // minimum size to decompress
    unsigned max_size=0;     // uncompressed full block size
    int j;
    assert(b.start>0);
    for (j=0; j<b.size; ++j) {
      assert(b.start+j<job.jd.ht.size());
      assert(job.jd.ht[b.start+j].usize>=0);
      assert(j==0 || job.jd.ht[b.start+j].csize==-j);
      output_size+=job.jd.ht[b.start+j].usize;
    }
    max_size=output_size+j*4+8;  // uncompressed full block size
    for (; b.start+j<job.jd.ht.size() && job.jd.ht[b.start+j].csize<0; ++j){
      assert(job.jd.ht[b.start+j].csize==-j);
      max_size+=job.jd.ht[b.start+j].usize+4;
    }

    // Decompress
    try {
      assert(b.start>0);
      assert(b.start<job.jd.ht.size());
      assert(b.size>0);
      assert(b.start+b.size<=job.jd.ht.size());
      in.seek(job.jd.ht[b.start].csize, SEEK_SET);
      libzpaq::Decompresser d;
      d.setInput(&in);
      out.reset();
      out.setLimit(max_size);
      d.setOutput(&out);
      if (!d.findBlock()) error("archive block not found");
      const int64_t now=mtime();
      while (d.findFilename()) {
        StringWriter comment;
        d.readComment(&comment);
        if (comment.s.size()>=5
            && comment.s.substr(comment.s.size()-5)==" jDC\x01") {
          while (out.size()<output_size && d.decompress(1<<14));
          break;
        }
        else {
          d.decompress();
          d.readSegmentEnd();
        }
      }
      if (quiet<=size(out)) {
        lock(job.mutex);
        printf("Job %d: [%d..%d] %1.0f -> %d (%1.3f sec)\n",
               jobNumber, b.start, b.start+b.size-1,
               double(in.tell()-job.jd.ht[b.start].csize),
               size(out), (mtime()-now)*0.001);
        release(job.mutex);
      }
      if (out.size()<output_size)
        error("unexpected end of compressed data");

      // Verify fragment checksums if present
      const char* q=out.c_str();
      for (unsigned j=b.start; !fragile && j<b.start+b.size; ++j) {
        libzpaq::SHA1 sha1;
        for (unsigned k=job.jd.ht[j].usize; k>0; --k) sha1.put(*q++);
        if (memcmp(sha1.result(), job.jd.ht[j].sha1, 20)) {
          for (int k=0; k<20; ++k) {
            if (job.jd.ht[j].sha1[k]) {  // all zeros is OK
              lock(job.mutex);
              fprintf(stderr, 
                     "Job %d: fragment %d size %d checksum failed\n",
                     jobNumber, j, job.jd.ht[j].usize);
              release(job.mutex);
              error("bad checksum");
            }
          }
        }
        lock(job.mutex);
        job.jd.ht[j].csize=EXTRACTED;
        release(job.mutex);
      }
    }
    catch (std::exception& e) {
      lock(job.mutex);
      fprintf(stderr, "Job %d: skipping frags %u-%u at offset %1.0f: %s\n",
              jobNumber, b.start, b.start+b.size-1,
              double(in.tell()), e.what());
      release(job.mutex);
      continue;
    }

    // Write the files in dt that point to this block
    lock(job.write_mutex);
    for (unsigned ip=0; ip<b.files.size(); ++ip) {
      DTMap::iterator p=b.files[ip];
      DT& dtr=p->second;
      if (dtr.written<0 || size(dtr.dtv)==0 
          || dtr.written>=size(dtr.dtv.back().ptr))
        continue;  // don't write

      // Look for pointers to this block
      vector<unsigned>& ptr=dtr.dtv.back().ptr;
      string filename="";
      int64_t offset=0;  // write offset
      for (unsigned j=0; j<ptr.size(); ++j) {
        if (ptr[j]<b.start || ptr[j]>=b.start+b.size) {
          offset+=job.jd.ht[ptr[j]].usize;
          continue;
        }

        // Close last opened file if different
        if (p!=job.lastdt) {
          if (job.outf.isopen()) {
            assert(job.lastdt!=job.jd.dt.end());
            assert(job.lastdt->second.dtv.size()>0);
            assert(job.lastdt->second.dtv.back().date);
            assert(job.lastdt->second.written
                   <size(job.lastdt->second.dtv.back().ptr));
            job.outf.close();
          }
          job.lastdt=job.jd.dt.end();
        }

        // Open file for output
        if (job.lastdt==job.jd.dt.end()) {
          filename=job.jd.rename(p->first);
          assert(!job.outf.isopen());
          if (dtr.written==0) {
            makepath(filename);
            if (quiet<=dtr.dtv.back().size) {
              printf("Job %d: extracting ", jobNumber);
              printUTF8(filename.c_str());
              printf("\n");
            }
            if (job.outf.open(filename.c_str()))  // create new file
              job.outf.truncate();
          }
          else
            job.outf.open(filename.c_str());  // update existing file
          if (!job.outf.isopen()) break;  // skip file if error
          else job.lastdt=p;
          assert(job.outf.isopen());
        }
        assert(job.lastdt==p);

        // Find block offset of fragment
        const char* q=out.c_str();
        for (unsigned k=b.start; k<ptr[j]; ++k)
          q+=job.jd.ht[k].usize;
        assert(q>=out.c_str());
        assert(q<=out.c_str()+out.size()-job.jd.ht[ptr[j]].usize);

        // Write the fragment and any consecutive fragments that follow
        assert(offset>=0);
        ++dtr.written;
        int usize=job.jd.ht[ptr[j]].usize;
        while (j+1<ptr.size() && ptr[j+1]==ptr[j]+1
               && ptr[j+1]<b.start+b.size) {
          ++dtr.written;
          assert(dtr.written<=size(ptr));
          usize+=job.jd.ht[ptr[++j]].usize;
        }
        assert(q+usize<=out.c_str()+out.size());
        job.outf.write(q, offset, usize);
        offset+=usize;
        if (dtr.written==size(ptr)) {  // close file
          assert(dtr.dtv.size()>0);
          assert(dtr.dtv.back().date);
          assert(job.lastdt!=job.jd.dt.end());
          assert(job.outf.isopen());
          job.outf.close(dtr.dtv.back().date, dtr.dtv.back().attr);
          job.lastdt=job.jd.dt.end();
        }
      }
    }

    // Last file
    release(job.write_mutex);
  }

  // Last block
  in.close();
  return 0;
}

// Extract files from archive. Return 1 if error else 0.
int Jidac::extract() {

  // Read HT, DT
  if (!read_archive())
    return 1;
  read_args(false);

  // Map fragments to blocks.
  // Mark blocks with unknown or large fragment sizes as streaming.
  ExtractJob job(*this);
  vector<unsigned> hti(ht.size());  // fragment index -> block index
  int64_t usize=0;
  for (unsigned i=1; i<ht.size(); ++i) {
    if (ht[i].csize>=0) {
      job.block.push_back(Block(i, ht[i].csize));
      usize=0;
    }
    assert(job.block.size()>0);
    hti[i]=job.block.size()-1;
    if (ht[i].usize<0 || ht[i].usize>(1<<24))
      job.block.back().streaming=true;
    if (usize<0 || ht[i].usize<0) usize=-1;
    else usize+=ht[i].usize;
  }

  // Don't clobber
  for (DTMap::iterator p=dt.begin(); p!=dt.end(); ++p) {
    if (p->second.dtv.size() && p->second.dtv.back().date
        && p->second.written==0) {
      if (!force) {
        if (exists(rename(p->first))) {
          fprintf(stderr, "File exists: ");
          printUTF8(rename(p->first).c_str(), stderr);
          fprintf(stderr, "\n");
          error("won't clobber existing files without -force");
        }
      }

      // Make a list of files and the number of fragments to extract
      // from each block. If the file size is unknown, then mark
      // all blocks that it points to as streaming.
      assert(p->second.dtv.size()>0);
      for (unsigned i=0; i<p->second.dtv.back().ptr.size(); ++i) {
        unsigned j=p->second.dtv.back().ptr[i];
        if (j==0 || j>=ht.size()) {
          fprintf(stderr, "%s: bad frag IDs, skipping\n", p->first.c_str());
          continue;
        }
        assert(j>0 && j<ht.size());
        assert(ht.size()==hti.size());
        int64_t c=-ht[j].csize;
        if (c<0) c=0;  // position of fragment in block
        j=hti[j];  // block index
        assert(j>=0 && j<job.block.size());
        if (job.block[j].size<=c) job.block[j].size=c+1;
        if (job.block[j].files.size()==0 || job.block[j].files.back()!=p)
          job.block[j].files.push_back(p);
        if (p->second.dtv.back().size<0) job.block[j].streaming=true;
      }
    }
  }

  // Decompress archive in parallel
  if (quiet<MAX_QUIET) printf("Starting %d decompression jobs\n", threads);
  vector<ThreadID> tid(threads);
  for (int i=0; i<size(tid); ++i) run(tid[i], decompressThread, &job);

  // Decompress streaming files in a single thread
  InputFile in;
  if (!in.open(archive.c_str())) return 1;
  OutputFile out;
  DTMap::iterator p=dt.end();  // currently open output file (initially none)
  string lastfile=archive;  // default output file: drop .zpaq from archive
  if (lastfile.size()>5 && lastfile.substr(lastfile.size()-5)==".zpaq")
    lastfile=lastfile.substr(0, lastfile.size()-5);
  bool first=true;
  for (unsigned i=0; i<job.block.size(); ++i) {
    Block& b=job.block[i];
    if (b.size==0 || !b.streaming) continue;
    if (quiet<MAX_QUIET)
      printf("main:  [%d..%d] block %d\n", b.start, b.start+b.size-1, i+1);
    try {
      libzpaq::Decompresser d;
      libzpaq::SHA1 sha1;
      d.setInput(&in);
      d.setSHA1(&sha1);
      if (out.isopen()) d.setOutput(&out);
      else d.setOutput(0);
      in.seek(b.offset, SEEK_SET);
      if (!d.findBlock()) error("findBlock failed");
      StringWriter filename;
      
      // decompress segments
      for (int j=0; d.findFilename(&filename); ++j) {
        d.readComment();

        // Named segment starts new file
        if (filename.s.size()>0 || first) {
          for (unsigned i=0; i<filename.s.size(); ++i)
            if (filename.s[i]=='\\') filename.s[i]='/';
          if (filename.s.size()>0) lastfile=filename.s;
          if (out.isopen()) {
            out.close();
            p=dt.end();
          }
          first=false;
          string newfile;
          p=dt.find(lastfile);
          if (p!=dt.end() && p->second.written==0) {  // todo
            newfile=rename(lastfile);
            makepath(newfile);
            if (out.open(newfile.c_str())) {
              if (quiet<MAX_QUIET) {
                printf("main: extracting ");
                printUTF8(newfile.c_str());
                printf("\n");
              }
              out.truncate(0);
            }
            if (out.isopen()) d.setOutput(&out);
            else d.setOutput(0), p=dt.end();
          }
        }
        filename.s="";

        // Decompress, verify checksum
        if (j<b.size) {
          d.decompress();
          char sha1out[21];
          d.readSegmentEnd(sha1out);
          if (!fragile && sha1out[0] && memcmp(sha1out+1, sha1.result(), 20))
            error("checksum error");
          else {
            assert(b.start+j<ht.size());
            lock(job.mutex);
            ht[b.start+j].csize=EXTRACTED;
            release(job.mutex);
            if (p!=dt.end()) ++p->second.written;
          }
        }
        else
          break;
      }
    }
    catch (std::exception& e) {
      fprintf(stderr, "main: skipping frags %u-%u at offset %1.0f: %s\n",
              b.start, b.start+b.size-1, double(in.tell()), e.what());
      continue;
    }
  }

  // Wait for threads to finish
  for (int i=0; i<size(tid); ++i) join(tid[i]);

  // Create empty directories and set directory dates and attributes
  for (DTMap::reverse_iterator p=dt.rbegin(); p!=dt.rend(); ++p) {
    if (p->second.written==0 && p->first!=""
        && p->first[p->first.size()-1]=='/') {
      string s=rename(p->first);
      if (p->second.dtv.size())
        makepath(s, p->second.dtv.back().date, p->second.dtv.back().attr);
    }
  }

  // Report failed extractions
  unsigned extracted=0, errors=0;
  for (DTMap::iterator p=dt.begin(); p!=dt.end(); ++p) {
    if (p->second.dtv.size() && p->second.dtv.back().date
          && p->second.written>=0) {
      DTV& dtv=p->second.dtv.back();
      ++extracted;
      unsigned f=0;  // fragments extracted OK
      for (unsigned j=0; j<dtv.ptr.size(); ++j) {
        const unsigned k=dtv.ptr[j];
        if (k>0 && k<ht.size() && ht[k].csize==EXTRACTED) ++f;
      }
      if (f!=dtv.ptr.size() || f!=unsigned(p->second.written)) {
        if (++errors==1)
          fprintf(stderr,
          "\nFailed (extracted,written/total fragments, version, file):\n");
        fprintf(stderr, "%u,%u/%u %d ",
                f, p->second.written, int(dtv.ptr.size()), dtv.version);
        printUTF8(rename(p->first).c_str(), stderr);
        fprintf(stderr, "\n");
      }
    }
  }
  if (quiet<MAX_QUIET || errors>0) {
    fprintf(stderr, "Extracted %u of %u files OK (%u errors)\n",
           extracted-errors, extracted, errors);
  }
  return errors>0;
}

/////////////////////////////// test //////////////////////////////////

// Read fragments ht[n..n+f-1] and verify their checksums and redundant
// size list at end.
class OutTester: public libzpaq::Writer {
  const vector<HT>& ht;  // list of fragments sizes and checksums
  unsigned n;  // first fragment
  unsigned f;  // number of fragments
  unsigned b;  // last 4 bytes, little-endian
  unsigned frag; // current fragment n..n+f-1, or n+f if reading list
  unsigned pos;  // current position in fragment or list
  libzpaq::SHA1 sha1;
public:
  OutTester(const vector<HT>& ht_):
            ht(ht_), n(0), f(0), b(0), frag(n), pos(0) {}
  void init(unsigned n_, unsigned f_) {
    frag=n=n_;
    f=f_;
    assert(n>0);
    assert(n+f<=ht.size());
    while (frag<n+f && ht[frag].usize<=0) ++frag;  // skip empty frags
  }
  void put(int c);
  void write(const char* buf, int n_) {
    while (n_-->0) put(*buf++);
  }
};

// Write byte c
void OutTester::put(int c) {
  assert(frag>=n && frag<=n+f);
  ++pos;
  if (frag==n+f) {  // reading size list
    b=b>>8|c<<24;
    if (pos>f*4+8) error("wrote past end of block");
    if (pos%4==0) {
      if (pos<=f*4 && int(b)!=ht[n+pos/4-1].usize
          && (pos>8 || (pos==4 && b!=0 && b!=n) || (pos==8 && b!=0 && b!=f)))
        error("bad frag size");
      if (pos==f*4+4 && b!=n && b!=0) error("bad frag start");
      if (pos==f*4+8 && b!=f && b!=0) error("bad frag list size");
    }
  }
  else {
    assert(ht[frag].usize>0);
    sha1.put(c);
    if (int(pos)==ht[frag].usize) {
      assert(sha1.usize()==pos);
      if (memcmp(sha1.result(), ht[frag].sha1, 20)) {
        fprintf(stderr, "fragment %u checksum error\n", frag);
        error("bad frag checksum");
      }
      pos=0;
      while (++frag<n+f && ht[frag].usize<=0);
    }
  }
}

// Test blocks in a job until none are READY
ThreadReturn testThread(void* arg) {
  ExtractJob& job=*(ExtractJob*)arg;
  int jobNumber=0;
  InputFile in;

  // Get job number
  lock(job.mutex);
  jobNumber=++job.job;

  // Open archive for reading
  if (!in.open(job.jd.archive.c_str())) {
    release(job.mutex);
    return 0;
  }
  release(job.mutex);

  // Look for next READY job. Streaming blocks must be extracted in
  // sequential order in the first thread.
  for (unsigned i=0; i<job.block.size(); ++i) {
    Block& b=job.block[i];
    lock(job.mutex);
    if (b.state==Block::READY) {
      b.state=Block::WORKING;
      release(job.mutex);
    }
    else {
      release(job.mutex);
      continue;
    }

    // Decompress and verify checksums
    StringWriter filename;
    try {
      in.seek(job.jd.ht[b.start].csize, SEEK_SET);
      libzpaq::Decompresser d;
      d.setInput(&in);
      double memory=0;
      if (!d.findBlock(&memory)) error("archive block not found");
      lock(job.mutex);
      if (memory>job.maxMemory) job.maxMemory=memory;
      release(job.mutex);
      while (d.findFilename(&filename)) {
        StringWriter comment;
        d.readComment(&comment);

        // Test JIDAC format
        int64_t outsize=-1;  // uncompressed size
        OutTester out(job.jd.ht);
        if (comment.s.size()>4
            && comment.s.substr(comment.s.size()-4)=="jDC\x01") {
          if (filename.s.size()!=28) error("bad filename size");
          assert(filename.s.size()==28);
          if (filename.s.substr(0, 3)!="jDC") error("bad filename prefix");
          if (filename.s[17]!='d') error("bad filename type");
          if (unsigned(atol(filename.s.c_str()+18))!=b.start)
            error("bad fragment id in filename");
          outsize=8;  // output size or -1 if unknown
          for (unsigned i=b.start; i<b.start+b.size; ++i) {
            assert(i>0 && i<job.jd.ht.size());
            outsize+=job.jd.ht[i].usize+4;
          }
          int commentSize=atoi(comment.s.c_str());
          if (commentSize!=outsize && commentSize!=outsize-4*b.size)
            error("bad size in comment");
          out.init(b.start, b.size);
          d.setOutput(&out);
        }

        // Test size and checksum
        libzpaq::SHA1 sha1;
        d.setSHA1(&sha1);
        char sha1result[21]={0};
        d.decompress();
        d.readSegmentEnd(sha1result);
        const int64_t dsize=sha1.usize();
        if (quiet<MAX_QUIET) {
          lock(job.mutex);
          if (sha1result[0]!=1) printf("NOT CHECKED: ");
          printf("%d/%d %s (%1.3f MB) %1.0f -> %1.0f\n",
                 i+1, size(job.block), filename.s.c_str(), memory*0.000001,
                 double(in.tell()-job.jd.ht[b.start].csize), double(dsize));
          release(job.mutex);
        }
        if (outsize>=0 && outsize!=dsize && outsize!=dsize+4*b.size)
          error("wrong decompressed size");
        if (sha1result[0] && memcmp(sha1.result(), sha1result+1, 20))
          error("checksum mismatch");
        filename.s="";
      }

      // Mark successfully extracted fragments
      lock(job.mutex);
      b.state=Block::GOOD;
      for (unsigned i=b.start; i<b.start+b.size; ++i)
        job.jd.ht[i].csize=EXTRACTED;
      release(job.mutex);
    }
    catch (std::exception& e) {
      lock(job.mutex);
      fprintf(stderr, "Job %d: %s [%u-%u] at offset %1.0f: %s\n",
              jobNumber, filename.s.c_str(),
              b.start, b.start+b.size-1, double(in.tell()), e.what());
      b.state=Block::BAD;
      release(job.mutex);
      continue;
    }
  }

  // Last block
  in.close();
  return 0;
}

// Test archive. Throw error() if bad.
void Jidac::test() {

  // Report basic stats: versions, fragments, files, bytes
  printf("Testing %s\n", archive.c_str());
  int errors=0;
  bool iserr=false;
  int64_t archive_end=read_archive(&errors);
  printf("%1.0f bytes read from archive\n", double(archive_end));
  if (errors) {
    printf("%d errors found in index\n", errors);
    iserr=true;
  }

  // Report version statistics
  printf("\n%u versions\n", size(ver)-1);
  int updates=0, deletes=0, undated=0;
  int64_t earliest=0, latest=0;
  errors=0;
  for (unsigned i=1; i<ver.size(); ++i) {
    updates+=ver[i].updates;
    deletes+=ver[i].deletes;
    undated+=ver[i].date==0;
    if (ver[i].date) {
      if (earliest==0) earliest=ver[i].date;
      if (ver[i].date<=latest) ++errors, iserr=true;
      latest=ver[i].date;
    }
  }
  printf("%u file additions or updates\n", updates);
  printf("%u file deletions\n", deletes);
  printf("%s is the first version\n", dateToString(earliest).c_str());
  printf("%s is the latest version\n", dateToString(latest).c_str());
  printf("%u undated versions\n", undated);
  printf("%u version dates are out of sequence\n", errors);

  // Report ht statistics
  printf("\n%u fragments\n", size(ht)-1);
  int64_t usize=0;
  int unknown=0, blocks=0, nohash=0, largestFragment=0;
  double blockSize=0, largestBlock=0;
  errors=0;
  for (unsigned i=1; i<ht.size(); ++i) {
    if (ht[i].csize>=0) ++blocks, blockSize=0;
    if (ht[i].usize<0) ++unknown;
    else {
      usize+=ht[i].usize;
      if (ht[i].usize>largestFragment) largestFragment=ht[i].usize;
      blockSize+=ht[i].usize;
      if (blockSize>largestBlock) largestBlock=blockSize;
    }
    if (ht[i].csize>archive_end || ht[i].csize<-int(i)) ++errors;
    unsigned j;
    for (j=0; j<20; ++j)
      if (ht[i].sha1[j]) break;
   nohash+=j==20;
  }
  printf("%u blocks\n", blocks);
  printf("%1.0f known uncompressed bytes\n", double(usize));
  if (size(ht)-unknown>1)
    printf("%1.3f is average fragment size\n", usize/(size(ht)-unknown-1.0));
  printf("%u is the largest fragment size\n", largestFragment);
  printf("%1.0f is the largest uncompressed block size\n", largestBlock);
  printf("%u fragments of unknown size\n", unknown);
  printf("%u fragments without hashes\n", nohash);
  printf("%u missing fragments\n", errors);

  // Report dt statistics
  printf("\n%u files\n", size(dt));
  int files=0, versions=0, deleted=0, fragments=0, selected=0;
  usize=0;
  int64_t current=0;
  vector<bool> ref(ht.size());  // true if fragment is referenced
  DTMap::iterator largest=dt.end();
  for (DTMap::iterator p=dt.begin(); p!=dt.end(); ++p) {
    for (unsigned i=0; i<p->second.dtv.size(); ++i) {
      ++versions;
      fragments+=p->second.dtv[i].ptr.size();
      if (i+1==p->second.dtv.size()) {
        p->second.dtv[i].date==0 ? ++deleted : ++files;
        if (largest==dt.end()
            || p->second.dtv[i].size>largest->second.dtv[i].size)
          largest=p;
        selected+=p->second.written==0;
      }
      for (unsigned j=0; j<p->second.dtv[i].ptr.size(); ++j) {
        unsigned k=p->second.dtv[i].ptr[j];
        if (k<1 || k>=ht.size() || ht[k].csize>archive_end
            || ht[k].csize<-int(k)) {
          fprintf(stderr, 
                 "File %s version %u fragment %u out of range: %u\n",
                  p->first.c_str(), p->second.dtv[i].version, i, k);
          error("index corrupted");
        }
        ref[k]=true;
        if (ht[k].usize>=0) {
          usize+=ht[k].usize;
          if (i+1==p->second.dtv.size()) current+=ht[k].usize;
        }
      }
    }
  }
  printf("%u file versions\n", versions);
  printf("%u files in current version\n", files);
  printf("%u files selected by command line arguments\n", selected);
  printf("%u deleted files in current version\n", deleted);
  printf("%u references to fragments\n", fragments);
  printf("%1.0f known uncompressed bytes in all versions\n", double(usize));
  printf("%1.0f in current version\n", double(current));
  if (current>0)
    printf("%1.3f%% compression ratio\n", archive_end*100.0/current);
  if (largest!=dt.end()) {
    printf("%1.0f is size of the largest file, ",
           double(largest->second.dtv.back().size));
    printUTF8(largest->first.c_str());
    printf("\n");
  }
  errors=0;
  for (unsigned i=1; i<ref.size(); ++i) errors+=!ref[i];
  printf("%u unreferenced fragments\n", errors);

  // Make a list of blocks to decompress
  ExtractJob job(*this);
  for (unsigned i=1; i<ht.size(); ++i) {
    if (ht[i].csize>=0) {
      job.block.push_back(Block(i, ht[i].csize));
    }
    assert(job.block.size()>0);
    ++job.block.back().size;
  }

  // Decompress blocks in parallel
  printf("\nTesting %d blocks in %d threads\n", size(job.block), threads);
  vector<ThreadID> tid(threads);
  for (int i=0; i<size(tid); ++i) run(tid[i], testThread, &job);

  // Wait for threads to finish
  for (int i=0; i<size(tid); ++i) join(tid[i]);

  // Test for bad blocks
  errors=0;
  for (unsigned i=0; i<job.block.size(); ++i)
    if (job.block[i].state!=Block::GOOD)
      ++errors;
  printf("%1.3f MB memory per thread needed to decompress\n",
         job.maxMemory*0.000001);
  printf("\n%d data blocks bad\n", errors);
  iserr|=errors>0;

  // Report damaged files
  errors=0;
  int tested=0;
  for (DTMap::iterator p=dt.begin(); p!=dt.end(); ++p) {
    for (unsigned i=0; i<p->second.dtv.size(); ++i) {
      ++tested;
      unsigned j=0;
      for (; j<p->second.dtv[i].ptr.size(); ++j) {
        unsigned k=p->second.dtv[i].ptr[j];
        if (k<1 || k>=ht.size() || ht[k].csize!=EXTRACTED) break;
      }
      if (j!=p->second.dtv[i].ptr.size()) {
        if (++errors==1)
          printf("\nDamaged files:\n");
        printf("%d ", p->second.dtv[i].version);
        printUTF8(p->first.c_str());
        if (i+1<p->second.dtv.size())
          printf(" (%d'th of %d versions)", i+1, size(p->second.dtv));
        printf("\n");
      }
    }
  }
  iserr|=errors>0;
  printf("%d of %d files damaged\n\n", errors, tested);
  if (iserr) error("archive corrupted");
}

/////////////////////////////// list //////////////////////////////////

// For counting files and sizes by -list -summary
struct TOP {
  int64_t size;
  int count;
  TOP(): size(0), count(0) {}
  void inc(int64_t n) {size+=n; ++count;}
};

void Jidac::list_versions(int64_t csize) {
  printf("\n"
         "Ver Last frag Date      Time (UT) Files Deleted"
         "   Original MB  Compressed MB\n"
         "---- -------- ---------- -------- ------ ------ "
         "-------------- --------------\n");
  for (int i=0; i<size(ver); ++i) {
    int64_t osize=((i<size(ver)-1 ? ver[i+1].offset : csize)-ver[i].offset);
    if (i==0 && ver[i].updates==0
        && ver[i].deletes==0 && ver[i].date==0 && ver[i].usize==0)
      continue;
    printf("%4d %8d %s %6d %6d %14.6f %14.6f\n", i,
      i<size(ver)-1 ? ver[i+1].firstFragment-1 : size(ht)-1,
      dateToString(ver[i].date).c_str(),
      ver[i].updates, ver[i].deletes, ver[i].usize/1000000.0,
      osize/1000000.0);
  }
}

// List contents
void Jidac::list() {

  // Summary. Show only the largest files and directories, sorted by size,
  // and block and fragment usage statistics.
  const int64_t csize=read_archive();
  if (csize==0) exit(1);
  if (summary) {
    read_args(false);

    // Report biggest files, directories, and extensions
    printf("\nRank      Size (MB)     Files File, Directory/, or .Type\n"
             "---- -------------- --------- --------------------------\n");
    map<string, TOP> top;  // filename or dir -> total size and count
    vector<int> frag(ht.size());  // frag ID -> reference count
    int unknown_ref=0;  // count fragments and references with unknown size
    int unknown_size=0;
    for (DTMap::const_iterator p=dt.begin(); p!=dt.end(); ++p) {
      if (p->second.dtv.size() && p->second.dtv.back().date
          && p->second.written==0) {
        top[""].inc(p->second.dtv.back().size);
        top[p->first].inc(p->second.dtv.back().size);
        int ext=0;  // location of . in filename
        for (unsigned i=0; i<p->first.size(); ++i) {
          if (p->first[i]=='/') {
            top[p->first.substr(0, i+1)].inc(p->second.dtv.back().size);
            ext=0;
          }
          else if (p->first[i]=='.') ext=i;
        }
        if (ext)
          top[lowercase(p->first.substr(ext))].inc(p->second.dtv.back().size);
        else
          top["."].inc(p->second.dtv.back().size);
        for (unsigned i=0; i<p->second.dtv.back().ptr.size(); ++i) {
          const unsigned j=p->second.dtv.back().ptr[i];
          if (j<frag.size()) {
            ++frag[j];
            if (ht[j].usize<0) ++unknown_ref;
          }
        }
      }
    }
    map<int64_t, vector<string> > st;
    for (map<string, TOP>::const_iterator p=top.begin();
         p!=top.end(); ++p)
      st[-p->second.size].push_back(p->first);
    int i=1;
    for (map<int64_t, vector<string> >::const_iterator p=st.begin();
         p!=st.end() && i<=summary; ++p) {
      for (unsigned j=0; i<=summary && j<p->second.size(); ++i, ++j) {
        printf("%4d %14.6f %9d ", i, (-p->first)/1000000.0,
               top[p->second[j].c_str()].count);
        printUTF8(p->second[j].c_str());
        printf("\n");
      }
    }

    // Report block and fragment usage statistics
    printf("\nShares Fragments Deduplicated MB    Extracted MB\n"
             "------ --------- --------------- ---------------\n");
    map<unsigned, TOP> fr, frc; // refs -> deduplicated, extracted count, size
    for (unsigned i=1; i<frag.size(); ++i) {
      assert(i<ht.size());
      int j=frag[i];
      if (j>10) j=10;
      fr[j].inc(ht[i].usize);
      fr[-1].inc(ht[i].usize);
      frc[j].inc(int64_t(ht[i].usize)*frag[i]);
      frc[-1].inc(int64_t(ht[i].usize)*frag[i]);
      if (ht[i].usize<0) ++unknown_size;
    }
    for (map<unsigned, TOP>::const_iterator p=fr.begin(); p!=fr.end(); ++p) {
      if (int(p->first)==-1) printf(" Total ");
      else if (p->first==10) printf("   10+ ");
      else printf("%6u ", p->first);
      printf("%9d %15.6f %15.6f\n", p->second.count,
        p->second.size/1000000.0, frc[p->first].size/1000000.0);
    }

    // Print versions
    list_versions(csize);

    // Report fragments with unknown size
    printf("\n%d references to %d of %d fragments have unknown size.\n",
           unknown_ref, unknown_size, size(ht)-1);

    // Count blocks and used blocks
    int blocks=0, used=0, isused=0;
    for (unsigned i=1; i<ht.size(); ++i) {
      if (ht[i].csize>=0) {
        ++blocks;
        used+=isused;
        isused=0;
      }
      isused|=frag[i]>0;
    }
    used+=isused;
    const double usize=top[""].size;
    printf("%d of %d blocks used.\nCompression %1.6f -> %1.6f MB",
           used, blocks, usize/1000000.0, csize/1000000.0);
    if (usize>0) printf(" (ratio %1.3f%%)", csize*100.0/usize);
    printf("\n");
    return;
  }

  // Ordinary list
  int64_t usize=0;
  unsigned nfiles=0, shown=0;
  read_args(false, true);
  if (since<0) since+=ver.size();
  printf("\n"
    "Ver  Date      Time (UT) Attr           Size File\n"
    "---- ---------- -------- ------ ------------ ----\n");
  for (DTMap::const_iterator p=dt.begin(); p!=dt.end(); ++p) {
    if (p->second.written==0) {
      for (unsigned i=0; i<p->second.dtv.size(); ++i) {
        if (p->second.dtv[i].version>=since && p->second.dtv[i].size>=quiet
            && (all || (i+1==p->second.dtv.size() && p->second.dtv[i].date))){
          printf("%4d ", p->second.dtv[i].version);
          if (p->second.dtv[i].date) {
            ++shown;
            usize+=p->second.dtv[i].size;
            printf("%s %s %12.0f ",
                   dateToString(p->second.dtv[i].date).c_str(),
                   attrToString(p->second.dtv[i].attr).c_str(),
                   double(p->second.dtv[i].size));
          }
          else
            printf("%-40s", "Deleted");
          printUTF8(p->first.c_str());
          printf("\n");
        }
      }
    }
    if (p->second.dtv.size() && p->second.dtv.back().date) ++nfiles;
  }
  printf("%u of %u files shown. %1.0f -> %1.0f\n",
         shown, nfiles, double(usize), double(csize));
  list_versions(csize);
}

// Convert argv to UTF-8 and replace \ with /
#ifdef unix
int main(int argc, const char** argv) {
#else
#ifdef _MSC_VER
int wmain(int argc, LPWSTR* argw) {
#else
int main() {
  int argc=0;
  LPWSTR* argw=CommandLineToArgvW(GetCommandLine(), &argc);
#endif
  vector<string> args(argc);
  libzpaq::Array<const char*> argp(argc);
  for (int i=0; i<argc; ++i) {
    args[i]=wtou(argw[i]);
    argp[i]=args[i].c_str();
  }
  const char** argv=&argp[0];
#endif

  int64_t start=mtime();  // get start time
  init_mutex(global_mutex);
  int errorcode=0;
  try {
    Jidac jidac;
    errorcode=jidac.doCommand(argc, argv);
  }
  catch (std::exception& e) {
    fprintf(stderr, "zpaq exiting from main: %s\n", e.what());
    errorcode=1;
  }
  if (quiet<MAX_QUIET) {
    printf("%1.3f seconds", (mtime()-start)/1000.0);
    if (errorcode) printf(" (with errors)");
    printf("\n");
  }
  destroy_mutex(global_mutex);
  return errorcode;
}
