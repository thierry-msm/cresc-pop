#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <math.h>
#include <time.h>

#define NUM_COLONIAS 8
#define NUM_RECURSOS 4

int todas_colonias_terminaram = 0;  // Variável global de controle
pthread_mutex_t rand_mutex;  // Mutex para proteger o uso de rand()

// Estrutura para armazenar os parâmetros de cada colônia
typedef struct {
    int id;
    double P0;
    double r;
    double T;
} Colonia;

pthread_mutex_t recursosA[NUM_RECURSOS];
pthread_mutex_t recursosB[NUM_RECURSOS];

int deadlock_detectado = 0;

// Função que verifica continuamente se houve deadlock
void* verificar_deadlock(void* arg) {
    int tempo_maximo_espera = 5;

    while (!todas_colonias_terminaram) {
        sleep(tempo_maximo_espera);

        if (deadlock_detectado == 0) {
            printf("\n[DEADLOCK DETECTADO]: As colônias estão presas em um impasse!\n");
        } else {
            printf("\nNenhum deadlock detectado até agora.\n");
        }
    }

    printf("Verificação de deadlock terminada, todas as colônias concluíram.\n");
    pthread_exit(NULL);
}

// Função que cada thread de colônia executa
void* colonia(void* arg) {
    Colonia* colonia_data = (Colonia*)arg;

    // Proteger o uso de rand() com um mutex
    pthread_mutex_lock(&rand_mutex);
    int recurso_id = rand() % NUM_RECURSOS;
    pthread_mutex_unlock(&rand_mutex);

    int id = colonia_data->id;

    if (id % 2 == 1) {
        printf("Colônia %d tentando adquirir Recurso A[%d]\n", id, recurso_id);
        pthread_mutex_lock(&recursosA[recurso_id]);
        sleep(1);
        printf("Colônia %d adquiriu Recurso A[%d], agora tentando Recurso B[%d]\n", id, recurso_id, recurso_id);
        pthread_mutex_lock(&recursosB[recurso_id]);
    } else {
        printf("Colônia %d tentando adquirir Recurso B[%d]\n", id, recurso_id);
        pthread_mutex_lock(&recursosB[recurso_id]);
        sleep(1);
        printf("Colônia %d adquiriu Recurso B[%d], agora tentando Recurso A[%d]\n", id, recurso_id, recurso_id);
        pthread_mutex_lock(&recursosA[recurso_id]);
    }

    deadlock_detectado = 1;

    printf("Colônia %d adquiriu ambos os recursos. Crescimento em andamento...\n", id);
    for (int t = 0; t <= colonia_data->T; t++) {
        printf("Tempo %d: População da Colônia %d: %.2f\n", t, id, colonia_data->P0 * exp(colonia_data->r * t));
        sleep(1);
    }

    pthread_mutex_unlock(&recursosA[recurso_id]);
    pthread_mutex_unlock(&recursosB[recurso_id]);
    printf("Colônia %d liberou os recursos.\n", id);

    pthread_exit(NULL);
}

int main() {
    srand(time(NULL));  // Inicializa a semente para geração de números aleatórios
    pthread_mutex_init(&rand_mutex, NULL);  // Inicializa o mutex para proteger o rand()

    pthread_t threads[NUM_COLONIAS];
    pthread_t deadlock_verificador;
    Colonia colonias[NUM_COLONIAS];

    for (int i = 0; i < NUM_RECURSOS; i++) {
        pthread_mutex_init(&recursosA[i], NULL);
        pthread_mutex_init(&recursosB[i], NULL);
    }

    printf("Simulação de crescimento de %d colônias.\n", NUM_COLONIAS);

    // Criar thread para verificar deadlock
    pthread_create(&deadlock_verificador, NULL, verificar_deadlock, NULL);

    // Criando threads das colônias
    for (int i = 0; i < NUM_COLONIAS; i++) {
        colonias[i].id = i + 1;
        colonias[i].P0 = (rand() % 500 + 500);
        colonias[i].r = ((double)(rand() % 5 + 1)) / 100;
        colonias[i].T = (rand() % 5 + 5);
        pthread_create(&threads[i], NULL, colonia, &colonias[i]);
    }

    // Esperar todas as colônias terminarem
    for (int i = 0; i < NUM_COLONIAS; i++) {
        pthread_join(threads[i], NULL);
    }

    // Depois que todas as colônias terminarem, definir a variável de controle
    todas_colonias_terminaram = 1;

    // Esperar a thread de verificação de deadlock terminar
    pthread_join(deadlock_verificador, NULL);

    for (int i = 0; i < NUM_RECURSOS; i++) {
        pthread_mutex_destroy(&recursosA[i]);
        pthread_mutex_destroy(&recursosB[i]);
    }

    pthread_mutex_destroy(&rand_mutex);  // Destrói o mutex usado para proteger o rand()

    return 0;
}