#ifndef _ENERGY_TABLES_H_
#define _ENERGY_TABLES_H_

#include "data.h"

extern int *V; 
extern int *VV; 
extern int *VV1; 
extern int *W; 
extern int *VBI; 
extern int *VM; 
extern int *WM; 
extern int *WMi; 
extern int *WMi1; 
extern int *WMi2; 
//extern int **VBI; 
//extern int **WM; 
//extern int **V; 
//extern int **VM; 
extern int *indx; 


#define V(i,j) V[indx[j]+i]
#define VM(i,j) VM[indx[j]+i]
#define WM(i,j) WM[indx[j]+i]
#define VBI(i,j) VBI[indx[j]+i]

//#define V(i,j) V[indx[i]+j]
//#define VM(i,j) VM[indx[i]+j]
//#define WM(i,j) WM[indx[i]+j]
//#define VBI(i,j) VBI[indx[i]+j]

//#define V(i,j) V[i][j]
//#define VM(i,j) VM[i][j]
//#define WM(i,j) WM[i][j]
//#define VBI(i,j) VBI[i][j]


#define auPen(i, j) ((( (i)==BASE_U || (j)==BASE_U ) && ( (i)==BASE_A || (i)==BASE_G || (j)==BASE_A || (j)==BASE_G )) ? auend : 0)

#ifdef __cplusplus
extern "C" {
#endif
int Ed3(int i, int j, int k);
int Ed5(int i, int j, int k);
int auPenalty(int i, int j);

#define Ec multConst[1]
#define Eb multConst[2]
#define Ea multConst[0] 

int eS(int i, int j);
int eH(int i, int j);
int eL(int i, int j, int ip, int jp);

void create_tables(int len);
void init_tables(int len);
void free_tables(int len);
#ifdef __cplusplus
}
#endif

#endif
