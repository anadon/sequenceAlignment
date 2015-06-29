/*
 * ruleFinder.c
 * 
 * Copyright 2015 Unknown <anadon@localhost>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 * 
 * 
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include "ruleFinder.hpp"

using namespace std;


typedef struct intPair{
	long long first;
	long long second;
	long long depth;
	long long links;
	intPair *list;
}intPair;


typedef struct intPairHead{
	long long length;
	intPair *list;
}intPairHead;


//Used to tell progress
long long total, iteration;


void exitWithMessage(const char* msg){
	fprintf(stderr, "%s\n", msg);
	fflush(stderr);
	exit(-1);
}


/***********************************************************************
 * Using the "total" and "iterations" global values, this function runs
 * until total == iterations, updating the percent complete message to 
 * the user in 1 second intervals and provides an estimated time to 
 * completion using the following format:
 * 
 * [<PERCENT>%] Estimated time remaining: <SECONDS> seconds
 * ********************************************************************/
void *reportProgress(void *notUsed){
	
	do{
		fprintf(stderr, "\33[2K\r%lld", iteration);
		fflush(stderr);
	}while(total > 0);
	fprintf(stderr, "\n%lld", iteration);
	fflush(stderr);
	
	pthread_exit(NULL);
}


void recursiveIntPairFree(intPair &toFree){
	for(long long i = 0; i < toFree.links; i++){
		recursiveIntPairFree(toFree.list[i]);
		toFree.list[i].list = NULL;
		toFree.list[i].links = 0;
		toFree.list[i].depth = 0;
	}
	free(toFree.list);
	toFree.list = NULL;
}


void sortHeadsAndTails(intPair &point, intPair **heads, 
												long long &headSize, intPair **tails, 
												long long &tailSize){
	/*0 = is headPair, 1  = not headPair (tail), 2 = invalid*/
	int status; 
	
	*heads = (intPair*) malloc(sizeof(intPair) * point.links);
	*tails = (intPair*) malloc(sizeof(intPair) * point.links);
		
	for(long long i = 0; i < point.links; i++){
		status = 0;
		for(long long j = 0; j < point.links; j++){
			if(  point.list[i].first > point.list[j].first ||
					point.list[i].second > point.list[j].second){
				continue; // invalid intPair, discard by not including
			}
			if(  point.list[i].first < point.list[j].first &&
					point.list[i].second < point.list[j].second){
				status = 1;
				break;
			}
		}
		
		if(status == 0){
			(*heads)[headSize++] = point.list[i];
		}else if(status == 1){
			(*tails)[tailSize++] = point.list[i];
		}
	}
	
	*heads = (intPair*) realloc(*heads, sizeof(intPair) * headSize);
	*tails = (intPair*) realloc(*tails, sizeof(intPair) * tailSize);
}


long long findDiag(intPair &point){
	for(long long i = 0; i < point.links; i++)
		if(point.list[i].first == point.first + 1 &&
				point.list[i].second == point.second+ 1)
			return i;
	return -1;
}


void *recursiveFormLinksPthreadEntry(void *input){
	recursiveFormLinks(*((intPair*)input));
	return NULL;
}


/***********************************************************************
 * Given an array of pairs, they need to be ordered such that each in 
 * a given level (head) cannot exist as part of another pair.  This
 * means that each pair that is considered a head if it does not have
 * both of its indexes greater than another pair.  each is then loaded 
 * with kinks which is has smaller indexes in both sequences.  When 
 * applied recursively, a subsequence hierarchy relevant graph is made.
 * ********************************************************************/
void recursiveFormLinks(intPair &point){
//DECLARATIONS//////////////////////////////////////////////////////////
	intPair *heads, *tails, swap;
	long long headSize, tailSize, indexOfDeepestChild, diagIndex;
	//pthread_t *workersTwo;
	
//INITIALIZE////////////////////////////////////////////////////////////
	
	if(point.links <= 0) return;

	indexOfDeepestChild = headSize = tailSize = 0;
	heads = tails = NULL;
	
//SEPERATE HEAP FROM TAIL POINTS////////////////////////////////////////
	//NOTE: This is an oppertunity to multithread, but not an easy one.
	//Also, scan using diagonals are always correct approach
	
	
	diagIndex = findDiag(point);
	if(diagIndex != -1){
		
		printf("diagonal hit\n"); fflush(stdout);
		
		indexOfDeepestChild = diagIndex;
		
		point.list[diagIndex].links = point.links;
		point.list[diagIndex].list = point.list;
		
		recursiveFormLinks(point.list[diagIndex]);
	}else{
		
		printf("no diagonal\n"); fflush(stdout);
		
		sortHeadsAndTails(point, &heads, headSize, &tails, tailSize);
	
		//free(point.list);
		point.list = heads;
		point.links = headSize;
		
		//while((workersTwo = (pthread_t*) malloc(sizeof(pthread_t) * headSize)) == NULL) sleep(1);
	
		for(long long i = 0; i < headSize; i++){
			point.list[i].links = 0;
			point.list[i].depth = 0;
			point.list[i].list = (intPair*) malloc(sizeof(intPair) * tailSize);
			for(long long j = 0; j < tailSize;j++)
				if(point.list[i].first < tails[j].first && 
						point.list[i].second < tails[j].second)
					point.list[i].list[point.list[i].links++] = tails[j];
		
			point.list[i].list = (intPair*) realloc(point.list[i].list, 
																sizeof(intPair) * point.list[i].links);
			//NOTE: Here's where to multithread
		
			/*pthread_create(&workersTwo[i], NULL, 
												recursiveFormLinksPthreadEntry, &point.list[i]);*/
			recursiveFormLinks(point.list[i]);
		}
		free(tails); tails = NULL;
		
		//for(long long i = 0; i < headSize; i++)
			//pthread_join(workersTwo[i], NULL);
		//free(workersTwo);
		
		for(long long i = 1; i < headSize; i++)
			if(point.list[indexOfDeepestChild].depth > point.list[i].depth)
				indexOfDeepestChild = i;
	}
	

//ORDER LONGEST SUB-SUBSTRING///////////////////////////////////////////

			
	if(indexOfDeepestChild != 0){
		swap = point.list[indexOfDeepestChild];
		point.list[indexOfDeepestChild] = point.list[0];
		point.list[0] = swap;
	}

//CLEANUP///////////////////////////////////////////////////////////////
	//for(long long i = 1; i < headSize; i++){
		//recursiveIntPairFree(point.list[i]);
	//}
	//free(heads); heads = NULL;
	
//OPTIMIZE AND PREPARE FOR CALLING LAYER////////////////////////////////
	point.list = (intPair*) realloc(point.list, sizeof(intPair));
	point.links = 1;
	point.depth = point.list[0].depth+1;
	
	//iteration++;
} 


/***********************************************************************
 * 
 * This function takes 2 sequences and returns a malloc'd array of ints
 * which are indexes of characters in each sequence which are the same
 * such that the indexes tell the longest common subsequence in each of
 * the input sequences in complete detail.
 * 
 * ********************************************************************/
intPair** substringMappings(const char *base, const long long baseLen, 
														const char *other, const long long otherLen){
//DECLARATIONS//////////////////////////////////////////////////////////
	intPair **toReturn;
	intPair *list, handle, itr;
	long long listSize, allocSize;
	
//INITALIZATION/////////////////////////////////////////////////////////

	listSize = 0;
	total = baseLen * otherLen;
	
	handle.list = NULL;
	handle.links = 0;
	handle.depth = 0;
	handle.first = -1;
	handle.second = -1;
	
	allocSize = baseLen * otherLen;
	list = (intPair*) malloc(sizeof(intPair) * allocSize);
	if(list == NULL) exitWithMessage("Failed to allocate \"list\"");

//RECORD PAIRINGS///////////////////////////////////////////////////////
//NOTE optimizations may be possible here to ease the next step
	fprintf(stderr, "number of comparisons: %lld\n", baseLen * otherLen);
	fflush(stderr);
	for(long long i = 0; i < baseLen; i++){
		for(long long j = 0; j < otherLen; j++){
			if(base[i] == other[j]){
				list[listSize].first = i;
				list[listSize].second = j;
				list[listSize].list = NULL;
				list[listSize].links = 0;
				list[listSize].depth = 0;
				listSize++;
			}
		}
	}
	list = (intPair*) realloc(list, sizeof(intPair) * listSize);

//SETUP PAIRING GRAPH AND SOLVE/////////////////////////////////////////

	fprintf(stderr, "number of pairs: %lld\n", listSize);
	fflush(stderr);
	handle.list = list;
	handle.links = listSize;
	fprintf(stderr, "substring is %lld characters long\n", handle.depth);
	recursiveFormLinks(handle);
	total = -1;
	fprintf(stderr, "\ngraph setup and solving complete\n");
	fprintf(stderr, "substring is %lld characters long\n", handle.depth);
	fflush(stderr);

//FORMAT RETURN/////////////////////////////////////////////////////////

	itr = (*handle.list);

	toReturn = (intPair**) malloc(sizeof(intPair*) * (1+handle.depth));
	if(toReturn == NULL) exitWithMessage("Failed to allocate "
																												"\"toReturn\"");
	for(long long i = 0; i < handle.depth-1; i++){
		toReturn[i] = (intPair*) malloc(sizeof(intPair));
		if(toReturn[i] == NULL) exitWithMessage("Failed to allocate "
																										"\"toReturn[i]\"");
		toReturn[i][0] = itr;
		itr = (*itr.list);
	}
	toReturn[handle.depth] = NULL;
	//recursiveIntPairFree(handle);
	
	return toReturn;
}


int main(int argc, char **argv){
	
//DECLARATIONS//////////////////////////////////////////////////////////
	
	fprintf(stderr, "Starting substring search\n");
	fflush(stderr);
	
	char *seqOne, *seqTwo;
	FILE *fp;
	long long sizeSeqOne, sizeSeqTwo;
	intPair **check, **itr;
	//pthread_t completionMonitor;
	
//INITIALIZATION////////////////////////////////////////////////////////
	
	fp = fopen(argv[1], "r");
	fseek(fp, 0, SEEK_END);
	sizeSeqOne = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	seqOne = (char*) malloc(sizeof(char) * (sizeSeqOne+1));
	if(seqOne == NULL) exitWithMessage("Failed to allocate \"seqOne\"");
	fread(seqOne, sizeof(char), sizeSeqOne, fp);
	fclose(fp);
	fprintf(stderr, "Read in first sequence\n");
	fflush(stderr);
	
	fp = fopen(argv[2], "r");
	fseek(fp, 0, SEEK_END);
	sizeSeqTwo = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	seqTwo = (char*) malloc(sizeof(char) * (sizeSeqTwo+1));
	if(seqTwo == NULL) exitWithMessage("Failed to allocate \"seqTwo\"");
	fread(seqTwo, sizeof(char), sizeSeqTwo, fp);
	fclose(fp);
	fprintf(stderr, "Read in second sequence\n");
	fflush(stderr);
	
	iteration = 0;
	//pthread_create(&completionMonitor, NULL, reportProgress, NULL);
	
//MAIN LOOP/////////////////////////////////////////////////////////////
	
	check = substringMappings(seqOne, sizeSeqOne, seqTwo, sizeSeqTwo);
	
//PRINT RESULTS/////////////////////////////////////////////////////////
	
	//pthread_join(completionMonitor, NULL);
	
	fprintf(stderr, "Search complete\n");
	fflush(stderr);
	
	itr = check;
	for(long long i = 0; (*itr) != NULL;  i++){
		printf("{%lld, %lld}, ", itr[i]->first, itr[i]->second);
		//free(itr[0]); itr[0] = NULL;
	}
	printf("\n");
	
//CLEANUP///////////////////////////////////////////////////////////////
	
	free(seqOne); seqOne = NULL;
	free(seqTwo); seqTwo = NULL;
	free(check); check = NULL;
	
	return 0;
}

