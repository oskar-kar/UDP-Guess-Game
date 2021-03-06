//serwer

#include "pch.h"
#include <iostream>
#include <SFML/Network.hpp>
#include <cstdlib>
#include <cstdio>
#include <ctime>

//Funkcja pakująca dane do zmiennej o wielkości 32 bitów

void pakuj(sf::Int32 &data, const int &op, const int &odp, const int &sesja, const unsigned int &czas)
{
	data = 0;
	data = op;
	data = (data << 4) | odp;
	data = (data << 8) | sesja;
	data = (data << 8) | czas;
	data = data << 6;
}

//Funkcja odpakowująca dane ze zmiennej o wielkości 32 bitów

void odpakuj(sf::Int32 &data, int &op, unsigned int &odp, int &sesja, unsigned int &zgadywana)
{
	data = data >> 6;
	zgadywana = data;
	zgadywana = zgadywana << 24;
	zgadywana = zgadywana >> 24;

	data = data >> 8;

	sesja = data;
	sesja = sesja << 24;
	sesja = sesja >> 24;

	data = data >> 8;
	odp = data;
	odp = odp << 28;
	odp = odp >> 28;

	data = data >> 4;

	op = data;

	data = 0;
}

//struktura "Klient" przechowująca dane o kliencie

struct client
{
	sf::IpAddress ip;
	int session_id = 0;
	unsigned short port = 0;
	client()
	{
		this->ip = "";
		this->session_id = 0;
		this->port = 0;
	}
	client(const sf::IpAddress &a, const int &SID, const unsigned short &port)
	{
		this->ip = a;
		this->session_id = SID;
		this->port = port;
	}

};

//Funkcja sprawdzająca czy podany klient został już dodany
//Zapobiega próbie wielokrotnego połączenia się z jednej aplikacji

bool czy_zawiera(const std::vector<client> &v, const unsigned short &port)
{
	for (int i = 0; i < v.size(); i++)
	{
		if (v[i].port == port)
		{
			return true;
		}
	}
	return false;
}

int main()
{

	srand(time(NULL));

	//Wstępne deklaracje zmiennych, portów itp.

	std::cout << "Gierka!" << std::endl;
	sf::UdpSocket socket;
	sf::IpAddress rec_ip;
	socket.bind(50000);

	std::size_t received;
	std::vector<client> klienci;

	unsigned short port;
	sf::Packet recP;
	sf::Packet sendP;

	bool koniec_gry = false;
	bool start_gry = false;

	int op = 0;
	unsigned  odp = 0;
	int sesja = 0;

	int limit_czasu = 0;

	unsigned int liczba = 0;
	unsigned int uliczba = 0;

	sf::Clock czas_gry;
	sf::Clock czas_komunikatu;
	sf::Time time;
	unsigned int czasek = 0;

	int czas = limit_czasu - time.asSeconds();

	sf::Int32 dane = 0;
	socket.setBlocking(false);

	//1 Faza serwera - oczekiwanie na połączenie dwóch klientów
	//Przerywana po połączeniu się dwóch klientów

	while (true)
	{
		//Ustalamy reakcję serwera na odbiór pakietu
		if (socket.receive(recP, rec_ip, port) == sf::UdpSocket::Done)
		{
			//po otrzymaniu pakietu "recP" jego zawartość przesyłamy do zmiennej "dane"
			//a następnie rozdzielamy je na odpowiednie fragmenty
			recP >> dane;
			odpakuj(dane, op, odp, sesja, uliczba);
			//sorawdzamy numer operacji
			if (op == 7)
			{
				//sprawdzamy czy klient nie jest już dodany oraz ich ilość
				if (!czy_zawiera(klienci, port) && klienci.size() != 2)
				{
					//dodajemy klienta oraz przesyłamy potwierdzenie
					klienci.push_back(client(rec_ip, klienci.size() + 1, port));
					dane = 0;
					//pakowanie potwierdzenia do zmiennej "dane"
					pakuj(dane, 0, 0, klienci.size(), limit_czasu);
					//poprzednia zawartość pakietu jest czyszczona
					sendP.clear();
					//zawartość zmiennej "dane" umieszczamy w pakiecie "sendP"
					sendP << dane;
					//Przesyłamy odpowiedź do klienta na podstawie jego IP oraz portu
					if (socket.send(sendP, klienci[klienci.size() - 1].ip, klienci[klienci.size() - 1].port) == sf::UdpSocket::Done)
					{
						std::cout << "Dodano klienta" << std::endl;
						dane = 0;
						//Wysyłamy pakiet z informacją o dołączeniu do sesji oraz numerem sesji równym wielkości
						//vectora "klienci"
						pakuj(dane, 2, 0, klienci.size(), limit_czasu);
						sendP.clear();
						sendP << dane;
						socket.send(sendP, klienci[klienci.size() - 1].ip, klienci[klienci.size() - 1].port);
					}
				}
				//jeżeli klient jest już połączony lub osiągnięto limit graczy
				else
				{
					for (int i = 0; i < klienci.size(); i++)
					{
						//odnajdujemy klienta, którego port jest zgodny z tym, od którego otrzymaliśmy pakiet
						if (port == klienci[i].port)
						{
							//wysyłamy potwierdzenie
							dane = 0;
							pakuj(dane, 0, 0, klienci[i].session_id, limit_czasu);
							sendP.clear();
							sendP << dane;
							if (socket.send(sendP, klienci[i].ip, klienci[i].port) == sf::UdpSocket::Done)
							{
								//wysyłamy informację o nie dodaniu klienta
								std::cout << "Nie dodano klienta" << std::endl;
								dane = 0;
								pakuj(dane, 2, 1, klienci[i].session_id, limit_czasu);
								sendP.clear();
								sendP << dane;
								socket.send(sendP, klienci[i].ip, klienci[i].port);
							}
						}
					}
				}

			}
			//Jeżeli otrzymano potwierdzenie od klienta
			else if (op == 0 && odp == 0)
			{
				std::cout << "Odebrano potwierdzenie komunikatu od gracza nr: " << sesja << std::endl;
				odp = 1;
				if (klienci.size() == 2) break;
			}
		}
	}
	//2 Faza gry - rozgrywka - trwa póki dostępnych jest 2 klientów oraz gra się nie zakończyła
	while (klienci.size() == 2 && koniec_gry == false)
	{
		//sprawdzamy czy jest to pierwsza iteracja tej pętli
		//a więc czy należy zainicjować rozgrywkę
		if (start_gry == false)
		{
			//ustalamy limit czasu według wzoru z zadania
			limit_czasu = ((klienci[0].session_id + klienci[1].session_id) * 99) % 100 + 30;
			//Wysyłamy komunikaty o rozpoczęciu gry
			dane = 0;
			pakuj(dane, 6, 0, klienci[0].session_id, limit_czasu);
			sendP.clear();
			sendP << dane;
			socket.send(sendP, klienci[0].ip, klienci[0].port);

			dane = 0;
			pakuj(dane, 6, 0, klienci[1].session_id, limit_czasu);
			sendP.clear();
			sendP << dane;
			socket.send(sendP, klienci[1].ip, klienci[1].port);

			//restartujemy zegary zadeklarowane na początku programu
			//aby liczyły czas od rozpoczęcia rozgrywki
			czas_gry.restart();
			czas_komunikatu.restart();
			//zmieniamy wartość zmiennej, aby przy następnej iteracji pominąć ten fragment kodu
			//inicjalizacja rozgrywki następuje tylko raz
			start_gry = true;

			//losujemy liczbę
			liczba = std::rand() % 255;

			//W konsoli serwera umieszczamy kontrolnie informacje o grze
			std::cout << "Wylosowana liczba to " << liczba << std::endl;
			std::cout << "Limit czasu to " << limit_czasu << std::endl;

			std::cout << "Rozpoczeto gre" << std::endl;
		}
		//przypisujemy czas gry na podstawie zegaru
		time = czas_gry.getElapsedTime();
		//jeżeli minie 10 sekund na zegarze komunikatu
		if (czas_komunikatu.getElapsedTime().asSeconds() >= 10)
		{
			//resetujemy zegar aby odliczał czas ponownie
			czas_komunikatu.restart();
			//obliczamy pozostały czas
			czasek = limit_czasu - int(czas_gry.getElapsedTime().asSeconds());
			//wyświetlamy go kontrolnie w konsoli serwera
			std::cout <<"Czas: "<< czasek << std::endl;
			//przesyłamy wszystkim klientom komunikat o pozostałym czasie
			for (int i = 0; i < klienci.size(); i++)
			{
				dane = 0;
				pakuj(dane, 5, 0, klienci[i].session_id, czasek);
				sendP.clear();
				sendP << dane;
				socket.send(sendP, klienci[i].ip, klienci[i].port);
			}
		}
		//jeżeli osiągniemy limit czasu gry
		if (limit_czasu <= czas_gry.getElapsedTime().asSeconds())
		{
			std::cout << "KONIEC GRY, KONIEC CZASU" << std::endl;
			//ustalamy odpowiednią zmienną kontrolną
			koniec_gry = true;
			//wysyłamy wszystkim klientom informację o końcu gry z powodu osiągnięcia limitu czasu
			for (int i = 0; i < klienci.size(); i++)
			{
					dane = 0;
					pakuj(dane, 4, 2, klienci[i].session_id, liczba);
					sendP.clear();
					sendP << dane;
					socket.send(sendP, klienci[i].ip, klienci[i].port);
			}
		}
		//Podobnie jak w poprzedniej "fazie gry"
		if (socket.receive(recP, rec_ip, port) == sf::UdpSocket::Done)
		{
			recP >> dane;
			odpakuj(dane, op, odp, sesja,uliczba);
			//Jeżeli odebraliśmy potwierdzenie
			if (op == 0)
			{
				std::cout << "Potwierdzono odbior danych od gracza o ID: "<< sesja << std::endl;
			}
			//Jeżeli klient spróbował zgadnąć liczbę
			if (op == 3)
			{
				//Wysyłamy potwierdzenie odbioru danych
				dane = 0;
				pakuj(dane, 0, 0, sesja, 0);
				sendP.clear();
				sendP << dane;
				socket.send(sendP, rec_ip, port);
				std::cout  << "Liczba uzytkownika "<<  sesja<<": " << uliczba << std::endl;
				//Jeżeli "uliczba" (wysłana przez klienta) jest taka sama jak wylosowana
				if (uliczba == liczba)
				{
					koniec_gry = true;
					//Dla każdego klienta wysyłamy stosowny komunikat
					for (int i = 0; i < klienci.size(); i++)
					{
						//Dla zwycięscy (na podstawie portu)
						if (port == klienci[i].port)
						{
							dane = 0;
							pakuj(dane, 4, 0, klienci[i].session_id, liczba);
							sendP.clear();
							sendP << dane;
							socket.send(sendP, klienci[i].ip, klienci[i].port);
						}
						//oraz innym klientom
						if (port != klienci[i].port)
						{
							dane = 0;
							pakuj(dane, 4, 1, klienci[i].session_id, liczba);
							sendP.clear();
							sendP << dane;
							socket.send(sendP, klienci[i].ip, klienci[i].port);
						}
					}
					std::cout << "Liczba odgadnieta - wyslano wyniki" << std::endl;
				}
				//Jeżeli liczba nie została odgadnięta, przesyłamy klientowi stosowną informację
				if (uliczba != liczba)
				{
					dane = 0;
					pakuj(dane, 2, 2, sesja, 0);
					sendP.clear();
					sendP << dane;
					socket.send(sendP, rec_ip, port);

					std::cout << "Otrzymano bledna liczbe" << std::endl;
				}
			}
		}
	}
	//Serwer po zakończeniu gry działą tak długo, aż nie otrzyma od każdego z klientów
	//komunikatu potwierdzającego odebranie danych o zakończeniu
	int k_licznik = 2;
	while (k_licznik > 0)
	{
		socket.receive(recP, rec_ip, port);
		for (int i = 0; i < klienci.size(); i++)
		{
			if (klienci[i].port == port)
			{
				k_licznik--;
			}
		}
	}
	socket.unbind();
}