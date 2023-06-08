#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

// Определение размера буфера
#define BUFFER_SIZE 1024

// Функция для обработки ошибок
void error(const char *msg) {
    perror(msg);
    exit(1);
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

    // Создание клиентского сокета
    int client_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (client_socket < 0) {
        // Обработка ошибки создания сокета
        error("Ошибка открытия сокета");
    }

    // Настройка адреса сервера
    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = inet_addr(server_ip);
    server_address.sin_port = htons(server_port);

    // Отправка начального сообщения серверу
    char buffer[BUFFER_SIZE];
    snprintf(buffer, BUFFER_SIZE, "Привет, сервер!");
    ssize_t send_size = sendto(client_socket, buffer, strlen(buffer), 0, (struct sockaddr *) &server_address, sizeof(server_address));
    if (send_size < 0) {
        // Обработка ошибки отправки
        error("Ошибка отправки серверу");
    }

    // Прием и обработка сообщений от сервера
    while (1) {
        // Прием сообщений от сервера
        ssize_t recv_size = recvfrom(client_socket, buffer, BUFFER_SIZE - 1, 0, NULL, NULL);
        if (recv_size < 0) {
            // Обработка ошибки приема
            error("Ошибка приема от сервера");
        }

        // Добавление завершающего нулевого символа к полученному сообщению
        buffer[recv_size] = '\0';

        // Вывод полученного сообщения
        printf("%s\n", buffer);

        // Проверка, завершена ли игра
        if (strstr(buffer, "Игра окончена!") != NULL) {
            break;
        }
    }

    // Закрытие клиентского сокета
    close(client_socket);

    return 0;
}
