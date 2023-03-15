#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/wait.h>
#define BUFSIZE 5000

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s input_file output_file\n", argv[0]);
        return 1;
    }

    //Аргументы командной строки
    char *input_file = argv[1];
    char *output_file = argv[2];

    //Создание неименованных каналов
    int pipe1[2], pipe2[2];
    if (pipe(pipe1) < 0 || pipe(pipe2) < 0) {
        perror("pipe");
        return 1;
    }

    //Разветвляем первый дочерний процесс для чтения из входного файла и записи в 1 канал
    if (fork() == 0) {
        close(pipe1[0]);
        close(pipe2[0]);
        close(pipe2[1]);

        int fd = open(input_file, O_RDONLY);
        char buf[BUFSIZE];
        int n;
        while ((n = read(fd, buf, BUFSIZE)) > 0) {
            write(pipe1[1], buf, n);
        }

        close(fd);
        close(pipe1[1]);

        return 0;
    }

    //Разветвляем второй дочерний процесс для чтения из 1 канала и записи во 2 канал
    if (fork() == 0) {
        close(pipe1[1]);
        close(pipe2[0]);

        char buf;
        char num_buf[BUFSIZE];
        int num_pos = 0;
        //цикл обработки первоначальной строки и записи новой строки во 2 канал
        while (read(pipe1[0], &buf, 1) > 0) {
            if (isdigit(buf)) {
                num_buf[num_pos++] = buf;
            } else if (num_pos > 0) {
                num_buf[num_pos++] = '+';
                write(pipe2[1], num_buf, num_pos);
                num_pos = 0;
            }
        }

        if (num_pos > 0) {
            num_buf[num_pos++] = '+';
            write(pipe2[1], num_buf, num_pos);
        }

        close(pipe1[0]);
        close(pipe2[1]);

        return 0;
    }

    //Разветвляем третий дочерний процесс для чтения из 2 канала и записи в выходной файл
    if (fork() == 0) {
        close(pipe1[0]);
        close(pipe1[1]);
        close(pipe2[1]);

        int fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0666);
        char buf[BUFSIZE];
        int n;
        while ((n = read(pipe2[0], buf, BUFSIZE)) > 0) {
            write(fd, buf, n);
        }

        close(fd);
        close(pipe2[0]);

        return 0;
    }

    //Закрываем все каналы в родительском процессе
    close(pipe1[0]);
    close(pipe1[1]);
    close(pipe2[0]);
    close(pipe2[1]);

    //Завершения всех дочерних процессов
    for (int i = 0; i < 3; ++i) {
        wait(NULL);
    }

    return 0;
}
