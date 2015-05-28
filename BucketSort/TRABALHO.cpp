//#include <pthread.h>
#include <mpi.h>
#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <vector>
using namespace std;

int tamvet, nbuckets, nprocs;
//pthread_mutex_t mutex;
int * vetorPrincipal;
int ** buckets;  // Vetor de buckets (vetores)
int * tamanhoBucket;
//pthread_mutex_t mutex;

void erroLT0() {  // Less Than 0
	cout << "ERRO: VALOR DEVE SER UM INTEIRO MAIOR DO QUE 0\n"; 
	exit(1);
}

void verificaDados(int argc, char *argv[]) {
	if(argcounter != 3 || atoi(argv[1]) < 1 || atoi(argv[2]) < 1 || atoi(argv[2]) > atoi(argv[1])) {
		cout << "Parametros invalidos. Considere inserir o comando na forma:\n\n      $ mpirun -np NUMERO_DE_THREADS ";
		cout << argv[0] << " TAMANHO_DO_VETOR NUMERO_DE_BUCKETS\n\n";
		cout << "O programa será finalizado."
		exit(0);
	}
}	

void geraVetorAleatorio() {
	vetorPrincipal = new int[tamvet];
	int k;
	srand (time(NULL));
	for (k=0; k<tamvet; k++) {
		vetorPrincipal[k] = rand() % tamvet;
	}		
}

void imprimeVetor() {
	int k;
	for (k=0; k<tamvet; k++) {
		cout << vetorPrincipal[k] << ' ';
	}
	cout << "\n";
}

void separaVetorParaBuckets() {
	buckets = new int*[nbuckets];
	tamanhoBucket = new int[nbuckets];
	int bucketAtual = 0, valorInicialAtual = 0; 
	for (; bucketAtual < nbuckets; bucketAtual++) {
		int tamanhoFaixaAtual = (tamvet - valorInicialAtual) / (nbuckets - bucketAtual);
		if (((tamvet - valorInicialAtual) % (nbuckets - bucketAtual)) != 0)
			tamanhoFaixaAtual ++;
		tamanhoBucket[bucketAtual] = 0;
		int k;
/*		// Loop para ver quantos elementos estao na faixa do bucket autal:
		for(k=0; k<tamvet; k++) {
			// Se o valor esta na faixa atual:
			int valorAtual = vetorPrincipal[k];
			if (valorInicialAtual <= valorAtual && valorAtual < (valorInicialAtual + tamanhoFaixaAtual))
				tamanhoBucket[bucketAtual] ++;  vai pro outro loop
		} 
*/		buckets[bucketAtual] = new int[tamvet];
		// Loop para adicionar elementos no bucket atual:
		int i = 0;
		for (k=0; k<tamvet; k++) {
			int valorAtual = vetorPrincipal[k];
			if (valorInicialAtual <= valorAtual && valorAtual < (valorInicialAtual + tamanhoFaixaAtual)) {
				buckets[bucketAtual][i] = valorAtual;
				i++;
				tamanhoBucket[bucketAtual] ++;
			}
		}
		valorInicialAtual += tamanhoFaixaAtual;
/*		// IMPRIME PARA TESTE:
			for (i=0; i<tamanhoBucket[bucketAtual]; i++)
				cout << buckets[bucketAtual][i] << ' ';
			cout << "tamanhoFaixaAtual = " << tamanhoFaixaAtual << '\n';
		//FIM teste */
	}
}

void bubbleSort(int bucketAtual) {
	int * v;
	v = buckets[bucketAtual];
	int tam = tamanhoBucket[bucketAtual];
	int i, j, temp, trocou;
	for(j = 0; j < tam-1; j++) {
		trocou = 0;
		for(i = 0; i < tam-1; i++) {
			if(v[i+1] < v[i]) {
				temp = v[i];
				v[i] = v[i+1];
				v[i+1] = temp;
				trocou = 1;
			}
		} 
		if(!trocou)
			break;
	}
}

int proximoBucket;

void *threadFunction(void *idThread) {
	int id = *((int *) idThread);
	while (true) {
		pthread_mutex_lock(&mutex);
		 if (proximoBucket == nbuckets) {
			 pthread_mutex_unlock(&mutex);
			 break;  // Sai do loop quando nao há mais buckets
		 }
		 int bucketAtual = proximoBucket;
		 proximoBucket++;
		pthread_mutex_unlock(&mutex);
		printf("Thread %d processando bucket %d\n", id, bucketAtual);
		bubbleSort(bucketAtual);
	}
	pthread_exit(NULL);
}

void executaMestre(int nEscravos) {
	int j, k;
	int executar = 1;
	for (j = nbuckets; j>=0; j--)
		for (k = 1; k = nEscravos+1; k++) {
			MPI_Send MENSADEM cOM EXECUTAR
			MPI_Send(&buckets[k-1], tamvet, MPI_INT, k, 0, MPI_COMM_WORLD);  // envia bucket
			MPI_Send(&tamanhoBucket[k-1], 1, MPI_INT, k, 0, MPI_COMM_WORLD);  // envia tamanho do bucket
		}
		for (k = 1; k = nEscravos+1; k++) {
			MPI_Receive bucket ordenado
		}
	}
	executar = 0;
	ENVIAR EXECUTAR PARA TODAS AS THREADS
}

int main(int argc, char *argc[]) {
	//!!!!!!!!!!!!!!!!!!!!!!DEVE RECEBER PARAMETROS POR LINHA DE COMANDO!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// aqui deve pegar o valor inserido no comando
	verificaDados();  //trocar por verifica
	geraVetorAleatorio();
	cout << "Vetor inicial:\n";
	imprimeVetor();
	separaVetorParaBuckets();
	// INICIA MPI:
	int rank;
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &nprocs);  //	nprocs recebe o numero de processos criados (incluindo o main)
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);  // rank do processo
	int nEscravos = nprocs - 1;
	if (rank == 0)
		executaMestre(nEscravos);
	else 
		executaEscravo();


/*	int k, id[nprocs];
	for (k=0; k<nprocs; k++) {
		id[k] = k;
		pthread_create(&threads[k], NULL, threadFunction, (void *)&id[k]);
	}
	// ESPERA THREADS:
	for (k=0; k<nprocs; k++) {
		pthread_join(threads[k], NULL);
	}
	pthread_mutex_destroy(&mutex);
	// ESCREVE BUCKETS NO VETOR PRINCIPAL:
	int i, j;  
	k = 0;  // k = Posição Atual
	for (i=0; i<nbuckets; i++) {
		for(j=0; j<tamanhoBucket[i]; j++) {
			vetorPrincipal[k] = buckets[i][j];
			k++;
		}
	}
	*/
	
	MPI_Finalize();
	imprimeVetor();
	return 0;
};
