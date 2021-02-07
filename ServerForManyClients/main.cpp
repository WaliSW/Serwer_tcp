#include <iostream>
#include <WS2tcpip.h>
#include <string>
#include <sstream>

#pragma comment (lib, "ws2_32.lib")

using namespace std;

int main()
{
	// Inicjalizacja winsock
	WSADATA wsData;
	WORD ver = MAKEWORD(2, 2);
	int wsOk = WSAStartup(ver, &wsData);
	if (wsOk != 0)
	{
		cerr << "Can't Initialize winsock! Quitting" << endl;
		return 1;
	}

	// tworzenie socketu
	SOCKET listening = socket(AF_INET, SOCK_STREAM, 0);
	if (listening == INVALID_SOCKET)
	{
		cerr << "Can't create a socket! Quitting" << endl;
		return 2;
	}
	// Bindowanie ip do scoketu
	sockaddr_in hint;
	hint.sin_family = AF_INET;
	hint.sin_port = htons(54000);
	inet_pton(AF_INET,"192.168.1.100", &hint.sin_addr);

	bind(listening, (sockaddr*)&hint, sizeof(hint));

	// Przekazywanie do winsock ¿e socket s³ucha
	listen(listening, SOMAXCONN);

	// Create the master file descriptor set and zero it
	fd_set master;
	FD_ZERO(&master);
	FD_SET(listening, &master);

	// \quit komenda do wyjscia
	bool running = true;

	while (running)
	{

		fd_set copy = master;

		// Kto do nas pisze
		int socketCount = select(0, &copy, nullptr, nullptr, nullptr);

		// pêtla dla obecnych po³¹czeñ i nowych
		for (int i = 0; i < socketCount; i++)
		{
			// Wrzucenie socketów do array
			SOCKET sock = copy.fd_array[i];

			if (sock == listening)
			{
				// Akceptacja nowego po³¹czenia
				SOCKET client = accept(listening, nullptr, nullptr);

				// Dodawanie nowego po³aczenia do listy po³¹czeñ
				FD_SET(client, &master);

				// wys³anie wiadomoœci witaj¹cej
				string welcomeMsg = "Welcome to the Awesome Chat Server!\r\n";
				send(client, welcomeMsg.c_str(), welcomeMsg.size() + 1, 0);
			}
			else
			{
				char buf[4096];
				ZeroMemory(buf, 4096);

				// odbieranie wiadomoœæi
				int bytesIn = recv(sock, buf, 4096, 0);
				if (bytesIn <= 0)
				{
					// wy³¹czenie scoketa
					closesocket(sock);
					FD_CLR(sock, &master);
				}
				else
				{
					// sprawdzanie czy to komenda \quit wy³¹cza serwer
					if (buf[0] == '\\')
					{
						
						string cmd = string(buf, bytesIn);
						if (cmd == "\\quit")
						{
							running = false;
							break;
						}

						// nie wiadomo jaka komenmda
						continue;
					}

					// wysy³anie wiadomoœci do innych po³¹czeñ

					for (int i = 0; i < master.fd_count; i++)
					{
						SOCKET outSock = master.fd_array[i];
						ostringstream ss;
						ss << "SOCKET #" << sock << ": " << buf << "\r\n";
						string strOut = ss.str();
						send(outSock, strOut.c_str(), strOut.size() + 1, 0);
					}
				}
			}
		}
	}


	FD_CLR(listening, &master);
	closesocket(listening);

	// wiadomoœæ do klientów
	string msg = "Server is shutting down. Goodbye\r\n";

	while (master.fd_count > 0)
	{
		// Pobierz numer coketu
		SOCKET sock = master.fd_array[0];

		// wyœlij wiadomoœæ na dowidzenia
		send(sock, msg.c_str(), msg.size() + 1, 0);

		// usuñ z listy master i zamknij socket
		FD_CLR(sock, &master);
		closesocket(sock);
	}

	// wyczyœæ WSAC
	WSACleanup();
	return 0;
	system("pause");
}