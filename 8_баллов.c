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

    int fd1, fd2, fd3;
    pid_t pid;
    
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
    } else if (pid == 0) {      //1 процесс
        
        //Открываем 1 канал для чтения
         if ((fd1 = open("pipe1", O_RDONLY)) == -1) {
            perror("open");
            return 1;
        }

        //Открываем 2 канал для записи
        if ((fd2 = open("pipe2", O_WRONLY)) == -1) {
            perror("open");
            return 1;
        }
        char buf;
        char num_buf[BUFSIZE];
        int num_pos = 0;
        //цикл обработки первоначальной строки из 1 канала и записи новой строки во 2 канал
        while (read(fd1, &buf, 1) > 0) {
            if (isdigit(buf)) {
                num_buf[num_pos++] = buf;
            } else if (num_pos > 0) {
                num_buf[num_pos++] = '+';
                write(fd2, num_buf, num_pos);
                num_pos = 0;
            }
        }

        if (num_pos > 0) {
            num_buf[num_pos++] = '+';
            write(fd2, num_buf, num_pos);
        }
    
        //Закрываем файловые дескрипторы
        close(fd1);
        close(fd2);
        
        unlink("pipe1");
        unlink("pipe2");
        
    } else {      //2 процесс
    
        //Открываем 1 канал для записи
        if ((fd1 = open("pipe1", O_WRONLY)) == -1) {
            perror("open");
            return 1;
        }

        //Открываем входной файл для чтения
        if ((fd2 = open(input_file, O_RDONLY)) == -1) {
            perror("open");
            return 1;
        }    
        
        char buf[BUFSIZE];
        int n;
        //Читаем из входного файла и записываем в 1 канал
        while ((n = read(fd2, buf, BUFSIZE)) > 0) {
            if (write(fd1, buf, n) == -1) {
                perror("write");
                return 1;
            }
        }
        //Закрываем файловые дескрипторы
        close(fd1);
        close(fd2);
        
        buf[0] = '\0';
        n = 0;
        
        //Открываем 2 канал для чтения
        if ((fd1 = open("pipe2", O_RDONLY)) == -1) {
            perror("open");
            return 1;
        }
        
        //Открываем выходной файл для записи
        if ((fd2 = open(output_file, O_WRONLY | O_CREAT, 0666)) == -1) {
            perror("open");
            return 1;
        }
        
         //Читаем из 2 канала и записываем в выходной файл
        while ((n = read(fd1, buf, BUFSIZE)) > 0) {
            if (write(fd2, buf, n) == -1) {
                perror("write");
                return 1;
            }
        }

        //Закрываем файловые дескрипторы
        close(fd1);
        close(fd2);
        
        unlink("pipe1");
        unlink("pipe2");
    }
    return 0;
}
