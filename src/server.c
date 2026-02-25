#include "server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

void *handle_client(void *args) {
  ClientHandlerArgs *handler_args = (ClientHandlerArgs *)args;
  Server *server = handler_args->server;
  int client_index = handler_args->client_index;
  int client_socket = server->clients[client_index].socket;
  int max_clients = handler_args->max_clients;
  free(handler_args);

  char buffer[1024];

  while (1) {
    int bytes_read = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_read <= 0) {
      if (bytes_read == 0) {
        printf("Client %d disconnected\n", client_index);
      }
      else {
        perror("recv");
      }
      break;
    }

    buffer[bytes_read] = '\0';

    char tagged[1200];
    snprintf(tagged, sizeof(tagged), "[Client %d] %s", client_index, buffer);

    broadcast_message(server, client_index, tagged, max_clients);
  }

  pthread_mutex_lock(&server->mutex);
  server->clients[client_index].active = 0;
  server->num_clients--;
  pthread_mutex_unlock(&server->mutex);
  
  close(client_socket);
  return NULL;
}

/*
void broadcast_message(Server *server, int sender_index, const char *message, int max_clients) {
  pthread_mutex_lock(&server->mutex);

  for (int i = 0; i < max_clients; i++) {
    if (server->clients[i].active && i != sender_index) {
      send(server->clients[i].socket, message, strlen(message), 0);
    }
  }

  pthread_mutex_unlock(&server->mutex);
} */

void broadcast_message(Server *server, int sender_index, const char *message, int max_clients) {
  pthread_mutex_lock(&server->mutex);

  /*
  char ip_port_message[1024];
  Client sender = server->clients[sender_index];
  char sender_ip[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &(sender.address.sin_addr), sender_ip, INET_ADDRSTRLEN);
  int sender_port = ntohs(sender.address.sin_port);

  sprintf(ip_port_message, "%-20s%-10d%s", sender_ip, sender_port, message);
  */
  for (int i = 0; i < max_clients; i++) {
    if (server->clients[i].active) {
      send(server->clients[i].socket, message, strlen(message), 0);
      //send(server->clients[i].socket, ip_port_message, strlen(ip_port_message), 0);
    }
  }
  pthread_mutex_unlock(&server->mutex);
}

int main(int argc, char *argv[]) {
  
  if (argc != 2) {
    fprintf(stderr, "Usage: %s <max clients>\n", argv[0]);
    exit(1);
  }

  int max_clients = atoi(argv[1]);
  if (max_clients <= 0) {
    fprintf(stderr, "Invalid number of max clients. It must be a positive integer.\n");
    exit(1);
  }

  Server server;
  server.num_clients = 0;
  server.clients = malloc(sizeof(Client) * max_clients);
  
  if (server.clients == NULL) {
    perror("Failed to allocate memory");
    exit(1);
  }

  pthread_mutex_init(&server.mutex, NULL);

  int server_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (server_socket < 0) {
    perror("socket");
    exit(1);
  }

  struct sockaddr_in server_address;
  memset(&server_address, 0, sizeof(server_address));
  server_address.sin_family = AF_INET;
  server_address.sin_addr.s_addr = INADDR_ANY;
  server_address.sin_port = htons(PORT);

  if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
    perror("bind");
    exit(1);
  }

  if (listen(server_socket, max_clients) < 0) {
    perror("listen");
    exit(1);
  }

  printf("Server started on port %d\n", PORT);

  while(1) {
    struct sockaddr_in client_address;
    socklen_t client_address_len = sizeof(client_address);
    int client_socket = accept(server_socket, (struct sockaddr *)&client_address, &client_address_len);

    if (client_socket < 0) {
      perror("accept");
      continue;
    }

    pthread_mutex_lock(&server.mutex);

    if (server.num_clients >= max_clients) {
      printf("Maximum number of clients reached\n");
      close(client_socket);
      pthread_mutex_unlock(&server.mutex);
      continue;
    }

    int client_index = -1;
    for (int i = 0; i < max_clients; i++) {
      if (!server.clients[i].active) {
        server.clients[i].socket = client_socket;
        server.clients[i].address = client_address;
        server.clients[i].active = 1;
        client_index = i;
        server.num_clients++;
        printf("Accepted client %d from %s:%d\n", client_index, inet_ntoa(client_address.sin_addr),
            ntohs(client_address.sin_port));
        break;
      }
    }

    pthread_mutex_unlock(&server.mutex);

    if (client_index == -1) {
      printf("Failed to allocare client index\n");
      close(client_socket);
      continue;
    }

    ClientHandlerArgs *args = malloc(sizeof(ClientHandlerArgs));
    if (!args) {
      perror("Failed to allocare memory");
      close(client_socket);
      continue;
    }
    args->server = &server;
    args->client_index = client_index;
    args->max_clients = max_clients;

    pthread_t thread;
    if (pthread_create(&thread, NULL, handle_client, args) != 0) {
      perror("pthread_create");
      close(client_socket);
      server.clients[client_index].active = 0;
      server.num_clients--;
      free(args);
    }
    else {
      pthread_detach(thread);
    }
  }
  close(server_socket);
  free(server.clients);
  pthread_mutex_destroy(&server.mutex);
  
  return 0;
}



