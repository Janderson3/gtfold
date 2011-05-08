#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>

#include "global.h"
#include "options.h"
#include "constraints.h"

int* BP;
int* ind;

/*
ZS: Explanation to BP array. 

BP(i,j) for i!=j can have one of the following values: 
0: Nothing is required about a pair i,j in constraints
1: Force pairing between i and j. This implies also that
   any non-nested pairings will be prohibited and any 
   other pairs i and j would be involved in are also prohibited. 
2: Prohibit pairing between i and j. (Nothing else is done)

BP(i,i) can have one of the following values: 
0: Nothing is known about position i at all
3: Position i is forced to be single-stranded. This implies also that
   any pairs with i will be prohibited. 
4: Position i is NOT single-stranded, it is forced to be in a pair. 
5: Position i is prohibited from pairing with at least one nucleotide. 
*/

int** PBP;
int** FBP;

int nPBP;
int nFBP;

bool compare_bp(const std::pair<int,int>& o1, 
			   	const std::pair<int,int>& o2) {
	return o1.first < o2.first;
}


static int load_constraints(const char* constr_file, int verbose=0) {
	

	fprintf(stdout, "- Running with constraints\n");


	std::ifstream cfcons;
	cfcons.open(constr_file, std::ios::in);
    if (cfcons == 0) {
        fprintf(stderr, "Error opening constraint file\n\n");
        cfcons.close();
        return -1;
    }

    char cons[100];

    while (!cfcons.eof()) {
        cfcons.getline(cons, 100);
        if (cons[0] == 'F' || cons[0] == 'f') nFBP++;
        if (cons[0] == 'P' || cons[0] == 'p') nPBP++;
    }
    cfcons.close();

    //fprintf(stdout, "Number of Constraints given: %d\n\n", nFBP + nPBP);
    if (nFBP + nPBP == 0) {
        fprintf(stderr, "No Constraints found.\n\n");
        return -1;
    }

    FBP = (int**) malloc(nFBP*sizeof(int*));
    PBP = (int**) malloc(nPBP*sizeof(int*));

    int fit = 0, pit = 0, it = 0;

    for (it = 0; it < nFBP; it++) {
        FBP[it] = (int*) malloc(2* sizeof (int));
    }
    for(it=0; it < nPBP; it++) {
        PBP[it] = (int*)malloc(2*sizeof(int));
    }
    cfcons.open(constr_file, std::ios::in);

    while(!cfcons.eof()) {
        cfcons.getline(cons,100);
        char *p=strtok(cons, " ");
        p = strtok(0, " ");
        if(cons[0]=='F' || cons[0]=='f') {
            int fit1=0;
            while(p!=0) {
                FBP[fit][fit1++] = atoi(p);
                p = strtok(0, " ");
            }
            fit++;
        }
        if(cons[0]=='P' || cons[0]=='p') {
            int pit1=0;
            while(p!=0) {
                PBP[pit][pit1++] = atoi(p);
                p = strtok(0, " ");
            }
            pit++;
        }
    }

	std::vector<std::pair<int,int> > v_fbp;
	for(it=0; it< nFBP; it++) {
		for(int k=1;k<= FBP[it][2];k++)
			v_fbp.push_back(std::pair<int,int>(FBP[it][0]+k-1, FBP[it][1]-k+1));
	}
	
	if(v_fbp.size()>1){
		std::sort(v_fbp.begin(), v_fbp.end(), compare_bp);
		for (size_t ii = 0; ii < v_fbp.size() -1 ; ++ii) {
			if (v_fbp[ii].second!=0&&v_fbp[ii].second <= v_fbp[ii+1].second
				&& v_fbp[ii].second >= v_fbp[ii+1].first) {
				fprintf(stderr, "\nConstraints create pseudoknots, exiting !!!\n");
				exit(-1);
			}
			
		}
	}

  
    return 0;
}

int init_constraints(const char* constr_file,int length) {
	load_constraints(constr_file);


	int i,j,it,k;
	ind = (int*) malloc((length+1) * sizeof(int));
	if (ind == NULL) {
		perror("Cannot allocate variable 'ind'");
		exit(-1);
	}
	for(i = 1; i<=length; i++){
		ind[i] = (i*(i-1)) >>  1; //n(n-1)/2
	}


	BP = (int*) malloc((((length+1)*(length))/2+1)*sizeof(int));
	    if (BP == NULL) {
        	perror("Cannot allocate variable 'constraints'");
	        exit(-1);
	    }
	
	int LLL = length*(length+1)/2 + 1;


	//ZS: initialize all basepairing constraints to 0 (default is nothing known)
	for(i = 0; i<LLL; i++){
		BP[i] = 0;
	}

	
	//ZS: for noncanonical bases (right now this only handles 'N'), force single-stranded. 
	for(i = 1; i <= length; i++){
		if(RNA[i]=='N'){
			//force single-stranded
			BP(i,i) = 3;
			//Prohibit pairing with anything else
			for(j = i+1; j<=length; j++){
				BP(i,j) = 2;
			}
		}
	}

	
	//ZS: set prohibited basepairs
	if(nPBP != 0){
		for(it = 0; it < nPBP; it++){
			if(PBP[it][2] < 1 || PBP[it][1] == 0){
				printf("Invalid entry (P: %d %d %d)\n", PBP[it][0], PBP[it][1], PBP[it][2]);
				continue;
			}
		
			for(k = 1; k <= PBP[it][2]; k++){
				
				BP(PBP[it][0]+k-1, PBP[it][1]-(k-1)) = 2;

				//Mark that these two nucleotides are involved in a prohibited pair (only used for for printing out)
				BP(PBP[it][0]+k-1,PBP[it][0]+k-1) = 5;
				BP(PBP[it][1]-(k-1),PBP[it][1]-(k-1)) = 5;
			}
		}	
	}

	//ZS: set forced basepairs and single-stranded nucleotides

	if(nFBP != 0){
		for(it = 0; it<nFBP; it++){
			if(FBP[it][2] < 1){
				printf("Invalid entry (F: %d %d %d)\n", FBP[it][0], FBP[it][1], FBP[it][2]);
				continue;
			}


			for(k = 1; k<=FBP[it][2]; k++){
				int i1 = FBP[it][0]+k-1;
				int j1 = FBP[it][1]-k+1;
				if(FBP[it][1]!=0&&!canPair(RNA[FBP[it][0]+k-1], RNA[FBP[it][1]-k+1])){
					printf("Can't force (%d, %d) to pair (non-canonical) \n", FBP[it][0]+k-1, FBP[it][1]-k+1);
					continue;			
				}
				if(FBP[it][1]!=0&&(j1-i1 < TURN)){
					printf("Can't force (%d, %d) to pair (turn too tight) \n", FBP[it][0]+k-1, FBP[it][1]-k+1);
					continue;
				}
				if(FBP[it][1] == 0){
					//force single-stranded
					BP(FBP[it][0]+k-1, FBP[it][0]+k-1) = 3;
					//prohibit all pairs with that base
					for(i = 1; i<=length; i++){
						if(i<FBP[it][0]+k-1){
							BP(i, FBP[it][0]+k-1) = 2;
						}
						if(FBP[it][0]+k-1<i){
							BP(FBP[it][0]+k-1, i) = 2;
						}
					}
				}
				else{
					//force pairing
					BP(FBP[it][0]+k-1, FBP[it][1]-(k-1)) = 1;
					//prohibit all pairs not-nested with respect to this one 
					//(including the ones which include either of the bases)
					for(i = 1; i<FBP[it][0]+k-1; i++){
						for(j = FBP[it][0]+k-1; j<=FBP[it][1]-(k-1); j++){
							BP(i,j)=2;
						}
					}
					for(i = FBP[it][0]+k-1; i<=FBP[it][1]-(k-1); i++){
						for(j = FBP[it][1]-(k-1)+1; j<=length; j++){
							BP(i,j)=2;
						}
					}
					//prohibit all remaining pairs with the pairing bases 
					//(inside enclosed region)
					for(i = FBP[it][0]+k; i<FBP[it][1]-(k-1); i++){
						BP(FBP[it][0]+k-1,i) = 2;
						BP(i,FBP[it][1]-(k-1)) = 2;
					}
					

					//mark that these two nucleotides are involved in a constrained pair
					//to avoid searching in O(N^2) time
					BP(FBP[it][0]+k-1, FBP[it][0]+k-1) = 4;
					BP(FBP[it][1]-(k-1), FBP[it][1]-(k-1))=4;
					//force pairing 
					BP(FBP[it][0]+k-1, FBP[it][1]-(k-1)) = 1;
					
				}
			}
		}
	}

	return 0;
}

void free_constraints(int len) {
	free(BP);
}

void print_constraints(int len) {

    int i = 1;
    int j = 1;
/*

    printf("Printing constraints \n");


    for(j = 1; j<=len; j++){
	for(i = 1; i<=j; i++){
		printf("BP(%d,%d)=%d\t", i,j,BP(i,j));
	}
	printf("\n\n");
    }

*/
    for (i = 1; i <= len; i++) {
	    switch(BP(i,i)){
		case 3:
			//printf("%d: .\n", i);break;
			printf("x");break;
		case 4:
			//printf("%d - ", i);
			for(j = 1;j<=len;j++){ 
				if(i<j&&BP(i,j)==1){
					//printf("%d",j);
					printf("(");
				}
				else if(j<i&&BP(j,i)==1){
					//printf("%d",j);
					printf(")");
				}
			}
			//printf("\n");
			break;
		case 5: 
			//printf("%d P \n", i); break;
			printf("P");break;
		case 0:
			//printf("%d not constrained\n",i);
			printf(".");
			break;
		default:
			printf("ERROR in constraint value, debugging info: i=%d, BP(i,i)=%d",i,BP(i,i));break;
	    }  
    }
    printf("\n");
 
}
/* Original Prashant's code commented out
int is_ss(int i, int j) {

	if (CONS_ENABLED) {
		int it;
		for (it = i + 1; it < j; it++) {
			if (BP[it] > 0) return 1;
		}
		return 0;
	}
	else
		return 0;

return 0;
}


int prohibit_base(int i) {
//	return (BP[i] == -1);
return 0;
}

*/
//int check_base(int i) {

//	if (CONS_ENABLED) 
//		return (BP[i] <= 0);
//	else
//		return 1;

//return 1;
//}

/*int force_pair(int i, int j) {

		return (BP[i] > 0 && j != BP[i]);

return 0;
}*/

int force_pair1(int i, int j) {
//ZS: this function returns true if a pair between i and j is forced.

	if (CONS_ENABLED) 
		return BP(i,j)==1;
	else
		return 0;
}

int force_ss1(int i){
//ZS: this function returns true if a base is forced to be single-stranded 

	if(CONS_ENABLED)
		return BP(i,i)==3;
	else 
		return 0;
}

int force_ssregion1(int i, int j){
	if(CONS_ENABLED){
		for(int p = i; p<j; p++){
			if(BP(p,p)==3){return 1;}
		}
		return 0;
	}
	else return 0;
}

int check_iloop(int i, int j, int p, int q) {
//ZS: This function returns 1 if internal loop with pairs i,j and p,q is not allowed.
	if (CONS_ENABLED){
		//Need to check that i,j and p,q pairs are allowed, 
		//and single-stranded regions between i,p and q,j are allowed
		return check_pair(i,j)&&check_pair(p,q)&&check_ssregion(i,p)&&check_ssregion(q,j);
	}
		//Original code (Prashant's):
		//return is_ss(i,p) || is_ss(q,j);
	else 
		return 0;
}

int check_pair(int i, int j) {
//ZS: This function returns 1 if i and j are not allowed
//to pair according to the constraints. 
	if (CONS_ENABLED){ 
		//can't pair if i,j is prohibited or i or j are forced to be single-stranded
		if(BP(i,j)==2||BP(i,i)==3||BP(j,j)==3) return 1;
		//can't pair if i or j are forced to pair with something other than each other
		if((BP(i,i)==4||BP(j,j)==4)&&BP(i,j)!=1) return 1;	
		else return 0;
		//Original code (Prashant's):
		//return prohibit_base(i) || prohibit_base(j) || force_pair(i,j) || force_pair(j,i);
	}
	else return 0;
}

int check_ssregion(int i, int j){
//ZS: This function returns 1 if any nucleotide between i and j is forced to pair with something
//(i and j are NOT included in the check)
	for(int p = i+1; p<j; p++){
		if(BP(p,p)==4) return 1;
	}
	return 0;
}

int can_dangle(int i){
	if (CONS_ENABLED){
		return BP(i,i)!=4;
	}
	else{
		return 1;
	}
}

int check_stack(int i, int j) {
	if (CONS_ENABLED){
		//Just need to check if pair is allowed
		return check_pair(i,j);
		//Original code (Prashant's)
		//return force_pair(i,j) || force_pair(j,i);
	}
	else return 0;
}

int check_hairpin(int i, int j) {

	//ZS: According to algorithms.c, this should return 
	//1 when a hairpin with i,j as closing pair isn't allowed.
	if (CONS_ENABLED){
		//Need to check if pair is allowed and if anything pairs between them
		return check_pair(i,j)&&check_ssregion(i,j);
		//Original code (Prashant's)
		//return is_ss(i,j) || force_pair(i,j) || force_pair(j,i);
	}	
	else return 0;
}

int withinCD(int i, int j){
	if (LIMIT_DISTANCE){	
		return j-i+1>contactDistance;
	}
	else return 1;
}