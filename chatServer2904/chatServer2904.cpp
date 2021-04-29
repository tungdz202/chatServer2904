#include <stdio.h>
#include <winsock2.h>
#pragma comment(lib, "ws2_32")

DWORD WINAPI ClientThread(LPVOID);
void RemoveClient(SOCKET);

SOCKET clients[64];
int numClients;

int main() {
	WSADATA wsa;
	WSAStartup(MAKEWORD(2, 2), &wsa);

	SOCKADDR_IN addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(9000);

	SOCKET listener = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);

	bind(listener, (SOCKADDR*)&addr, sizeof(addr));
	listen(listener, 5);

	numClients = 0;

	while (true) {
		SOCKET client = accept(listener, NULL, NULL);
		printf("New client connected: %d\n", client);
		CreateThread(0, 0, ClientThread, &client, 0, 0);
	}
}

DWORD WINAPI ClientThread(LPVOID lpParam) {
	SOCKET client = *(SOCKET*)lpParam;
	int ret;
	char buf[256], msg[512], cmd[32], option[32];
	while (true) {
		ret = recv(client, buf, sizeof(buf), 0);
		if (ret <= 0) {
			RemoveClient(client);
			break;
		}
		buf[ret] = 0;
		printf("%d: %s %s\n", client, cmd, buf);
		_itoa_s(client, msg, 10);
		strcat_s(msg, buf);
		if (strcmp(cmd, "CONNECT")) {
			int id = atoi(option);
			int existed = 0;
			for (int i = 0; i < numClients; i++) {
				if (id == clients[i]) {
					existed = 1;
					break;
				}
			}
			if (existed) send(client, "ERROR", 6, 0);
			else {
				send(client, "OK", 3, 0);
			}
		}
		else if (strcmp(cmd, "SEND")) {
			if (strcmp(option, "ALL")) { 
				for (int i = 0; i < numClients; i++) {
					if (clients[i] != client) {
						send(clients[i], msg, strlen(msg), 0);
					}
				}
				send(client, "OK", 3, 0);
			}
			else { 
				if (atoi(option) < numClients && atoi(option) != client) {
					for (int i = 0; i < numClients; i++) {
						if (clients[i] == atoi(option)) send(clients[i], msg, strlen(msg), 0);
					}
					send(client, "OK", 3, 0);
				}
				else send(client, "ERROR", 6, 0);
			}
		}
		else if (strcmp(cmd, "LIST")) {
			char loggedin[512];
			strcat_s(loggedin, "Logged in users:\n");
			for (int i = 0; i < numClients; i++) {
				if (clients[i] != NULL) {
					_itoa_s(clients[i], loggedin, 10);
					strcat_s(loggedin, "\n");
				}
			}
			send(client, loggedin, strlen(loggedin), 0);
		}
		else if (strcmp(cmd, "DISCONNECT")) {
			int success = 0;
			for (int i = 0; i < numClients; i++) {
				if (clients[i] == client) {
					for (int j = i; j < numClients; j++) {
						clients[j] = clients[j + 1];
						success = 1;
					}
					break;
				}
			}
			if (success) send(client, "OK", 3, 0);
			else send(client, "ERROR", 6, 0);
		}
	}
	closesocket(client);
}
