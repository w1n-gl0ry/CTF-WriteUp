#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <sys/random.h>
#include <sys/socket.h>
#include <sys/sendfile.h>

#define NUM_TRIES 100
#define NUM_MESSAGES 0x10
#define LICENSE_LENGTH sizeof(*license_key)

int try_count;
int client_socket;
FILE* client_read;
long* license_key;
char* messages[NUM_MESSAGES];

/**
 * Read the flag from flag.txt and send it to client_socket
 */
void send_flag() {
    // open flag
    int fd = open("flag.txt", O_RDONLY);
    if (fd == -1) {
        perror("failed to open the flag");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    // send flag
    if (sendfile(client_socket, fd, NULL, 256) == -1) {
        perror("failed to send the flag");
        close(fd);
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    // cleanup
    close(fd);
    close(client_socket);
    exit(EXIT_SUCCESS);
}

/**
 * Handle the connection of client_socket
 */
void serve_connection() {
    // convert socket to unbuffered file handles
    client_read = fdopen(client_socket, "r");
    if (client_read == NULL) {
        perror("failed to convert socket to file handle");
        close(client_socket);
        exit(EXIT_FAILURE);
    }
    setvbuf(client_read, NULL, _IONBF, 0);

    // enter loop
    while (1) {
        dprintf(client_socket, "Welcome to the secret message service.\n"
            "\n"
            "1: Write a short message\n"
            "2: Write a long message\n"
            "3: Send a message\n"
            "4: Enter license\n"
            "5: exit\n"
            "> "
        );
        int choice = 0;
        fscanf(client_read, "%d", &choice);
        switch(choice) {
            // Write a short message
            case 1:
                for (int i = 0; i < NUM_MESSAGES; i++) {
                    if (messages[i] == NULL) {
                        messages[i] = malloc(64);
                        dprintf(client_socket, "Please enter your short message: ");
                        read(client_socket, messages[i], 0x64);
                        dprintf(client_socket, "message stored at index %d\n", i);
                        break;
                    }
                }
                break;
            // Write a long message
            case 2:
                for (int i = 0; i < NUM_MESSAGES; i++) {
                    if (messages[i] == NULL) {
                        messages[i] = malloc(1040);
                        dprintf(client_socket, "Please enter your long message: ");
                        read(client_socket, messages[i], 1040);
                        dprintf(client_socket, "message stored at index %d\n", i);
                        break;
                    }
                }
                break;
            // Send a message
            case 3:
                dprintf(client_socket, "Please enter the index of the message: ");
                int index = 0;
                fscanf(client_read, "%d", &index);
                if (index < 0 || index >= NUM_MESSAGES) {
                    dprintf(client_socket, "index out of bounds\n");
                    continue;
                }
                // TODO: add logic for actually sending the message
                if (messages[index]) {
                    free(messages[index]);
                    messages[index] = NULL;
                }
                break;
            // Enter license
            case 4:
                dprintf(client_socket, "Please enter the license key: ");
                long key = 0;
                fscanf(client_read, "%lx", &key);
                if (key != license_key) {
                    dprintf(client_socket, "Wrong key!\n");
                } else {
                    dprintf(client_socket, "Congratulations, the key was correct!\nYou got it in %d tries.\n", try_count);
                    send_flag();
                }
                break;
            // exit
            case 5:
                dprintf(client_socket, "Goodbye.\n");
                fclose(client_read);
                exit(EXIT_SUCCESS);
            // also exit on invalid input
            default:
                dprintf(client_socket, "Unsupported option!\n");
                fclose(client_read);
                exit(EXIT_FAILURE);
        }
    }
}

int main(int argc, char *argv[])
{
    // set up pipes
    setvbuf(stdin, NULL, _IONBF, 0);
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    ssize_t size;
    if (getrandom(&size, 1, 0) == -1) {
        perror("failed to generate size");
        exit(EXIT_FAILURE);
    }
    if (malloc(size << 4) == NULL) {
        perror("failed to malloc");
        exit(EXIT_FAILURE);
    }

    // generate license key
    license_key = malloc(LICENSE_LENGTH);
    if (license_key == NULL) {
        perror("failed to malloc license");
        exit(EXIT_FAILURE);
    }
    ssize_t license_length = getrandom(license_key, LICENSE_LENGTH, 0);
    if (license_length == -1) {
        perror("failed to generate license");
        exit(EXIT_FAILURE);
    } else if (license_length < LICENSE_LENGTH) {
        fprintf(stderr, "failed to generate license key.\n");
        exit(EXIT_FAILURE);
    }

    // get socket
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("failed to create socket");
        exit(EXIT_FAILURE);
    }

    // bind socket to random port
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = 0;
    addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(server_socket, (struct sockaddr*) &addr, sizeof(addr)) == -1) {
        perror("failed to bind socket");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    // listen on port
    if (listen(server_socket, 0x10) == -1) {
        perror("failed to listen on socket");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    // get port
    socklen_t addr_len = sizeof(addr);
    if (getsockname(server_socket, (struct sockaddr*) &addr, &addr_len) == -1) {
        perror("failed to get socket information");
        close(server_socket);
        exit(EXIT_FAILURE);
    }
    printf("listening on port %d\n", ntohs(addr.sin_port));

    // start timer
    alarm(300);

    // allow multiple tries
    for (try_count = 0; try_count < NUM_TRIES; try_count++) {
        // clean up defunct childs
        while (waitpid(-1, NULL, WNOHANG) > 0) {
            // clean up next child
        }

        client_socket = accept(server_socket, NULL, NULL);
        if (client_socket == -1) {
            perror("failed to accept connection");
            close(server_socket);
            exit(EXIT_FAILURE);
        }

        pid_t child = fork();
        if (child == 0) {
            serve_connection();
        } else if (child == -1) {
            perror("failed to fork child");
            close(server_socket);
            exit(EXIT_FAILURE);
        } else {
            close(client_socket);
        }
    }

    return 0;
}
