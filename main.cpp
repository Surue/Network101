#include <WinSock2.h>
/*
#define WIN32_LEAN_AND_MEAN //Obliger si on utilise avec WinSock2
#include <windows.h>
*/

#pragma comment(lib, "ws2_32.lib") //permet de linker la librairie

#include <iostream>
#include <vector>

bool selectRecv(SOCKET socket, int interval_us = 1)
{
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(socket, &fds);
	timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = interval_us;
	return (select(socket + 1, &fds, 0, 0, &tv) == 1);
}

bool selectAccept(SOCKET socket, int interval_us = 1)
{
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(socket, &fds);
	timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = interval_us;
	return (select(socket + 1, &fds, 0, 0, &tv) == 1);
}

void SendMessageToEveryOne(std::vector<SOCKET> clients, std::string message)
{
	for (int i = 0; i < clients.size(); i++)
	{
		send(clients[i], &message[0], message.size(), NULL);
	}
}

int main()
{
	//Startup version of socket
	WORD word = MAKEWORD(2, 2);
	WSADATA wsaData;

	if(WSAStartup(word, &wsaData) != 0) //return 0 if succeed
	{
		std::cerr << "Failed to get socket version\n";
		return -1;
	}
	std::cout << "Startup socket version\n";

	//Create socket's listener
	SOCKET listenerSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(listenerSocket == INVALID_SOCKET)
	{
		std::cerr << "Failed to create socket listener\n";
		return -2;
	}
	std::cout << "Create socket listener\n";

	//Create an address
	sockaddr_in hint{};
	hint.sin_family = AF_INET;
	hint.sin_port = htons(42000); //htons => Host TO Network Short
	hint.sin_addr.S_un.S_addr = INADDR_ANY;

	//Bind socket to address
	if(bind(listenerSocket, (sockaddr*)&hint, sizeof hint) != 0)
	{
		std::cerr << "Failed to bind socket to address\n";
		return -2;
	}
	std::cout << "Bind socket listener to address\n";

	//Start listening
	if(listen(listenerSocket, SOMAXCONN) == SOCKET_ERROR)
	{
		std::cerr << "Failed to listen\n";
		return -3;
	}
	std::cout << "Start listening\n";

	std::vector<SOCKET> clients;

	while(true)
	{
		if(selectAccept(listenerSocket))
		{
			//Accept entering connections 
			SOCKET clientSocket = accept(listenerSocket, NULL, NULL);
			if (clientSocket == INVALID_SOCKET)
			{
				std::cerr << "Failed to create client socket\n";
				return -4;
			}
			std::cout << "New client connected\n";
			SendMessageToEveryOne(clients, "New client connected\n");
			clients.push_back(clientSocket);
		}

		std::vector<int> indexClientDisconnected;

		for (int i = 0; i < clients.size(); i++)
		{
			if(selectRecv(clients[i]))
			{
				//Create buffer for listening
				const auto bufferLength = 512;
				std::vector<char> buffer;
				buffer.resize(bufferLength);

				//Receive data from client
				const auto sizeReception = recv(clients[i], &buffer[0], bufferLength, NULL);

				//Client disconnected
				if(sizeReception == 0) 
				{
					std::cout << "Client disconnected\n";
					SendMessageToEveryOne(clients, "Client disconnected\n");
					indexClientDisconnected.push_back(i);
					continue;
				}

				//Send message to everyone
				for(int j = 0; j < clients.size(); j++)
				{
					if(j != i)
					{
						send(clients[j], &buffer[0], bufferLength, NULL);
					}
				}

				//write message on server
				for (auto k = 0; k < sizeReception; k++)
				{
					std::cout << static_cast<char>(buffer[k]);
				}
			}
		}

		//Remove disconnected client
		if (!indexClientDisconnected.empty())
		{
			std::vector<SOCKET> newClients;

			for (int i = 0; i < clients.size(); i++)
			{
				bool isDisconnected = false;

				for (int j = 0; j < indexClientDisconnected.size(); j++)
				{
					if (i == j)
					{
						isDisconnected = true;
						break;
					}
				}

				if (!isDisconnected)
				{
					newClients.push_back(clients[i]);
				}
			}

			clients.clear();

			for (int i = 0; i < newClients.size(); i++)
			{
				clients.push_back(newClients[i]);
			}
		}
	}

	//Pause 
	system("pause");

	//Cleanup
	closesocket(listenerSocket);

	WSACleanup(); //Delete all pointer and all shit

	return 0;
}
