#include "stdafx.h"
#pragma comment(lib, "ws2_32.lib")
#include <winsock2.h>
#include <iostream>
#include <string>
#include <ctime>
#include <chrono>
#include <iostream>
#include <tchar.h>
#include <Windows.h>
#include <tlhelp32.h>
using namespace std;

#pragma warning(disable: 4996)

int MaxClientsCount = 10;
SOCKET Sockets[10];
int SockCount = 0;


string Information[10];

string GetInformation() {

	string result = "";

	//получает информацию о потоке с помощью моментального снимка процессов и потоков

	HANDLE hSnapshot;
	hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, GetCurrentProcessId());//снимок

	PROCESSENTRY32 peProcessEntry;//структура для получения инфформации о процессе
	peProcessEntry.dwSize = sizeof(PROCESSENTRY32);
	Process32First(hSnapshot, &peProcessEntry);//получает первый процесс в снимке

	result+= "Информация о серверном процессе  и его потоках:\n";
	do {
		//находит именно текущий процесс
		if (GetCurrentProcessId() == peProcessEntry.th32ProcessID) {
			result += "Id процесса: " + std::to_string(peProcessEntry.th32ProcessID) +"\n";
			result += "Id количество потоков: " + std::to_string(peProcessEntry.cntThreads) + "\n";
			result += "Id базовый приоритет его потоков: " + std::to_string(peProcessEntry.pcPriClassBase) + "\n";
			break;
		}
	} while (Process32Next(hSnapshot, &peProcessEntry));//перебирает процессы
	CloseHandle(hSnapshot);
	return result;
}


void PrintTime() {

	auto mytime = std::chrono::system_clock::now();
	std::time_t timeSent = std::chrono::system_clock::to_time_t(mytime);
	std::cout << "Отправка сообщения клиенту выполнена в  " << std::ctime(&timeSent) << "\n";
}



void SentParameters(int index) {

	Information[index] = GetInformation();
	
	char msg[200];
	//преобразование строки к массиву символов для отправки
	strcpy(msg, Information[index].c_str());
	//отправка сообщения
	send(Sockets[index], msg, sizeof(msg), NULL); // сокет, буфер с инфой, размер буфера, флаги
	//вывод времени отправки
	PrintTime();

	//раз в  10 секунд сервер вычисляет информацю, если она изменилась, то вновнь отправялет клиенту
	while (true) {

		Sleep(10000); // аргумент в миллисекундах
		string inf= GetInformation();

		if (Information[index] != inf) {

			Information[index] = inf;
			strcpy(msg, Information[index].c_str());

			//если клиент отключися перестает отправляться
			if (send(Sockets[index], msg, sizeof(msg), NULL) == SOCKET_ERROR) {
				std::cout << "Прервано\n";
				return;
			}
			//если соединение нормальное и отправка завершена, печатается время отправления
			else {PrintTime();}
		}
	}
}


int main(int argc, char* argv[]) {

	setlocale(LC_ALL, "Russian");

	WSAData wsaData;
	WORD DLLVersion = MAKEWORD(2, 1);
	if (WSAStartup(DLLVersion, &wsaData) != 0) {
		std::cout << "Error" << std::endl;
		exit(1);
	}
	
	HANDLE hMutex = CreateMutex(NULL, TRUE, L"MutexForServer2");
	if (GetLastError() == ERROR_ALREADY_EXISTS) {
		cout << "Уже есть один экземпляр сервера!";
		exit(0);
	}

	SOCKADDR_IN addr;// структура для хранения адреса
	int sizeofaddr = sizeof(addr);
	addr.sin_addr.s_addr = inet_addr("127.0.0.2");//ip - local host
	addr.sin_port = htons(1112);//незарезервированный хост
	addr.sin_family = AF_INET;// семейство адресов, в данном случае Ipv4

	SOCKET sListen = socket(AF_INET,// семейство адресов, в данном случае Ipv4
		SOCK_STREAM,//протокол соединения, в нашем случае TCP
		NULL); // тип протокола, используем TCP, поэтому здесь NULL

	bind(sListen,(SOCKADDR*)&addr,sizeof(addr)); // привязка сокета к паре айпи - порт

	listen(sListen, MaxClientsCount); // первый аргумент - "слушающий" сокет, второй аргумент - макс. кол-во процессов для подкл.

	SOCKET newConnection;
	for (int i = 0; i < MaxClientsCount; i++) {

		newConnection = accept(sListen, (SOCKADDR*)&addr, &sizeofaddr); // 1 слущающий сокет, 2 указатель на пустоту, куда запишем инфу по подключившемуся клиенту, 3 размер структуры

		if (newConnection == 0) {
			std::cout << "Error connection with client " << i + 1 << "\n";
		}
		else
		{
			std::cout << "Client " << i + 1 << " connected!" << "\n";
			Sockets[i] = newConnection;
			SockCount++;
			CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)SentParameters, (LPVOID)(i), NULL, NULL);
		}
	}


	system("pause");
	return 0;
}