#include <pthread.h>
#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <vector>
using namespace std;
 
int tamvet, nbuckets, nthreads;
//pthread_mutex_t mutex;
int * vetorPrincipal;
int ** buckets;  // Vetor de buckets (vetores)
int * tamanhoBucket;
pthread_mutex_t mutex;
 
void erroLT0() {  // Less Than 0
    cout << "ERRO: VALOR DEVE SER UM INTEIRO MAIOR DO QUE 0\n"; 
    exit(1);
}
 
void solicitaDados() {
    cout << "Insira o tamanho do vetor: ";
    cin >> tamvet;
    if (tamvet <= 0)
         erroLT0();
    cout << "Insira o numero de buckets: ";
    cin >> nbuckets;
    if (nbuckets <= 0)
        erroLT0();
    else if (nbuckets > tamvet) {
        cout << "ERRO: VALOR NAO PODE SER MAIOR DO QUE O TAMANHO DO VETOR\n";
        exit(1);
    } 
    cout << "Insira o numero de threads: ";
    cin >> nthreads;
        if (nthreads <= 0)
            erroLT0();
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
    cout << "\n\n";
}
 
void separaVetorParaBuckets() {
    cout << "Distribuindo o vetor nos " << nbuckets << " buckets\n";
    buckets = new int*[nbuckets];
    tamanhoBucket = new int[nbuckets];
    int bucketAtual = 0, valorInicialAtual = 0; 
    for (; bucketAtual < nbuckets; bucketAtual++) {
        int tamanhoFaixaAtual = (tamvet - valorInicialAtual) / (nbuckets - bucketAtual);
        if (((tamvet - valorInicialAtual) % (nbuckets - bucketAtual)) != 0)
            tamanhoFaixaAtual ++;
        tamanhoBucket[bucketAtual] = 0;
        int k;
        // Loop para ver quantos elementos estao na faixa do bucket autal:
        for(k=0; k<tamvet; k++) {
            // Se o valor esta na faixa atual:
            int valorAtual = vetorPrincipal[k];
            if (valorInicialAtual <= valorAtual && valorAtual < (valorInicialAtual + tamanhoFaixaAtual))
                tamanhoBucket[bucketAtual] ++;
        } 
        buckets[bucketAtual] = new int[tamanhoBucket[bucketAtual]];
        // Loop para adicionar elementos no bucket atual:
        int i = 0;
        for (k=0; k<tamvet && i<tamanhoBucket[bucketAtual] ; k++) {
            int valorAtual = vetorPrincipal[k];
            if (valorInicialAtual <= valorAtual && valorAtual < (valorInicialAtual + tamanhoFaixaAtual)) {
                buckets[bucketAtual][i] = valorAtual;
                i++;
            }
        }
        valorInicialAtual += tamanhoFaixaAtual;
/*      // IMPRIME PARA TESTE:
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
 
int main() {
    solicitaDados();
    geraVetorAleatorio();
    cout << "Vetor inicial:\n";
    imprimeVetor();
    separaVetorParaBuckets();
    // INICIA THREADS:
    pthread_t threads[nthreads];
    pthread_mutex_init(&mutex, NULL);
    int k, id[nthreads];
    for (k=0; k<nthreads; k++) {
        id[k] = k;
        pthread_create(&threads[k], NULL, threadFunction, (void *)&id[k]);
    }
    // ESPERA THREADS:
    for (k=0; k<nthreads; k++) {
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
    cout << "Vetor ordenado:\n";
    imprimeVetor();
    return 0;
};
