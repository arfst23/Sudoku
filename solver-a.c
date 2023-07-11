//******************************************************************************
//
// #define OPENMP 0
//
// gcc -fopenmp -march=native -O3 -funroll-loops -fprofile-generate -o solver solver.c
//
// solver < sudoku-20000000 > /dev/null
//
// gcc -fopenmp -march=native -O3 -funroll-loops -fprofile-use -o solver solver.c
//
// #define PTHREAD 1
//
// gcc -pthread -march=native -O3 -funroll-loops -fprofile-generate -o solver solver.c
//
// solver < sudoku-20000000 > /dev/null
//
// gcc -pthread -march=native -O3 -funroll-loops -fprofile-use -o solver solver.c
//
//******************************************************************************

#if FILEIO
#include <stdio.h>
#else
#include <unistd.h>
#endif
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#include <sys/types.h>
#include <sys/sysinfo.h>
#include <pthread.h>

#define N 9
#define UNDEF UINT8_MAX

typedef uint8_t Num;

typedef struct
{
  Num cnt;
  Num value[N][N];
  Num feasible[N][N][N];

  Num numRowCol[N][N];
  Num numRowVal[N][N];
  Num numColVal[N][N];
  Num numBlkVal[N][N];
} Sudoku;

//******************************************************************************
// >>> translate coordinates

// static const Num rowColToBlk[N][N] =
// {
//   { 0, 0, 0, 1, 1, 1, 2, 2, 2 },
//   { 0, 0, 0, 1, 1, 1, 2, 2, 2 },
//   { 0, 0, 0, 1, 1, 1, 2, 2, 2 },
//   { 3, 3, 3, 4, 4, 4, 5, 5, 5 },
//   { 3, 3, 3, 4, 4, 4, 5, 5, 5 },
//   { 3, 3, 3, 4, 4, 4, 5, 5, 5 },
//   { 6, 6, 6, 7, 7, 7, 8, 8, 8 },
//   { 6, 6, 6, 7, 7, 7, 8, 8, 8 },
//   { 6, 6, 6, 7, 7, 7, 8, 8, 8 }
// };
#define rowColToBlk blkIdxToRow

static const Num blkIdxToRow[N][N] =
{
  { 0, 0, 0, 1, 1, 1, 2, 2, 2 },
  { 0, 0, 0, 1, 1, 1, 2, 2, 2 },
  { 0, 0, 0, 1, 1, 1, 2, 2, 2 },
  { 3, 3, 3, 4, 4, 4, 5, 5, 5 },
  { 3, 3, 3, 4, 4, 4, 5, 5, 5 },
  { 3, 3, 3, 4, 4, 4, 5, 5, 5 },
  { 6, 6, 6, 7, 7, 7, 8, 8, 8 },
  { 6, 6, 6, 7, 7, 7, 8, 8, 8 },
  { 6, 6, 6, 7, 7, 7, 8, 8, 8 }
};

static const Num blkIdxToCol[N][N] =
{
  { 0, 1, 2, 0, 1, 2, 0, 1, 2 },
  { 3, 4, 5, 3, 4, 5, 3, 4, 5 },
  { 6, 7, 8, 6, 7, 8, 6, 7, 8 },
  { 0, 1, 2, 0, 1, 2, 0, 1, 2 },
  { 3, 4, 5, 3, 4, 5, 3, 4, 5 },
  { 6, 7, 8, 6, 7, 8, 6, 7, 8 },
  { 0, 1, 2, 0, 1, 2, 0, 1, 2 },
  { 3, 4, 5, 3, 4, 5, 3, 4, 5 },
  { 6, 7, 8, 6, 7, 8, 6, 7, 8 }
};

// <<<
//******************************************************************************
// >>> sub

#define COL 0
#define ROW 1
#define VAL 2
#define BLK 3

static Num add(Sudoku *sudoku, Num row, Num col, Num val);

static Num sub(Sudoku *sudoku, Num row, Num col, Num val, Num mode)
{
  if (!sudoku->feasible[row][col][val])
    return 0; // already done
  
  sudoku->feasible[row][col][val] = 0;

  if (!--sudoku->numRowVal[row][val])
    return 1; // exception
  if (sudoku->numRowVal[row][val] == 1 && mode != COL)
    for (Num c = 0; c < N; c++)
      if (sudoku->feasible[row][c][val])
	if (sudoku->value[row][c] != val)
	{
	  if (add(sudoku, row, c, val))
	    return 1; // exception
	}
	else
	  break;

  if (!--sudoku->numColVal[col][val])
    return 1; // exception
  if (sudoku->numColVal[col][val] == 1 && mode != ROW)
    for (Num r = 0; r < N; r++)
      if (sudoku->feasible[r][col][val])
	if (sudoku->value[r][col] != val)
	{
	  if (add(sudoku, r, col, val))
	    return 1; // exception
	}
	else
	  break;

  if (!--sudoku->numRowCol[row][col])
    return 1; // exception
  if (sudoku->numRowCol[row][col] == 1 && mode != VAL)
    for (Num v = 0; v < N; v++)
      if (sudoku->feasible[row][col][v])
	if (sudoku->value[row][col] != v)
	{
	  if (add(sudoku, row, col, v))
	    return 1; // exception
	}
	else
	  break;

  Num blk = rowColToBlk[row][col];
  if (!--sudoku->numBlkVal[blk][val])
    return 1; // exception
  if (sudoku->numBlkVal[blk][val] == 1 && mode != BLK)
    for (Num idx = 0; idx < N; idx++)
    {
      Num r = blkIdxToRow[blk][idx];
      Num c = blkIdxToCol[blk][idx];
      if (sudoku->feasible[r][c][val])
	if (sudoku->value[r][c] != val)
	{
	  if (add(sudoku, r, c, val))
	    return 1; // exception
	}
	else
	  break;
    }

  return 0; // done
}

// <<<
//******************************************************************************
// >>> add

static Num add(Sudoku *sudoku, Num row, Num col, Num val)
{
  if (sudoku->value[row][col] == val)
    return 0; // already done
  if (sudoku->value[row][col] != UNDEF)
    return 1; // exception

  sudoku->value[row][col] = val;
  sudoku->cnt--;

  for (Num c = 0; c < N; c++)
    if (c != col)
      if (sub(sudoku, row, c, val, COL))
	return 1; // exception

  for (Num r = 0; r < N; r++)
    if (r != row)
      if (sub(sudoku, r, col, val, ROW))
	return 1; // exception

  for (Num v = 0; v < N; v++)
    if (v != val)
      if (sub(sudoku, row, col, v, VAL))
	return 1; // exception

  Num blk = rowColToBlk[row][col];
  for (Num idx = 0; idx < N; idx++)
  {
    Num r = blkIdxToRow[blk][idx];
    Num c = blkIdxToCol[blk][idx];
    if (r != row && c != col)
      if (sub(sudoku, r, c, val, BLK))
	return 1; // exception
  }

  return 0; // done
}

// <<<
//******************************************************************************
// >>> branch

static Num branch(Sudoku *sudoku)
{
  if (!sudoku->cnt)
    return 0; // solved

  Num bestRowCol = N;
  Num row, col;
  Num num = 2 * N;
  for (Num r = 0; r < N; r++)
    for (Num c = 0; c < N; c++)
      if (sudoku->numRowCol[r][c] > 1) // find best branch point
      {
	Num numRowCol = sudoku->numRowCol[r][c];
	Num n = 0;
	for (Num v = 0; v < N; v++)
	  n += sudoku->numRowVal[r][v] + sudoku->numColVal[c][v];
	if (numRowCol < bestRowCol || (numRowCol == bestRowCol && n < num))
	{
	  bestRowCol = numRowCol;
	  row = r;
	  col = c;
	  num = n; 
	}
      }
  assert(bestRowCol < N);

  Sudoku backup;
  memcpy(&backup, sudoku, sizeof(Sudoku));

  for (Num val = 0; val < N; val++)
    if (sudoku->feasible[row][col][val])
    {
      if (!add(sudoku, row, col, val)
	&& !branch(sudoku))
	return 0; // solved

      memcpy(sudoku, &backup, sizeof(Sudoku));
    }
  return 1; // exception
}

// <<<
//******************************************************************************
// >>> solve

void solve(char *str)
{
  assert(str[N * N] == '\n');

  Sudoku sudoku;
  sudoku.cnt = N * N;
  memset(&sudoku.value, UNDEF, N * N);
  memset(&sudoku.feasible, 1, N * N * N);
  memset(&sudoku.numRowCol, N, N * N);
  memset(&sudoku.numRowVal, N, N * N);
  memset(&sudoku.numColVal, N, N * N);
  memset(&sudoku.numBlkVal, N, N * N);

  for (Num row = 0, idx = 0; row < N; row++)
    for (Num col = 0; col < N; col++, idx++)
      if (str[idx] >= '1' && str[idx] <= '9')
	add(&sudoku, row, col, (Num)(str[idx] - '1'));

  assert(!branch(&sudoku));

  for (Num row = 0, idx = 0; row < N; row++)
    for (Num col = 0; col < N; col++, idx++)
      str[idx] = (char)('1' + sudoku.value[row][col]);
}

// <<<
//******************************************************************************

#if OPENMP

#define CHUNKSIZE (128 * 8192)
#define STRLEN (N * N + 1)

int main()
{
  static char str[CHUNKSIZE][STRLEN];

  for (size_t chunks = CHUNKSIZE; chunks == CHUNKSIZE;)
  {
#if FILEIO
    chunks = fread(str, STRLEN, CHUNKSIZE, stdin);
#else
    chunks = read(STDIN_FILENO, str, STRLEN * CHUNKSIZE);
    chunks /= STRLEN;
#endif

#pragma omp parallel for schedule(runtime)
    for (size_t idx = 0; idx < chunks; idx++)
      solve(str[idx]);

#if FILEIO
    fwrite(str, STRLEN, chunks, stdout);
#else
    size_t bytes = write(STDOUT_FILENO, str, STRLEN * chunks);
#endif
  }

  return EXIT_SUCCESS;
}

#endif

//******************************************************************************

#if PTHREAD

#define recordLength (N * N + 1)

#define recordsPerBatchBits 4
#define recordsPerBatch (1 << recordsPerBatchBits)
#define recordsPerBatchMask (recordsPerBatch - 1)

#define batchesPerChunkBits 12
#define batchesPerChunk (1 << batchesPerChunkBits)
#define batchesPerChunkMask (batchesPerChunk - 1)

static char records[2][batchesPerChunk][recordsPerBatch][recordLength];

static pthread_mutex_t readMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t readCond = PTHREAD_COND_INITIALIZER;
static int nextReadChunkNumber = 2;

static pthread_mutex_t writeMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t writeCond = PTHREAD_COND_INITIALIZER;
static int nextWriteChunkNumber = 0;

static pthread_mutex_t parityMutex[2] = { PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER };
static pthread_cond_t parityCond[2] = { PTHREAD_COND_INITIALIZER, PTHREAD_COND_INITIALIZER };
static int currentChunkNumber[2];
static int batchesInChunk[2];
static int recordsInLastBatch[2];
static int solvedBatchesInChunk[2];

//******************************************************************************

void control(void *p)
{
  int id = (int)(long)p;
  int chunkNumberConfirmed[2] = { -1, -1 };
  for (;;)
  {
    static pthread_mutex_t jobMutex = PTHREAD_MUTEX_INITIALIZER;
    static int nextJobNumber = 0;
    pthread_mutex_lock(&jobMutex);
    int jobNumber = nextJobNumber++;
    pthread_mutex_unlock(&jobMutex);

    int batchInChunk = jobNumber & batchesPerChunkMask;
    int chunkNumber = jobNumber >> batchesPerChunkBits;
    int parity = chunkNumber & 1;

    if (chunkNumber > chunkNumberConfirmed[parity])
    {
      pthread_mutex_lock(&parityMutex[parity]);
      if (chunkNumber > currentChunkNumber[parity])
      {
	pthread_cond_wait(&parityCond[parity], &parityMutex[parity]);
	if (chunkNumber != currentChunkNumber[parity])
	{
	  assert(batchesInChunk[parity] == 0);
	  pthread_mutex_unlock(&parityMutex[parity]);
	  return;
	}
      }
      chunkNumberConfirmed[parity] = chunkNumber;
      pthread_mutex_unlock(&parityMutex[parity]);
    }
    if (batchInChunk >= batchesInChunk[parity])
      return;

    int recordsInChunk = batchInChunk == batchesInChunk[parity] - 1
      ? recordsInLastBatch[parity] : recordsPerBatch;
    for (int record = 0; record < recordsInChunk; record++)
      solve(&records[parity][batchInChunk][record][0]);

    pthread_mutex_lock(&parityMutex[parity]);
    if (++solvedBatchesInChunk[parity] == batchesInChunk[parity])
    {
      int recordsInChunk = (batchesInChunk[parity] - 1) * recordsPerBatch
	+ recordsInLastBatch[parity];
      pthread_mutex_lock(&writeMutex);
      if (currentChunkNumber[parity] > nextWriteChunkNumber)
      {
	pthread_cond_wait(&writeCond, &writeMutex);
        assert(currentChunkNumber[parity] == nextWriteChunkNumber);
      }

#if FILEIO
      fwrite(&records[parity][0][0][0], recordLength, recordsInChunk, stdout);
#else
      size_t bytes = write(STDOUT_FILENO, &records[parity][0][0][0],
	recordLength * recordsInChunk);
#endif
      nextWriteChunkNumber++;
      pthread_cond_signal(&writeCond);
      pthread_mutex_unlock(&writeMutex);

      pthread_mutex_lock(&readMutex);
      if (currentChunkNumber[parity] + 2 > nextReadChunkNumber)
      {
	pthread_cond_wait(&readCond, &readMutex);
	assert(currentChunkNumber[parity] + 2 == nextReadChunkNumber);
      }
#if FILEIO
      int recordsRead = fread(&records[parity][0][0][0],
        recordLength, batchesPerChunk * recordsPerBatch, stdin);
#else
      int recordsRead = read(STDIN_FILENO, &records[parity][0][0][0],
	batchesPerChunk * recordsPerBatch * recordLength);
      recordsRead /= recordLength;
#endif
      nextReadChunkNumber++;
      pthread_cond_signal(&readCond);
      pthread_mutex_unlock(&readMutex);

      currentChunkNumber[parity] += 2;
      batchesInChunk[parity] = recordsRead >> recordsPerBatchBits;
      recordsInLastBatch[parity] = recordsRead & recordsPerBatchMask;
      if (recordsInLastBatch[parity])
	batchesInChunk[parity]++;
      else if (recordsRead)
	recordsInLastBatch[parity] = recordsPerBatch;
      solvedBatchesInChunk[parity] = 0;
      pthread_cond_broadcast(&parityCond[parity]);
    }
    pthread_mutex_unlock(&parityMutex[parity]);
  }
}

//******************************************************************************

int main()
{
#if FILEIO
  int recordsRead = fread(&records[0][0][0][0],
    recordLength, 2 * batchesPerChunk * recordsPerBatch, stdin);
#else
  int recordsRead = read(STDIN_FILENO, &records[0][0][0][0],
    2 * batchesPerChunk * recordsPerBatch * recordLength);
  recordsRead /= recordLength;
#endif
  if (recordsRead < batchesPerChunk * recordsPerBatch)
  {
    currentChunkNumber[0] = 0;
    batchesInChunk[0] = recordsRead >> recordsPerBatchBits;
    recordsInLastBatch[0] = recordsRead & recordsPerBatchMask;
    if (recordsInLastBatch[0])
      batchesInChunk[0]++;
    else if (recordsRead)
      recordsInLastBatch[0] = recordsPerBatch;
    solvedBatchesInChunk[0] = 0;

    currentChunkNumber[1] = 1;
    batchesInChunk[1] = 0;
    recordsInLastBatch[1] = 0;
    solvedBatchesInChunk[1] = 0;
  }
  else // recordsRead >= batchesInChunk * recordsInBatch
  {
    currentChunkNumber[0] = 0;
    batchesInChunk[0] = batchesPerChunk;
    recordsInLastBatch[0] = recordsPerBatch;
    solvedBatchesInChunk[0] = 0;
    recordsRead -= batchesPerChunk * recordsPerBatch;

    currentChunkNumber[1] = 1;
    if (recordsRead < batchesPerChunk * recordsPerBatch)
    {
      batchesInChunk[1] = recordsRead >> recordsPerBatchBits;
      recordsInLastBatch[1] = recordsRead & recordsPerBatchMask;
      if (recordsInLastBatch[1])
	batchesInChunk[1]++;
      else if (recordsRead)
	recordsInLastBatch[1] = recordsPerBatch;
    }
    else // recordsRead >= batchesInChunk * recordsInBatch
    {
      batchesInChunk[1] = batchesPerChunk;
      recordsInLastBatch[1] = recordsPerBatch;
      solvedBatchesInChunk[1] = 0;
    }
    solvedBatchesInChunk[1] = 0;
  }

  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);

  int cores = get_nprocs();
  pthread_t threads[cores - 1];
  for (int i = 0; i < cores - 1; i++)
    pthread_create(&threads[i], &attr, (void*(*)(void*))control, (void*)(long)i);
  control((void*)(long)(cores - 1));

  for (int i = 0; i < cores - 1; i++)
    pthread_join(threads[i], NULL);
  return EXIT_SUCCESS;
}

#endif

//******************************************************************************
