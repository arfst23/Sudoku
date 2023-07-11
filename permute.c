#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define N 9
#define L 362880
#define P 6
#define R 2

#define STRLEN (N * N + 1)

#define M 100

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

static void relabel(int perm, const char *src, char *dst)
{
  int label[N] = { -1, -1, -1, -1, -1, -1, -1, -1, -1 };
  for (int n = 9; n; n--)
  {
    int k =  perm % n;
    perm /= n;

    for (int i = 0; i < N; i++)
    {
      if (label[i] >= 0)
	continue;
      if (k)
	k--;
      else
      {
	label[i] = n - 1;
	break;
      }
    }
  }

  for (int idx = 0; idx < N * N; idx++)
    if (src[idx] >= '1' && src[idx] <= '9')
    {
      int labSrc = src[idx] - '1';
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

void permute(const char *src, char *dst)
{
  int ref = rand() & 1;
  char sudoku1[STRLEN];
  reflect(ref, src, sudoku1);

  int perm2 = rand() % 6;
  char sudoku2[STRLEN];
  permuteHorizontalBands(perm2, sudoku1, sudoku2);

  int perm3 = rand() % 6;
  char sudoku3[STRLEN];
  permuteVerticalBands(perm3, sudoku2, sudoku3);

  int perm4 = rand() % 6;
  char sudoku4[STRLEN];
  permuteRows(perm4, 0, sudoku3, sudoku4);

  int perm5 = rand() % 6;
  char sudoku5[STRLEN];
  permuteRows(perm5, 1, sudoku4, sudoku5);

  int perm6 = rand() % 6;
  char sudoku6[STRLEN];
  permuteRows(perm6, 2, sudoku5, sudoku6);

  int perm7 = rand() % 6;
  char sudoku7[STRLEN];
  permuteCols(perm7, 0, sudoku6, sudoku7);
  
  int perm8 = rand() % 6;
  char sudoku8[STRLEN];
  permuteCols(perm7, 1, sudoku7, sudoku8);
  
  int perm9 = rand() % 6;
  char sudoku9[STRLEN];
  permuteCols(perm7, 2, sudoku8, sudoku9);
  
  int perm = rand() % 362880;
  relabel(perm, sudoku9, dst);

  dst[N * N] = '\n';
}

//******************************************************************************

int main(int ac, char *av[])
{
  srand(time(NULL));

  if (!av[1])
    return 1;
  int m = atoi(av[1]);
  if (!m)
    return 2;
  if (m > M)
    return 3;

  for (;;)
  {
    char sudoku[STRLEN];
    int n = fread(sudoku, 82, 1, stdin);
    if (n != 1)
      break;

    char permuted[M][STRLEN];
    for (int i = 0; i < m; i++)
    {
    REDO:
      permute(sudoku, &permuted[i][0]);
      permuted[i][N * N] = '\n';

      for (int j = 0; j < i; j++)
	if (!strncmp(permuted[i], permuted[j], N * N))
	  goto REDO;
    }

    fwrite(permuted, STRLEN, m, stdout);
  }

  return 0;
}

//******************************************************************************
