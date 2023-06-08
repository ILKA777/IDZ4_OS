#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

// Определяем размер буфера
#define BUFFER_SIZE 1024

// Функция для обработки ошибок
void error(const char *msg) {
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[]) {
    // Проверяем аргументы командной строки
    if (argc != 3) {
        printf("Использование: %s <IP-адрес сервера> <порт>\n", argv[0]);
        return 1;
    }

    // Получаем IP-адрес сервера
    char *server_ip = argv[1];
    // Получаем порт сервера
    int server_port = atoi(argv[2]);

    // Создаем клиентский сокет
    int client_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (client_socket < 0) {
        // Обрабатываем ошибку создания сокета
        error("Ошибка открытия сокета");
    }

    // Конфигурируем адрес сервера
    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = inet_addr(server_ip);
    server_address.sin_port = htons(server_port);

    // Отправляем начальное сообщение серверу
    char buffer[BUFFER_SIZE];
    snprintf(buffer, BUFFER_SIZE, "Привет, сервер!");
    ssize_t send_size = sendto(client_socket, buffer, strlen(buffer), 0, (struct sockaddr *) &server_address, sizeof(server_address));
    if (send_size < 0) {
        error("Ошибка отправки серверу");
    }

    // Принимаем и обрабатываем сообщения от сервера
    while (1) {
        // Принимаем сообщения от сервера
        ssize_t recv_size = recvfrom(client_socket, buffer, BUFFER_SIZE - 1, 0, NULL, NULL);
        if (recv_size < 0) {
            error("Ошибка приема от сервера");
        }

        // Добавляем нулевой символ в конце принятого сообщения
        buffer[recv_size] = '\0';

        // Выводим принятое сообщение
        printf("%s\n", buffer);

        // Проверяем, завершена ли игра
        if (strstr(buffer, "Игра окончена!") != NULL) {
            break;
        }
    }

    // Закрываем клиентский сокет
    close(client_socket);

    return 0;
}
