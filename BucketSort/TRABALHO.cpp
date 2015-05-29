// compilar com 'mpic++'
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

void kill() {
	cout << "\nO programa será finalizado.\n--------------------------------------------------------------------------\n";
	exit(0);
}

void verificaDados(int argc, char *argv[]) {
	// Obs: se nao recebeu número como parametro, atoi retorna 0.
	if (argc != 3 || atoi(argv[2]) > atoi(argv[1])) {
		cout << "Parametros invalidos. Considere inserir o comando na forma:\n\n  $ mpirun -np NUMERO_DE_PROCESSOS ";
		cout << argv[0] << " TAMANHO_VETOR NUMERO_BUCKETS\n";
		kill();
	} else if (atoi(argv[1]) < 1) {
		cout << "O tamanho do vetor deve ser um número maior do que 0";
		kill();
	} else if (atoi(argv[2]) < 1) {
		cout << "O numero de buckets deve ser um número maior do que 0";
		kill();
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
		for (k=i; k<tamvet; k++)
			buckets[bucketAtual][k] = -1;  // Preenche elementos do vetor que nao estao no bucket com -1
		valorInicialAtual += tamanhoFaixaAtual;
		// IMPRIME PARA TESTE:
/*			for (i=0; i<tamanhoBucket[bucketAtual]; i++)
				cout << buckets[bucketAtual][i] << ' ';
			cout << "tamanhoFaixaAtual = " << tamanhoFaixaAtual << '\n';
		//FIM teste */
	}
}

void bubbleSort(int *v, int tam) {
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

void executaEscravo() {
	while (true) {
		int *bucketRecebido, executar, tamBucket;
		cout << "AAAAA\n";
		MPI_Recv(&executar, 1, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		cout << "BBBBB\n";
		if (executar == 0)
			break;
		MPI_Recv(&bucketRecebido, tamvet, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		cout << "CCCCC\n";

		MPI_Recv(&tamBucket, 1, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		cout << "DDDDD\n";
		
		//TESTE
		cout << bucketRecebido[0];
		//MOREU AQUI
		// bucketRecebido ESTÁ VAZIO 
		
		bubbleSort(bucketRecebido, tamBucket);
		cout << "EEEEE\n";
		MPI_Send(&bucketRecebido, tamvet, MPI_INT, 0, 0, MPI_COMM_WORLD);
 	}
 }

void sendBucket(int rank, int bucketPosit) {
	int executar = 1;
	MPI_Send(&executar, 1, MPI_INT, rank, 0, MPI_COMM_WORLD);  // Envia mensagem avisando que deve executar
	MPI_Send(&buckets[bucketPosit], tamvet, MPI_INT, rank, 0, MPI_COMM_WORLD);  // envia bucket
	MPI_Send(&tamanhoBucket[bucketPosit], 1, MPI_INT, rank, 0, MPI_COMM_WORLD);  // envia tamanho do bucket
}

void redistribuir() {
	int i, j, k;  
	k = 0;  // k = Posição Atual
	for (i=0; i<nbuckets; i++) {
		for(j=0; j<tamanhoBucket[i]; j++) {
			vetorPrincipal[k] = buckets[i][j];
			k++;
		}
	}
}

void executaMestre(int nEscravos) {
	geraVetorAleatorio();
	imprimeVetor();
	separaVetorParaBuckets();

	int k;
	int nroBucketsEnviados = 0;
	int nroBucketsRecebidos = 0;
	int positBucketAtualDoEscravo[nEscravos];
	for (k = 1; k < nEscravos+1 && k < nbuckets; k++) {  // Envia inicialmente para todos os escravos (ou até enviar todos os buckets).
		if (tamanhoBucket[k-1] <= 1)  // Buckets vazios ou com apenas 1 elemento não devem ser enviados.
			k++;  // Pula bucket
		positBucketAtualDoEscravo[k] = k-1;
		sendBucket(k, k-1);
		cout << "Mestre ENVIOU bucket " << (k-1) << " para Escravo " << k << "\n";
		nroBucketsEnviados++;		
	}
	
	for (; nroBucketsRecebidos < nbuckets;) {  // Loop para receber todos.
		int tempBucket[tamvet];
		MPI_Status st;
		MPI_Recv(&tempBucket, tamvet, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &st);
		int source = st.MPI_SOURCE;
		int posit = positBucketAtualDoEscravo[source];
		cout << "Mestre RECEBEU bucket " << posit << " do Escravo " << source << "\n";
		buckets[posit] = tempBucket;
		nroBucketsRecebidos++;

		if (nroBucketsEnviados < nbuckets) {  // Se ainda restam buckets, envia para o escravo que acabou de terminar.
			int positProx = nroBucketsEnviados;
			positBucketAtualDoEscravo[k] = positProx;
			sendBucket(source, positProx);
			cout << "Mestre ENVIOU bucket " << positProx << " para Escravo " << source << "\n";
			nroBucketsEnviados++;	
		}
	}
	int executar = 0;
	for (k = 1; k < nEscravos+1; k++) // Loop para terminar os escravos.
		MPI_Send(&executar, 1, MPI_INT, k, 0, MPI_COMM_WORLD);  // Envia mensagem avisando que não deve executar.
	redistribuir();  // Redistribui buckets ordenados no vetor original;
}

int main(int argc, char *argv[]) {
	verificaDados(argc, argv);
	nbuckets = atoi(argv[2]);
	tamvet = atoi(argv[1]);
	// INICIA MPI:
	int rank;
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &nprocs);  // nprocs recebe o numero de processos criados (incluindo o main).
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);    // Rank do processo
	if (nprocs < 2) {
		// Pode não ter chamado pelo mpirun
		cout << "O número de processos deve ser um número maior do que 1\n";
		verificaDados(0, argv);  // Imprime erro de formatação inválida
	}
	int nEscravos = nprocs - 1;
	if (rank == 0)
		executaMestre(nEscravos);
	else 
		executaEscravo();
	MPI_Finalize();
	imprimeVetor();
	return 0;
};
