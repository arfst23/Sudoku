#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#define N 9
#define P 6
#define R 2

#define STRLEN (N * N + 1)

static const int permutation[P][3] =
{
  { 0, 1, 2 },
  { 0, 2, 1 },
  { 1, 0, 2 },
  { 1, 2, 0 },
  { 2, 0, 1 },
  { 2, 1, 0 }
};

inline int idx(int row, int column)
{
  return N * row + column;
}

//******************************************************************************

static void relabel(const char *src, char *dst)
{
  int label[N] = { -1, -1, -1, -1, -1, -1, -1, -1, -1 };
  int lab = 0;
  for (int idx = 0; idx < N * N; idx++)
    if (src[idx] >= '1' && src[idx] <= '9')
    {
      int labSrc = src[idx] - '1';
      if (label[labSrc] < 0)
	label[labSrc] = lab++;
      int labDst = label[labSrc];
      dst[idx] = '1' + labDst;
    }
    else
      dst[idx] = src[idx];
  dst[N * N] = '\0';
}

//******************************************************************************

static void permuteRows(int perm, int inBand, const char *src, char *dst)
{
  for (int band = 0; band < 3; band++)
    for (int subDst = 0; subDst < 3; subDst++)
    {
      int subSrc = band == inBand ? permutation[perm][subDst] : subDst;
      int rowSrc = band * 3 + subSrc;
      int rowDst = band * 3 + subDst;
      for (int col = 0; col < N; col++)
      {
	int idxSrc = idx(rowSrc, col);
	int idxDst = idx(rowDst, col);
	dst[idxDst] = src[idxSrc];
      }
    }
  dst[N * N] = '\0';
}

static void permuteCols(int perm, int inBand, const char *src, char *dst)
{
  for (int band = 0; band < 3; band++)
    for (int subDst = 0; subDst < 3; subDst++)
    {
      int subSrc = band == inBand ? permutation[perm][subDst] : subDst;
      int colSrc = band * 3 + subSrc;
      int colDst = band * 3 + subDst;
      for (int row = 0; row < N; row++)
      {
	int idxSrc = idx(row, colSrc);
	int idxDst = idx(row, colDst);
	dst[idxDst] = src[idxSrc];
      }
    }
  dst[N * N] = '\0';
}

//******************************************************************************

static void permuteHorizontalBands(int perm, const char *src, char *dst)
{
  for (int bandDst = 0; bandDst < 3; bandDst++)
    for (int sub = 0; sub < 3; sub++)
    {
      int bandSrc = permutation[perm][bandDst];
      int rowSrc = bandSrc * 3 + sub;
      int rowDst = bandDst * 3 + sub;
      for (int col = 0; col < N; col++)
      {
	int idxSrc = idx(rowSrc, col);
	int idxDst = idx(rowDst, col);
	dst[idxDst] = src[idxSrc];
      }
    }
  dst[N * N] = '\0';
}

static void permuteVerticalBands(int perm, const char *src, char *dst)
{
  for (int bandDst = 0; bandDst < 3; bandDst++)
    for (int sub = 0; sub < 3; sub++)
    {
      int bandSrc = permutation[perm][bandDst];
      int colSrc = bandSrc * 3 + sub;
      int colDst = bandDst * 3 + sub;
      for (int row = 0; row < N; row++)
      {
	int idxSrc = idx(row, colSrc);
	int idxDst = idx(row, colDst);
	dst[idxDst] = src[idxSrc];
      }
    }
  dst[N * N] = '\0';
}

//******************************************************************************

static void reflect(int ref, const char *src, char *dst)
{
  if (!ref)
    memcpy(dst, src, STRLEN);
  else
    for (int rowDst = 0; rowDst < N; rowDst++)
      for (int colDst = 0; colDst < N; colDst++)
      {
	int rowSrc = colDst;
	int colSrc = rowDst;
	int idxSrc = idx(rowSrc, colSrc);
	int idxDst = idx(rowDst, colDst);
	dst[idxDst] = src[idxSrc];
      }
  dst[N * N] = '\0';
}

//******************************************************************************

static void normalize(char *sudoku0)
{
  char norm[STRLEN];
  norm[0] = 'X';
  norm[N * N] = '\0';

  for (int ref = 0; ref < R; ref++)
  {
    char sudoku1[STRLEN];
    reflect(ref, sudoku0, sudoku1);

    for (int perm = 0; perm < P; perm++)
    {
      char sudoku2[STRLEN];
      permuteHorizontalBands(perm, sudoku1, sudoku2);

      for (int perm = 0; perm < P; perm++)
      {
	char sudoku3[STRLEN];
	permuteVerticalBands(perm, sudoku2, sudoku3);
	
	for (int perm = 0; perm < P; perm++)
	{
	  char sudoku4[STRLEN];
	  permuteRows(perm, 0, sudoku3, sudoku4);

	  for (int perm = 0; perm < P; perm++)
	  {
	    char sudoku5[STRLEN];
	    permuteRows(perm, 1, sudoku4, sudoku5);
	  
	    for (int perm = 0; perm < P; perm++)
	    {
	      char sudoku6[STRLEN];
	      permuteRows(perm, 2, sudoku5, sudoku6);
	  
	      for (int perm = 0; perm < P; perm++)
	      {
		char sudoku7[STRLEN];
		permuteCols(perm, 0, sudoku6, sudoku7);

		for (int perm = 0; perm < P; perm++)
		{
		  char sudoku8[STRLEN];
		  permuteCols(perm, 1, sudoku7, sudoku8);

		  for (int perm = 0; perm < P; perm++)
		  {
		    char sudoku9[STRLEN];
		    permuteCols(perm, 2, sudoku8, sudoku9);

		    char sudoku[STRLEN];
		    relabel(sudoku9, sudoku);

		    if (strcmp(sudoku, norm) < 0)
		      memcpy(norm, sudoku, STRLEN);
		  }
		}
	      }
	    }
	  }
	}
      }
    }
    memcpy(sudoku0, norm, STRLEN - 1);
  }
}

//******************************************************************************

#define CHUNKSIZE 1000

int main()
{
  static char str[CHUNKSIZE][STRLEN];

  for (size_t chunks = CHUNKSIZE; chunks == CHUNKSIZE;)
  {
    chunks = fread(str, STRLEN, CHUNKSIZE, stdin);

#pragma omp parallel for schedule(runtime)
    for (size_t idx = 0; idx < chunks; idx++)
      normalize(str[idx]);

    fwrite(str, STRLEN, chunks, stdout);
  }

  return 0;
}

//******************************************************************************
