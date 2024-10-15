#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <math.h>
#include <time.h>

#define NUM_COLONIAS 8
#define NUM_RECURSOS 3

// Estrutura para armazenar os parâmetros de cada colônia
typedef struct {
    int id;
    double P0;  // População inicial
    double r;   // Taxa de crescimento
    double T;   // Tempo total de simulação
    unsigned int seed;
} Colonia;

// Arrays de mutexes para representar os recursos
pthread_mutex_t recursosA[NUM_RECURSOS];
pthread_mutex_t recursosB[NUM_RECURSOS];

// Variável para rastrear se ocorreu deadlock
int deadlock_detectado = 0;
int threads_finalizadas = 0;

// Função de crescimento exponencial
double crescimento_populacao(double P0, double r, double t) {
    return P0 * exp(r * t);
}

// Função para verificar deadlock (timeout simples)
void* verificar_deadlock(void* arg) {
    int tempo_maximo_espera = 5;  // Tempo máximo de espera em segundos
    sleep(tempo_maximo_espera);
    while(!threads_finalizadas){
        if (deadlock_detectado == 0) {
        printf("\n[DEADLOCK DETECTADO]: As colonias estao presas em um impasse!\n");
        break; 
        }
    }
    pthread_exit(NULL);
}

// Função que cada thread de colônia executa
void* colonia(void* arg) {
    Colonia* colonia_data = (Colonia*)arg;  // Converte o argumento para a estrutura Colonia
    int id = colonia_data->id;
    int recurso_id = id % NUM_RECURSOS;  // Atribui um recurso com base no ID da colônia

    // As colônias com IDs ímpares adquirem A depois B, e as com IDs pares adquirem B depois A
    if (id % 2 == 1) {
        printf("Colonia %d tentando adquirir Recurso A[%d]\n", id, recurso_id);
        pthread_mutex_lock(&recursosA[recurso_id]);
        sleep(1);
        printf("Colonia %d adquiriu Recurso A[%d], agora tentando Recurso B[%d]\n", id, recurso_id, recurso_id);
        pthread_mutex_lock(&recursosB[recurso_id]);
    } else {
        printf("Colonia %d tentando adquirir Recurso B[%d]\n", id, recurso_id);
        pthread_mutex_lock(&recursosB[recurso_id]);
        sleep(1);
        printf("Colonia %d adquiriu Recurso B[%d], agora tentando Recurso A[%d]\n", id, recurso_id, recurso_id);
        pthread_mutex_lock(&recursosA[recurso_id]);
    }

    // Se a colônia chegar aqui, não houve deadlock
    deadlock_detectado = 1;
    printf("Colonia %d adquiriu ambos os recursos. Crescimento em andamento...\n", id);
    for (int t = 0; t <= colonia_data->T; t++) {
        printf("Tempo %d: Populacao da Colonia %d: %.2f\n", t, id, crescimento_populacao(colonia_data->P0, colonia_data->r, t));
        sleep(1);
    }

    // Libera os recursos após o uso
    pthread_mutex_unlock(&recursosA[recurso_id]);
    pthread_mutex_unlock(&recursosB[recurso_id]);
    printf("Colonia %d liberou os recursos.\n", id);

    pthread_exit(NULL);
}

int main() {
    // Inicializando a semente para geração de números aleatórios
    srand(time(NULL));

    // Inicializando os arrays de mutexes
    for (int i = 0; i < NUM_RECURSOS; i++) {
        pthread_mutex_init(&recursosA[i], NULL);
        pthread_mutex_init(&recursosB[i], NULL);
    }

    pthread_t threads[NUM_COLONIAS];
    pthread_t deadlock_verificador;
    Colonia colonias[NUM_COLONIAS];

    printf("Simulacao de crescimento de %d colonias.\n", NUM_COLONIAS);

    // Criar thread para verificar deadlock
    pthread_create(&deadlock_verificador, NULL, verificar_deadlock, NULL);

    // Criando 8 colônias com valores aleatórios de população inicial, taxa de crescimento e tempo de simulação
    for (int i = 0; i < NUM_COLONIAS; i++) {
        colonias[i].id = i + 1;
        colonias[i].P0 = (rand() % 500 + 500);  // População inicial entre 500 e 1000
        colonias[i].r = ((double)(rand() % 5 + 1)) / 100;  // Taxa de crescimento entre 0.01 e 0.05
        colonias[i].T = (rand() % 5 + 5);  // Tempo de simulação entre 5 e 10 segundos
        colonias[i].seed = rand();
        // Criando as threads das colônias
        pthread_create(&threads[i], NULL, colonia, &colonias[i]);
    }

    // Esperando todas as threads terminarem
    for (int i = 0; i < NUM_COLONIAS; i++) {
        pthread_join(threads[i], NULL);
    }
    threads_finalizadas = 1;

    // Verificação do deadlock
    pthread_join(deadlock_verificador, NULL);

    // Destruindo os mutexes após o uso
    for (int i = 0; i < NUM_RECURSOS; i++) {
        pthread_mutex_destroy(&recursosA[i]);
        pthread_mutex_destroy(&recursosB[i]);
    }

    return 0;
}
