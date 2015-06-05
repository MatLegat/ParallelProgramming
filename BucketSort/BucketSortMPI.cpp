// Compilar com 'mpic++'
#include <mpi.h>
#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <vector>
using namespace std;

int tamvet, nbuckets, nprocs;
int * vetorPrincipal;
int ** buckets;  // Vetor de buckets (vetores)
int * tamanhoBucket;  // Vetor dos tamanhos dos buckets
bool dadosOK;

#define TAG_TAMANHO 0
#define TAG_BUCKET 1
#define ENABLE_PRINT true


void kill(int rank) {
	if (rank == 0)
		cout << "\n\nO programa será finalizado.\n--------------------------------------------------------------------------\n";
	MPI_Finalize();
	exit(EXIT_FAILURE);
}

void verificaDados(int argc, char *argv[], int rank) {
	// Obs: se nao recebeu número como parametro, atoi retorna 0.
	// Só improme se for mestre (rank 0).
	dadosOK = false;
	if (argc != 3) {
		if (rank == 0) {	
			cout << "Parametros invalidos. Considere inserir o comando na forma:\n\n  $ mpirun -np NUMERO_DE_PROCESSOS ";
			cout << argv[0] << " TAMANHO_VETOR NUMERO_BUCKETS";
		}
		kill(rank);
	} else if (atoi(argv[1]) < 1) {
		if (rank == 0)
			cout << "O tamanho do vetor deve ser um número maior do que 0.";
		kill(rank);
	} else if (atoi(argv[2]) < 1) {
		if (rank == 0)	
			cout << "O numero de buckets deve ser um número maior do que 0.";
		kill(rank);
	} else if (atoi(argv[2]) > atoi(argv[1])) {
		if (rank == 0)
			cout << "O número de buckets não deve ser maior do que o tamanho do vetor.";
		kill(rank);
	} else {
		dadosOK = true;
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
	if (!ENABLE_PRINT)
		return;
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
	
	// Loop para verificar todos os buckets:
	for (; bucketAtual < nbuckets; bucketAtual++) {
		
		int tamanhoFaixaAtual = (tamvet - valorInicialAtual) / (nbuckets - bucketAtual);
		if (((tamvet - valorInicialAtual) % (nbuckets - bucketAtual)) != 0)
			tamanhoFaixaAtual ++;
			
		tamanhoBucket[bucketAtual] = 0;
		int k;
		buckets[bucketAtual] = new int[tamvet];  // Desperdício de memória por uma melhor performance.
		
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
		
		// Loop para preencher elementos do vetor que nao estao no bucket com -1.
		for (k=i; k<tamvet; k++)
			buckets[bucketAtual][k] = -1;
		valorInicialAtual += tamanhoFaixaAtual;
	}
}

void bubbleSort(int *v, int tam) {
	int i, j, temp;
	bool trocou;
	for (j = 0; j < tam-1; j++) {
		trocou = false;
		for (i = 0; i < tam-1; i++) {
			if (v[i+1] < v[i]) {
				temp = v[i];
				v[i] = v[i+1];
				v[i+1] = temp;
				trocou = true;
			}
		} 
		if (!trocou)
			break;
	}
}

// Envia o bucket da posição bucketPosit para o processo de rank rank:
void sendBucket(int rank, int bucketPosit) {
	MPI_Send(&tamanhoBucket[bucketPosit], 1, MPI_INT, rank, TAG_TAMANHO, MPI_COMM_WORLD);  // Envia tamanho do bucket.
	MPI_Send(buckets[bucketPosit], tamanhoBucket[bucketPosit], MPI_INT, rank, TAG_BUCKET, MPI_COMM_WORLD);  // Envia bucket.
	if (ENABLE_PRINT)
		cout << "Mestre ENVIOU bucket " << bucketPosit << " para Escravo " << rank << "\n";
}

// Une os buckets de volta no vetor principal:
void redistribuir() {
	int i, j, k;  
	k = 0;  // k = Posição Atual
	for (i=0; i<nbuckets; i++) {
		for (j=0; j<tamanhoBucket[i]; j++) {
			vetorPrincipal[k] = buckets[i][j];
			k++;
		}
	}
}

void executaEscravo() {
	while (true) {
		int tamBucket;
		MPI_Recv(&tamBucket, 1, MPI_INT, 0, TAG_TAMANHO, MPI_COMM_WORLD, MPI_STATUS_IGNORE);  // Recebe tamanho.
			
		if (tamBucket == -1)  // Se recebeu -1, termina escravo.
			break;
			
		int bucketRecebido[tamBucket];
		MPI_Recv(&bucketRecebido, tamBucket, MPI_INT, 0, TAG_BUCKET, MPI_COMM_WORLD, MPI_STATUS_IGNORE);  // Recebe bucket.
		
		bubbleSort(bucketRecebido, tamBucket);  // Ordena bucket.
		
		MPI_Send(&tamBucket, 1, MPI_INT, 0, TAG_TAMANHO, MPI_COMM_WORLD);  // Envia tamanho.
		MPI_Send(&bucketRecebido, tamBucket, MPI_INT, 0, TAG_BUCKET, MPI_COMM_WORLD);  // Envia bucket ordenado.
 	}
}

void executaMestre(int nEscravos) {
	geraVetorAleatorio();
	imprimeVetor();
	separaVetorParaBuckets();

	int k;  // Indice da thread.
	int nroBucketsEnviados = 0;
	int nroBucketsPulados = 0;
	int nroBucketsRecebidos = 0;
	int positBucketAtualDoEscravo[nEscravos+1];  // Para saber pelo rank do escravo qual vetor ele recebeu antes.

	// Envia inicialmente para todos os escravos:
	for (k = 1; k < nEscravos+1; k++) {
		int positSend = nroBucketsEnviados + nroBucketsPulados;
		for (; tamanhoBucket[positSend] <= 1 && positSend < nbuckets;) {  // Buckets vazios ou com apenas 1 elemento não devem ser enviados.
			// Pula bucket:
			positSend++;
			nroBucketsPulados++;
		}
		if (positSend >= nbuckets)  // Se não há mais buckets para enviar, sai do loop.
			break;
		positBucketAtualDoEscravo[k] = positSend;
		sendBucket(k, positSend);  // Envia.
		nroBucketsEnviados++;
	}

	// Loop para receber todos:
	for (; nroBucketsRecebidos < nroBucketsEnviados;) {
		int tempTamanho;
		MPI_Status st;
		
		MPI_Recv(&tempTamanho, 1, MPI_INT, MPI_ANY_SOURCE, TAG_TAMANHO, MPI_COMM_WORLD, &st);  // Recebe tamanho.
		int source = st.MPI_SOURCE;
		int posit = positBucketAtualDoEscravo[source];
		
		MPI_Recv(buckets[posit], tempTamanho, MPI_INT, source, TAG_BUCKET, MPI_COMM_WORLD, &st);  // Recebe bucket.
		if (ENABLE_PRINT)
			cout << "Mestre RECEBEU bucket " << posit << " do Escravo " << source << "\n";
		nroBucketsRecebidos++;

		// Se ainda restam buckets, envia para o escravo que acabou de terminar:
		// Buckets vazios ou com apenas 1 elemento não devem ser enviados.
		int positSend = nroBucketsEnviados + nroBucketsPulados;
		for (; tamanhoBucket[positSend] <= 1 && positSend < nbuckets;) {
			// Pula bucket:
			positSend++;
			nroBucketsPulados++;
		}
		if (positSend < nbuckets) {
			positBucketAtualDoEscravo[source] = positSend;
			sendBucket(source, positSend);  // Envia.
			nroBucketsEnviados++;	
		}
	}
	
	int tamanho = -1;
	// Loop para terminar todos os escravos:
	for (k = 1; k < nEscravos+1; k++)
		MPI_Send(&tamanho, 1, MPI_INT, k, TAG_TAMANHO, MPI_COMM_WORLD);  // envia tamanho -1 para terminar escravo

	redistribuir();
	imprimeVetor();
}

int main(int argc, char *argv[]) {

	int rank;
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &nprocs);  // nprocs recebe o numero de processos criados (incluindo o main).
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);  // Rank do processo.
	
	// Verificação de dados:
		if (nprocs < 2) {
			// Pode não ter sido chamado pelo mpirun.
			cout << "O número de processos deve ser um número maior do que 1.\n";
			verificaDados(0, argv, 0);  // Imprime erro de formatação inválida
		}
		verificaDados(argc, argv, rank);
	
	nbuckets = atoi(argv[2]);
	tamvet = atoi(argv[1]);
	int nEscravos = nprocs - 1;
	
	if (rank == 0)
		executaMestre(nEscravos);
	else
		executaEscravo();
		
	MPI_Finalize();	
	return 0;
};
