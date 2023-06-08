#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <time.h>

// Определение размера буфера
#define BUFFER_SIZE 1024

// Функция для обработки ошибок
void error(const char *msg) {
    perror(msg);
    exit(1);
}

// Функция для определения игрока с наивысшей энергией
int findWinner(int *energies, int *is_alive) {
    int max_energy = 0;
    int winner = -1;
    for (int i = 0; i < 3; i++) {
        if (is_alive[i] && energies[i] > max_energy) {
            max_energy = energies[i];
            winner = i;
        }
    }
    return winner;
}

int main(int argc, char *argv[]) {
    // Проверка аргументов командной строки
    if (argc != 3) {
        printf("Использование: %s <IP> <порт>\n", argv[0]);
        return 1;
    }

    // Получение IP-адреса сервера
    char *server_ip = argv[1];
    // Получение порта сервера
    int server_port = atoi(argv[2]);

    // Создание серверного сокета
    int server_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_socket < 0) {
        // Обработка ошибки создания сокета
        error("Ошибка при открытии сокета");
    }

    // Конфигурирование адреса сервера
    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = inet_addr(server_ip);
    server_address.sin_port = htons(server_port);

    // Привязка сокета к порту
    if (bind(server_socket, (struct sockaddr *) &server_address, sizeof(server_address)) < 0) {
        // Обработка ошибки привязки
        error("Ошибка при привязке");
    }

    printf("Сервер ожидает клиентов...\n");

    // Инициализация информации об игроках
    int energies[3]; // Энергия каждого игрока
    int is_alive[3]; // Жив ли игрок
    int resting_player; // Игрок, который будет отдыхать
    int has_rest_event_occurred = 0; // Флаг события отдыха
    srand(time(NULL));
    for (int i = 0; i < 3; i++) {
        // Генерация случайной энергии для каждого игрока
        energies[i] = rand() % 100 + 1;
        is_alive[i] = 1;
    }

    // Выбор игрока для отдыха
    resting_player = rand() % 3;

    // Получение начальных сообщений от клиентов
    char buffer[BUFFER_SIZE];
    struct sockaddr_in client_addresses[3];
    socklen_t client_address_len;

    for (int i = 0; i < 3; i++) {
        // Получение сообщений от клиентов
        client_address_len = sizeof(client_addresses[i]);
        ssize_t recv_size = recvfrom(server_socket, buffer, BUFFER_SIZE - 1, 0,
                                     (struct sockaddr *) &client_addresses[i], &client_address_len);
        if (recv_size < 0) {
            error("Ошибка при получении от клиента");
        }

        // Печать полученного сообщения
        printf("Получено сообщение от клиента: %s\n", buffer);

        // Разбор полученного сообщения и инициализация информации об игроке
        sscanf(buffer, "%*s %*s %d", &energies[i]);

        if (i == resting_player) {
            snprintf(buffer, BUFFER_SIZE,
                     "Вы игрок %d. Ваша начальная энергия составляет %d."
                     " Вы будете отдыхать и удвоите свою энергию во время первой битвы.\n",
                     i, energies[i]);
        } else {
            snprintf(buffer, BUFFER_SIZE, "Вы игрок %d."
                                          " Ваша начальная энергия составляет %d.\n",
                     i, energies[i]);
        }

        // Отправка сообщения клиенту
        ssize_t send_size = sendto(server_socket, buffer, strlen(buffer), 0,
                                   (struct sockaddr *) &client_addresses[i], client_address_len);
        if (send_size < 0) {
            error("Ошибка при отправке клиенту");
        }
    }

    printf("Инициализация завершена. Начало битвы...\n");

    // Проведение битв до тех пор, пока останется только один живой игрок
    int alive_players = 3;
    while (alive_players > 1) {
        // Выбор двух случайных игроков для битвы
        int player1, player2;
        do {
            player1 = rand() % 3;
            player2 = rand() % 3;
        } while (player1 == player2 || !is_alive[player1] || !is_alive[player2]);

        // Проверка, у какого игрока больше энергии
        int battle_winner, battle_loser;
        if (energies[player1] > energies[player2]) {
            battle_winner = player1;
            battle_loser = player2;
        } else {
            battle_winner = player2;
            battle_loser = player1;
        }

        // Обновление энергии после битвы
        energies[battle_winner] += energies[battle_loser];
        is_alive[battle_loser] = 0;
        alive_players--;

        // Отправка сообщений о битве обоим игрокам
        snprintf(buffer, BUFFER_SIZE, "Битва: Игрок %d против Игрока %d\n", battle_winner, battle_loser);
        ssize_t send_size = sendto(server_socket, buffer, strlen(buffer), 0,
                                   (struct sockaddr *) &client_addresses[battle_winner], client_address_len);
        if (send_size < 0) {
            error("Ошибка при отправке клиенту");
        }

        snprintf(buffer, BUFFER_SIZE, "Битва: Игрок %d против Игрока %d\n", battle_loser, battle_winner);
        send_size = sendto(server_socket, buffer, strlen(buffer), 0,
                           (struct sockaddr *) &client_addresses[battle_loser], client_address_len);
        if (send_size < 0) {
            error("Ошибка при отправке клиенту");
        }

        // Вывод информации о битве
        printf("Битва: Игрок %d против Игрока %d\n", battle_winner, battle_loser);
        printf("Игрок %d побеждает в битве против Игрока %d!\n", battle_winner, battle_loser);

        // Проверка, завершил ли отдыхающий игрок отдых
        if (battle_winner == resting_player && !has_rest_event_occurred) {
            has_rest_event_occurred = 1;
            printf("Игрок %d закончил отдых и удвоил свою энергию до %d.\n", battle_winner, energies[battle_winner]);
        }

        // Вывод оставшейся энергии каждого игрока
        for (int i = 0; i < 3; i++) {
            if (is_alive[i]) {
                printf("Энергия Игрока %d: %d\n", i, energies[i]);
            }
        }
    }

    // Определение победителя
    int winner = findWinner(energies, is_alive);
    printf("Игрок %d побеждает в игре!\n", winner);

    // Закрытие серверного сокета
    close(server_socket);

    return 0;
}
