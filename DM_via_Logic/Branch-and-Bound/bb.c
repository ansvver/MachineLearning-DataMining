#include <stdio.h>
#define BUFFER_SIZE 100
#define POS_EXAM_FILE "positive_examples.txt"
#define NEG_EXAM_FILE "negative_examples.txt"
#define OK 1
#define ERROR 0
#define TRUE 1
#define FALSE 0

typedef int Binary;
typedef int Status;
typedef int Flag;

// negative examples set, to storage the negative example ID
typedef struct {
	int *neg;
	int len;
}NEG;

typedef struct {
	int *path;	// the path of the solution, 0 for 'A1' , 1 for '^A1', 2 for 'A2', 3 for '^A2'...
	int path_len;
	int *neg;	// the current negative examples set
	int neg_len;
}QElemType;

typedef struct {
	QElemType e;
	Flag f;		// to mark down if the branch node needs to be cut. If TRUE, continue to expand it, while if FALSE, no need to expand it anymore
}SetCompBuff;

typedef struct {
	QElemType data;
	struct QNode *next;
}QNode, * QueuePtr;

typedef struct {
	QueuePtr front;
	QueuePtr rear;
}LinkQueue;

int dim; 			//the number of the dimensions
int pos_num; 		//the number of the positive examples
int neg_num; 		//the number of the negative examples
Binary **pos_set;	//the positive examples set
Binary **neg_set;	//the negative examples set
NEG **neg_func_set;	//the negative function set
LinkQueue EXPQ;		//store each optimal solution attained in each round
QElemType optimal;	//store the current optiaml solution

//Print the error message and exit
void exitERROR(char msg[]);

//Initialize queue
Status InitQueue(LinkQueue *Q);

//Add an element to queue
Status EnQueue(LinkQueue *Q, QElemType e);

//Delete an element of the queue
Status DeQueue(LinkQueue *Q, QElemType *e);

//Check if the queue is empty
Status QueueEmpty(LinkQueue Q);

//Destory queue
Status DestoryQueue(LinkQueue *Q);

//Initialize the primary examples set from the specified file
void initExamplesSet();

//Generate the Negative Function Set according the positive and negative examples set
void initNegFuncSet();

//Print the example set
void printExampleSet();

//Print the Negative Function Set
void printNegFuncSet();

//Print an queue element
//void print(QElemType e);

//Print the elements in the Set Comparision Buffer
//void printSCB(SetCompBuff scb);

//Compare the sets in the Set Comparision to estimate if the branch node can be cut
void setComp(SetCompBuff *scb);

//Combine the current element with the explansion information
void conSet(QElemType qe, NEG neg, int pathNode, QElemType *e);

//Branch-and-Bround method to figure out the optimal solution
void BB();

//Amend the negative examples
void changeNegExamples();

//Free the space
void clear();

//Print the final expression
void printResult();

//Print the current optimal solution
void printOptimal();

void exitERROR(char msg[]) {
	printf("\nERROR: %s\n", msg);
	system("pause");
	exit(1);	
}

/******************************\
|* the basic queue operations *|
\******************************/
Status InitQueue(LinkQueue *Q) {
	Q->front = Q->rear = (QueuePtr)malloc(sizeof(QNode));
	if(!Q->front) exitERROR("Out of memory!");
	Q->front->next = NULL;
	return OK;
}

Status EnQueue(LinkQueue *Q, QElemType e) {
	QueuePtr p = (QueuePtr)malloc(sizeof(QNode));
	if(!p) exitERROR("Out of memory!");
	p->data = e;
	p->next = NULL;
	Q->rear->next = p;
	Q->rear = p;
	return OK;
}

Status DeQueue(LinkQueue *Q, QElemType *e) {
	if(Q->front == Q->rear) return ERROR;
	QueuePtr p;
	p = Q->front->next;
	*e = p->data;
	Q->front->next = p->next;
	if(Q->rear == p) Q->rear = Q->front;
	free(p);
	return OK;
}

Status QueueEmpty(LinkQueue Q) {
	if(Q.rear == Q.front)
		return TRUE;
	else
		return FALSE;
} 

Status DestoryQueue(LinkQueue *Q) {
	QNode *p = Q->front;
	while(!p) {
		Q->front = Q->front->next;
		free(p);
		p = Q->front;
	}
	Q->rear = NULL;
	return OK;
}




void initExamplesSet() {
	FILE *fp;
	char buffer[ BUFFER_SIZE ];
	char ch;
	int i, j, index, check;
	
	/********************************\
	| init the positive examples set |
	\********************************/
	if((fp=fopen(POS_EXAM_FILE, "rt"))==NULL) 
		exitERROR("error on reading positive examples file!");

	fgets(buffer, BUFFER_SIZE, fp);
	dim = 0;
	for( i = 0, ch = buffer[i]; ch != '\n' && ch != '#'; ch = buffer[++i]) {
		if(ch != ' ') dim++;
	}
	check = dim;
	pos_num = 1;
	while( fgets(buffer, BUFFER_SIZE, fp) ) {
		pos_num++;
	}
	fclose(fp);
	
	fp=fopen(POS_EXAM_FILE, "rt");
	pos_set = (Binary **)malloc(pos_num * sizeof(Binary *));
	if(pos_set == NULL) 
		exitERROR("not enough room for pos_set!");	
		
	for(j=0; fgets(buffer, BUFFER_SIZE, fp); ++j) {
		pos_set[j] = (Binary *)malloc(dim * sizeof(Binary));
		if(pos_set[j] == NULL) 
			exitERROR("not enough room for pos_set!");	

		for( i=0, ch=buffer[i], index=0; ch != '\n' && ch != '#'; ch = buffer[++i]) {
			if(ch != ' ') 
				pos_set[j][index++] = ch-48;
		}
		if(ch == '#') break;
	}
	fclose(fp);
	
	/********************************\
	| init the negative examples set |
	\********************************/
	if((fp=fopen(NEG_EXAM_FILE, "rt"))==NULL) 
		exitERROR("error on reading negative examples file!");
	fgets(buffer, BUFFER_SIZE, fp);
	dim = 0;
	for( i = 0, ch = buffer[i]; ch != '\n' && ch != '#'; ch = buffer[++i]) {
		if(ch != ' ') dim++;
	}
	if(check != dim)
		exitERROR("the dimension of negative example differs from positive example!");
	neg_num = 1;
	while( fgets(buffer, BUFFER_SIZE, fp) ) {
		neg_num++;
	}
	fclose(fp);

	fp=fopen(NEG_EXAM_FILE, "rt");
	neg_set = (Binary **)malloc(neg_num * sizeof(Binary *));
	if(neg_set == NULL)
		exitERROR("not enough room for neg_set!");
	for(j=0; fgets(buffer, BUFFER_SIZE, fp); ++j) {
		neg_set[j] = (Binary *)malloc(dim * sizeof(Binary));
		if(neg_set[j] == NULL) 
			exitERROR("not enough room for pos_set!");
			
		for( i=0, ch=buffer[i], index=0; ch != '\n' && ch != '#'; ch = buffer[++i]) {
			if(ch != ' ')  
				neg_set[j][index++] = ch-48;
		}
		if(ch == '#') break;
	}
	fclose(fp);
}

void initNegFuncSet() {
	int i, j;
	neg_func_set = (NEG **)malloc(dim * sizeof(NEG*));
	if(neg_func_set == NULL)
		exitERROR("not enough room for negative function set!");
	for(i=0; i<dim; ++i) {
		
		//Use neg_func_set[i][0] to denote NEG(^Ai)
		//Use neg_func_set[i][1] to denote NEG(Ai)
		neg_func_set[i] = (NEG *)malloc(2 * sizeof(NEG));
		if(!neg_func_set[i])
			exitERROR("not enough room for negative function set!");
		NEG *neg0 = &neg_func_set[i][0];
		NEG *neg1 = &neg_func_set[i][1];
		neg0->neg = (int *)malloc(neg_num * sizeof(int));
		neg1->neg = (int *)malloc(neg_num * sizeof(int));
		if(!neg0->neg || !neg1->neg)
			exitERROR("Out of memory!");
		neg0->len = 0;
		neg1->len = 0;

		for(j=0; j<neg_num; ++j) {
			if(neg_set[j][i] == 0) {
				neg0->neg[neg0->len] = j;
				neg0->len++;
			}else if(neg_set[j][i] == 1) {
				neg1->neg[neg1->len] = j;
				neg1->len++;
			}
		}
	}
}


void printExampleSet() {
	int i, j;
	printf("Positive examples set:\n");
	for(i=0; i<pos_num; i++) {
		for(j=0; j<dim; j++)
			printf("%d ", pos_set[i][j]);
		printf("\n");
	}

	printf("\nNegative examples set:\n");
	for(i=0; i<neg_num; i++) {
		for(j=0; j<dim; j++)
			printf("%d ", neg_set[i][j]);
		printf("\n");
	}
	printf("\n");
}

void printNegFuncSet() {
	int i, j, k;
	printf("NEG() Set:\n");
	for(i=0; i<dim; i++) {
		for(j=0; j<2; j++) {
			if(j == 0) 
				printf("NEG(^A%d) = {", i + 1);
			else
				printf("NEG(A%d) = {", i + 1);
			for(k=0; k<(neg_func_set[i][j].len)-1; k++)
				printf("%d, ", neg_func_set[i][j].neg[k] + 1);
			if(neg_func_set[i][j].len == 0 )
				printf("}\n");
			else
				printf("%d}\n", neg_func_set[i][j].neg[k] + 1);
		}
	}
	printf("\n");
}

/*
void print(QElemType e) {
	int i;
	printf("**e.neg_len=%d; e.neg=", e.neg_len);
	for(i=0; i<e.neg_len; i++) {
		printf("%d", e.neg[i]);
	}
	printf("; e.path_len=%d; e.path=", e.path_len);
	for(i=0; i<e.path_len; i++) {
		printf("%d", e.path[i]);
	}
	printf("; **");
	system("pause");
}

void printSCB(SetCompBuff scb) {
	int i;
	printf("**scb.e.neg_len=%d; scb.e.neg=", scb.e.neg_len);
	for(i=0; i<scb.e.neg_len; i++) {
		printf("%d", scb.e.neg[i]);
	}
	printf("; scb.e.path_len=%d; scb.e.path=", scb.e.path_len);
	for(i=0; i<scb.e.path_len; i++) {
		printf("%d", scb.e.path[i]);
	}
	printf("; scb.e.f=%d; **", scb.f);
	system("pause");
}
*/

void setComp(SetCompBuff *scb) {
	int i, j, k, negID_a, negID_b, negID_index_a, negID_index_b, negID_index;
	for(i=0; i<dim; i++) {
		if(scb[i].f == FALSE) continue;
		for(j=i+1; j<dim; j++) {
			if(scb[j].f == FALSE) continue;
			if(scb[i].e.neg_len < scb[j].e.neg_len) {
				for(negID_index_a=0, negID_index_b=0; negID_index_b<scb[j].e.neg_len && negID_index_a<scb[i].e.neg_len;) {
					negID_a = scb[i].e.neg[negID_index_a];
					negID_b = scb[j].e.neg[negID_index_b];
					if(negID_a < negID_b) {
						break;
					} else if(negID_a > negID_b) {
						++negID_index_b;
					} else if(negID_a == negID_b) {
						++negID_index_a;
						++negID_index_b;
					}
				}
				//if all elments in the smaller set are explored then make the flag of the bigger one FALSE 
				if(negID_index_a == scb[i].e.neg_len) {
					scb[j].f = FALSE;
				}
			} else if(scb[i].e.neg_len > scb[j].e.neg_len) {
				for(negID_index_a=0, negID_index_b=0; negID_index_a<scb[i].e.neg_len && negID_index_b<scb[j].e.neg_len;) {
					negID_a = scb[i].e.neg[negID_index_a];
					negID_b = scb[j].e.neg[negID_index_b];
					if(negID_b < negID_a) {
						break;
					} else if(negID_b > negID_a) {
						++negID_index_a;
					} else if(negID_a == negID_b) {
						++negID_index_a;
						++negID_index_b;
					}
				}
				//if all elments in the smaller set are explored then make the flag of the bigger one FALSE 
				if(negID_index_b == scb[j].e.neg_len) {
					scb[i].f = FALSE;
				}
			} else {
				for(negID_index=0; negID_index<scb[i].e.neg_len;) {
					negID_a = scb[i].e.neg[negID_index];
					negID_b = scb[j].e.neg[negID_index];
					if(negID_b != negID_a) {
						break;
					} else {
						negID_index++;
					}
				}
				//if two sets are the same, then the latter one is signed by FALSE;
				if(negID_index == scb[i].e.neg_len) {
					scb[j].f = FALSE;
				}
			}
			 
		}
	}
}

void conSet(QElemType qe, NEG neg, int pathNode, QElemType *e) {
	int i = 0, j = 0;
	int index = 0;
	Flag flag;		//to mark if the pathNode is placed in qe
	e->neg_len = 0;
	e->path_len = 0;

	e->neg = (int *)malloc(neg_num * sizeof(int));
	e->path = (int *)malloc(pos_num * sizeof(int));
	if(!e->neg || !e->path)
		exitERROR("out of memory!");

	//combine the two negative sets
	while(i<qe.neg_len && j < neg.len) {
		if(qe.neg[i] < neg.neg[j]) {
			e->neg[index] = qe.neg[i];
			i++;
		} else if(qe.neg[i] > neg.neg[j]) {
			e->neg[index] = neg.neg[j];
			j++;
		} else {
			e->neg[index] = qe.neg[i];
			i++;
			j++;
		}
		index ++;
	}
	if(i < qe.neg_len) {
		while( i<qe.neg_len ){
			e->neg[index] = qe.neg[i];
			index++;
			i++;
		}
	} else {
		while( j<neg.len ) {
			e->neg[index] = neg.neg[j];
			index ++;
			j++;
		}
	}
	e->neg_len = index;
	
	//combine the path with the current path node
	flag = FALSE;
	for(i=0, index=0; i<qe.path_len;) { 
		if(qe.path[i] < pathNode) {
			e->path[index] = qe.path[i];
			i++;
			index++;
		}else if(qe.path[i] == pathNode) {
			flag = TRUE;
			e->path[index] = qe.path[i];
			i++;
			index++;
		} 
		else {
			if(!flag) { 
				e->path[index] = pathNode;
				index++;
			}
			for(j=i; j<qe.path_len;) {
				e->path[index] = qe.path[j];
				j++;
				index++;
			}
			flag = TRUE;
			break;
		}
	}

	if(!flag) {
		e->path[index] = pathNode;
		index++;
	}
	e->path_len = index;
	
}

//Use Data Structure Queue to implement Branch-and-Bround by FIFO method
void BB() {
	int i, j, k, l, m, n;
	int count, next_count;
	LinkQueue Q;
	InitQueue(&Q);
	QElemType e;
	SetCompBuff *scb;

	
	// init the Queue with the first positive example
	for(i=0; i<dim; ++i) {
		if(pos_set[0][i] == 1) {
			e.neg_len = neg_func_set[i][1].len;
			e.neg = (int *)malloc(neg_num * sizeof(int));
			if(!e.neg)
				exitERROR("out of memory!");
			for(j=0; j<e.neg_len; j++) {
				e.neg[j] = neg_func_set[i][1].neg[j];
			}
			e.path = (int *)malloc(pos_num * sizeof(int));
			if(!e.path)
				exitERROR("out of memory!");			
			e.path[0] = 2 * i;
			e.path_len = 1;
		} else {
			e.neg_len = neg_func_set[i][0].len;
			e.neg = (int *)malloc(neg_num * sizeof(int));
			if(!e.neg)
				exitERROR("out of memory!");
			for(j=0; j<e.neg_len; j++) {
				e.neg[j] = neg_func_set[i][0].neg[j];
			}
			e.path = (int *)malloc(pos_num * sizeof(int));
			if(!e.path)
				exitERROR("out of memory!");
			e.path[0] = (2 * i) + 1;
			e.path_len = 1;
		}
		EnQueue(&Q, e);
	}

	count = 1;
	next_count = 0;
	scb = (SetCompBuff*)malloc(dim * sizeof(SetCompBuff));
	if(!scb)
		exitERROR("out of memory!");

	for(i=1; i<pos_num; i++) {
		//the value of count notes how many node should be expanded on Level i
		for(j=0; j<count; j++) {
			for(k=0; k<dim; ++k) {
				DeQueue(&Q, &e);
				scb[k].e = e;
				scb[k].f = TRUE;
			}
			//compare the elements in scb with each other
			setComp(scb);
			for(k=0; k<dim; k++) {
				//TRUE donate that the node should be expanded
				if(scb[k].f == TRUE) {
					for(l=0; l<dim; l++) {
						if(pos_set[i][l] == 1) {
							conSet(scb[k].e, neg_func_set[l][1], 2 * l,  &e);
						} else if(pos_set[i][l] == 0) {
							conSet(scb[k].e, neg_func_set[l][0], 2 * l + 1, &e);
						}
						//Add to the queue
						EnQueue(&Q, e);
					
					}
					//note how many nodes should be expanded in the next round
					//for more, this use to note that how many elments should be deleted in the queue in the next round
					next_count++;				
				}
			}
		}

		count = next_count;
		next_count = 0;
	}
	

	//to find out the first element with the least size value of the negative examples set
	DeQueue(&Q, &e);
	optimal = e;
	while(!QueueEmpty(Q)) {
		DeQueue(&Q, &e);
		if(e.neg_len < optimal.neg_len) {
			optimal = e;
		} 
	}
	
	//Add this current optimal solution to the final expression queue
	EnQueue(&EXPQ, optimal);

}

void changeNegExamples() {
	
	if(optimal.neg_len == 0) {
		neg_num = 0;
		return;
	}
	
	int i, j;
	int negID;
	Binary **neg_buff;
	
	//Take the negative examples in the optimal to construct the current negative examples
	neg_buff = (Binary **)malloc(optimal.neg_len * sizeof(Binary *));
	
	for(i=0; i<optimal.neg_len; i++) {
		neg_buff[i] = (Binary *)malloc(dim * sizeof(Binary));
		negID = optimal.neg[i];
		for(j=0; j<dim; j++) {
			neg_buff[i][j] = neg_set[negID][j];
		}
	}

	//free the old negative examples space
	for(i=0; i<neg_num; i++) {
			free(neg_set[i]);
	}

	neg_set = neg_buff;
	neg_num = optimal.neg_len;
	
}

void clear() {
	int i, j, k;
	for(i=0; i<pos_num; i++) {
		free(pos_set[i]);
	}
	for(i=0; i<neg_num; i++) {
		free(neg_set[i]);
	}
	for(i=0; i<dim; i++) {
		for(j=0; j<2; j++) {
			free(neg_func_set[i][j].neg);
		}
	}
	DestoryQueue(&EXPQ);
	free(optimal.neg);
	free(optimal.path);
}


void printResult() {
	int i;
	QElemType e;
	printf("The Final Expression : \n\n\t");
	while(!QueueEmpty(EXPQ)) {
		DeQueue(&EXPQ, &e);
		printf("( ");
		for(i = 0; i<e.path_len - 1; i++) {
			if(e.path[i] % 2 == 0) {
				printf("A%d || ", (e.path[i] / 2) + 1);
			} else {
				printf("^A%d || ", (e.path[i] + 1) / 2);
			}
		}	
		if(e.path[i] % 2 == 0) {
			printf("A%d )", (e.path[i] / 2) + 1);
		} else {
			printf("^A%d )", (e.path[i] + 1) / 2);
		}
		if(!QueueEmpty(EXPQ)) {
			printf(" && ");
		}
	}
	printf("\n\n");
}

void printOptimal() {
	int i;
	printf("The First Optimal Solution:  ( ");
	for(i = 0; i<optimal.path_len - 1; i++) {
			if(optimal.path[i] % 2 == 0) {
				printf("A%d || ", (optimal.path[i] / 2) + 1);
			} else {
				printf("^A%d || ", (optimal.path[i] + 1) / 2);
			}
	}	
	if(optimal.path[i] % 2 == 0) {
		printf("A%d )\n\n", (optimal.path[i] / 2) + 1);
	} else {
		printf("^A%d )\n\n", (optimal.path[i] + 1) / 2);
	}
}

int main() {
	int i;
	QElemType e;
	char choice;

	while(1)
	{
		system("cls");
		printf("\n\tInferring a Boolean Function from Positive and\n");
		printf("\tNegative Examples by Branch-and-Bound Approach\n\n");
		printf("£½£½£½£½£½£½£½£½£½£½£½£½£½£½£½£½£½£½£½£½£½£½£½£½£½£½£½£½£½£½£½£½\n");
		printf("\t1.Print the Examples Set\n");
		printf("\t2.Print the Result\n");
		printf("\t3.Print the Processing Detail\n");
		printf("\t0.Exit\n");
		printf("£½£½£½£½£½£½£½£½£½£½£½£½£½£½£½£½£½£½£½£½£½£½£½£½£½£½£½£½£½£½£½£½\n");
		printf("Please Enter a Number 0 - 3£º");
		choice = getchar();
		if(choice == 10 || choice < '0' || choice > '3') //to check if it's "Enter"
		{
			continue;
		} else {
			switch( choice ) {
				case '0':
					exit(0); 
					break;
				case '1':
					initExamplesSet();
					system("cls");
					printExampleSet(); 
					system("pause");
					break;
				case '2':
					InitQueue(&EXPQ);
					initExamplesSet();
					while(neg_num > 0) {
						initNegFuncSet();
						BB();
						changeNegExamples();
					}
					system("cls");
					printResult();
					system("pause");
					clear();
					break;
				case '3':
					i = 0;
					InitQueue(&EXPQ);
					initExamplesSet();
					system("cls");
					while(neg_num > 0) {
						i++;
						printf("\n**************** ROUND %d ****************\n\n", i);
						printExampleSet();
						initNegFuncSet();
						printNegFuncSet();
						BB();
						changeNegExamples();
						printOptimal();
						if(neg_num >0) {
							system("pause");
						} else {
							printf("****************    END    ***************\n\n");
							printResult();
							system("pause");
						}
					}
					clear();
					break;
				default:
					break;
			}
		}
	}
	return 1;
}
