#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define BUFSIZE 5000

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s input_file output_file\n", argv[0]);
        return 1;
    }
    //Аргументы командной строки
    char *input_file = argv[1];
    char *output_file = argv[2];

    pid_t pid;
    char buffer[BUFSIZE], output_buffer[BUFSIZE];
    int fd1, fd2, pipe1, pipe2;
    int bytes_read, bytes_written;

    //Открываем входной файл и записываем его содержимое в буфер
    if ((fd1 = open(input_file, O_RDONLY)) == -1) {
        perror("open");
        return 1;
    }
    if ((bytes_read = read(fd1, buffer, BUFSIZE)) == -1) {
        perror("read");
        close(fd1);
        return 1;
    }
    buffer[bytes_read] = '\0';
    close(fd1);

    //Создание 1 именованного канала
    if (mkfifo("pipe1", 0666) == -1) {
        perror("mkfifo");
        return 1;
    }

    //Создание 2 именованного канала
    if (mkfifo("pipe2", 0666) == -1) {
        perror("mkfifo");
        return 1;
    }

    if ((pid = fork()) == -1) {
        perror("fork");
        return 1;
    }
    else if (pid == 0) {

        //Чтение данных из 1 именованного канала
        if ((pipe1 = open("pipe1", O_RDONLY)) == -1) {
            perror("open");
            return 1;
        }
        if ((bytes_read = read(pipe1, buffer, BUFSIZE)) == -1) {
            perror("read");
            close(pipe1);
            return 1;
        }
        buffer[bytes_read] = '\0';
        close(pipe1);

        //цикл обработки первоначальной строки и записи новой строки в буфер
        int i = 0, j = 0;
        while (buffer[i] != '\0') {
            if (isdigit(buffer[i])) {
                while (isdigit(buffer[i])) {
                    output_buffer[j++] = buffer[i++];
                }
                output_buffer[j++] = '+';
            }
            else {
                i++;
            }
        }
        output_buffer[j-1] = '\0';

        //Запись данных из буфера во 2 именованный канал
        if ((pipe2 = open("pipe2", O_WRONLY)) == -1) {
            perror("open");
            return 1;
        }
        if ((bytes_written = write(pipe2, output_buffer, strlen(output_buffer))) == -1) {
            perror("write");
            close(pipe2);
            return 1;
        }
        close(pipe2);

        return 0;
    }
    else {
        //Запись данных из буфера в 1 именованный канал
        if ((pipe1 = open("pipe1", O_WRONLY)) == -1) {
            perror("open");
            return 1;
        }
        if ((bytes_written = write(pipe1, buffer, strlen(buffer))) == -1) {
            perror("write");
            close(pipe1);
            return 1;
        }
        close(pipe1);

        //Чтение обработанных данных из 2 именованного канала
        if ((pipe2 = open("pipe2", O_RDONLY)) == -1) {
            perror("open");
            return 1;
        }
        if ((bytes_read = read(pipe2, output_buffer, BUFSIZE)) == -1) {
            perror("read");
            close(pipe2);
            return 1;
        }
        output_buffer[bytes_read] = '\0';
        close(pipe2);

        //Запись обработанных данных в выходной файл
        if ((fd2 = open(output_file, O_WRONLY | O_CREAT, 0666)) == -1) {
            perror("open");
            return 1;
        }
        if ((bytes_written = write(fd2, output_buffer, strlen(output_buffer))) == -1) {
            perror("write");
            close(fd2);
            return 1;
        }
        close(fd2);

        //Удаление именованных каналов
        unlink("pipe1");
        unlink("pipe2");

        return 0;
    }
}
