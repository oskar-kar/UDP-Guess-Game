//klient

#include "pch.h"
#include <iostream>
#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <bitset>
#include <string>

//Funkcja konwertująca liczbę na ciąg bitów - wykorzystywana w celach kontrolnych
void to_binary(int x)
{
	bool tab[32];
	for (int i = 31; i >= 0; i--)
	{
		if (x > 0)
		{
			tab[i] = x % 2;
			x /= 2;
		}
		else
		{
			tab[i] = 0;
		}
	}
	for (int i = 0; i < 32; i++)
	{
		std::cout << tab[i] << " ";
	}
}

//Funkcja pakująca dane do zmiennej 32-bitowej
void pakuj(sf::Int32 &data,  const int &op, const int &odp, const int &sesja, const unsigned int &czas)
{
	data = 0;
	data = op;
	data = (data << 4) | odp;
	data = (data << 8) | sesja;
	data = (data << 8) | czas;
	data = data << 6;
}
//Funkcja odpakowująca dane ze zmiennej 32-bitowej
void odpakuj(sf::Int32 &data, int &op, int &odp, int &sesja, unsigned int &czas)
{
	data = data >> 6;

	czas = data;
	czas = czas << 24;
	czas = czas >> 24;
	
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
//Struktura zawartości okienka wyświetlanego dla użytkownika
//wraz z polami tekstu, czcionkami, wielkością czcionki itp.
struct okienko
{
	sf::Text pole[6];
	sf::Font f;
	okienko()
	{
		f.loadFromFile("OpenSans-Regular.ttf");
		for (int i = 0; i < 6; i++)
		{
			pole[i].setCharacterSize(15);
			pole[i].setFont(f);
			pole[i].setFillColor(sf::Color::White);
			pole[i].setPosition(50, 10+( i * 50));
		}
	}
	void drw(sf::RenderWindow &w)
	{
		for (int i = 0; i < 6; i++)
		{
			w.draw(pole[i]);
		}
	}
};

int main()
{

	//Okno użytkownika
	sf::RenderWindow window(sf::VideoMode(600, 600), "Aplikacja klienta", sf::Style::Default);

	//Wstępne deklaracje zmiennych, adresów, portów itp.

	sf::UdpSocket connection;
	okienko gui;
	connection.bind(sf::Socket::AnyPort);
	connection.setBlocking(false);

	bool czy = true;
	bool dolaczony = false;
	bool koniec_gry = false;
	bool start = false;

	sf::Int32 data = 0;
	int op = 0;
	int sesja = 0;
	int odp = 0;
	int type = 0;
	unsigned int czas = 0;

	sf::Packet no_kek;

	unsigned int liczba = 0;
	sf::IpAddress ip_addr;
	std::string s_ip = "";
	sf::IpAddress serwer = "127.0.0.1";
	unsigned short port = 50000;
	int odepe = 0;
	std::string t;
	gui.pole[3].setString(std::to_string(liczba));


	//dopóki okno jest otwarte, a więc aplikacja ma działać
	while (window.isOpen())
	{
		//obsługiwane "wydarzenie" w oknie
		sf::Event evient;
		while (window.pollEvent(evient))
		{
			gui.pole[2].setString("Adres polaczenia: "+serwer.toString());
			//jeżeli użytkownik nie ma przypisanego ID
			if (sesja == 0)
			{
				gui.pole[5].setString("Twoje ID: BRAK");
			}
			//w przeciwnym wypadku
			else
			{
				gui.pole[5].setString("Twoje ID: " + std::to_string(sesja));
			}
			//Jeżeli kliknięto "X" w narożniku aplikacji, jest ona zamykana
			if (evient.type == sf::Event::Closed)
			{
				window.close();
			}
			//Reakcja na wciskane klawisze
			if (evient.type == sf::Event::KeyPressed)
			{
				//Aby każde wciśnięcie klawicza gracza było interpretowane tylko raz używamy zmiennej "Czy"
				//Domyślnie przyjmuje wartość "true", a więc program może interpretować co użytkownik wcisnął
				//Pierwszą czynnością przed interpretowaniem klawisza jest ustawienie jej wartości na "false"
				//Aby zapobiec wielokrotnemu odczytaniu tego samego klawisza
				//Po "odciśnięciu" klawisza, zmienna spowrotem przyjmuje wartość "true"
				if (czy)
				{
					//Jeżeli IP serwera nie zostało jeszcze wpisane
					if (type == 0)
					{
						//interpretujemy odpowiednie klawisze jako kolejne znaki adresu IP
						if (evient.key.code == 50) s_ip += '.';
						else if (evient.key.code - 26 >= 0 && evient.key.code - 26 <= 9) s_ip += std::to_string(evient.key.code - 26);
						//Backspace usuwa ostatni znak
						else if (evient.key.code == 59 && s_ip.size() > 0) s_ip.pop_back();
						//Escape resetuje podawany adres
						else if (evient.key.code == 36 ) s_ip = "";
						//Enter zatwierdza wpisany adres, przypisuje go jako adres serwera, zmienia zmienną "type"
						//aby wyjść z tego "etapu" konfiguracji gry
						else if (evient.key.code == 58)
						{
							type = 1;
							serwer = s_ip;
							s_ip = "";
							gui.pole[3].setString(std::to_string(liczba));
							continue;
						}
						gui.pole[3].setString(s_ip);
						
					}
					//Po wpisaniu adresu IP
					else
					{
						//Jeżeli wciśnięto "S" oraz klient nadal jest nie dołączony
						if (evient.key.code == sf::Keyboard::Key::S  && dolaczony == false)
						{
							odp = 0;
							liczba = 0;
							sesja = 0;
							op = 1;
							//Czyścimy poprzednią zawartość pakietu "no_kek"
							no_kek.clear();
							//Pakujemy dane do zmiennej "dane"
							pakuj(data, 7, 0, 0, 0);
							//Zawartość zmiennej "dane" umieszczamy w pakiecie
							no_kek << data;
							//Kontrolnie wyświetlamy binarną zawartość zmiennej dane w konsoli klienta
							to_binary(data);
							//Wysyłamy pakiet do serwera
							if (connection.send(no_kek, serwer, 50000) == sf::Socket::Done)
							{
							}
						}
						czy = false;
						//Proces wpisywania liczb edytuje wartość zmiennej "liczba" na podstawie kolejnych
						//wpisywanych cyfr oraz klawisza Backspace
						if (evient.key.code - 26 >= 0 && evient.key.code - 26 <= 9)
						{
							if ((liczba * 10) + (evient.key.code - 26) <= 255)
							{
								liczba = (liczba * 10) + (evient.key.code - 26);
								gui.pole[3].setString(std::to_string(liczba));
							}
						}
						else if (evient.key.code == 59)
						{
							liczba /= 10;
							gui.pole[3].setString(std::to_string(liczba));

						}
						//Jeżeli wciśnięto Enter, klient jest dołączony, lecz gra nie wystartowała
						else if (evient.key.code == 58 && dolaczony == true && start == false)
						{
							gui.pole[0].setString("gra sie nie rozpoczela");
						}
						//Jeżeli wciśnięto Enter, klient jest dołączony, a gra wystartowała
						else if (evient.key.code == 58 && dolaczony == true)
						{
							//Wyświetlamy podaną liczbę w konsoli
							std::cout << liczba << std::endl;
							no_kek.clear();
							//Przesyłamy zgadywaną liczbę do serwera odpowiednim komunikatem
							pakuj(data, 3, 0, sesja, liczba);
							no_kek << data;
							
							if (connection.send(no_kek, serwer, 50000) == sf::Socket::Done)
							{
								std::cout << "wyslano dane" << std::endl;
							}
							liczba = 0;
							gui.pole[3].setString(std::to_string(liczba));

						}
					}
				}
			}
			//Gdy klawisz zostaje zwolniony, "czy" przyjmuje wartosc "true" aby program był gotowy
			//na interpretacje kolejnego klawisza
			if (evient.type == sf::Event::KeyReleased)
			{
				czy = true;
			}
		}
		//Dopóki gracz nie jest dołączony, wyświetla się instrukcja
		if (dolaczony == false)
		{
			gui.pole[0].setString("Twoim zadaniem jest odgadniecie liczby od 0 do 255");

			gui.pole[1].setString("Kliknij S aby dolaczyc do sesj");
		}
		//Reakcje klienta na odbierane dane
		if (connection.receive(no_kek, ip_addr, port) == sf::UdpSocket::Done)
		{
			//Zawartość odebranego pakietu przesyłamy do zmiennej "odepe"
			no_kek >> odepe;
			//"odepe" dzielimy na odpowiednie fragmenty
			odpakuj(odepe, op, odp, sesja, czas);
			//Sprawdzamy wartośi operacji
			//Odebrano potwierdzenie
			if (op == 0)
			{
				std::cout << "Odebrano potwierdzenie od serwera" << std::endl;
				gui.pole[1].setString("Potwierdzono odbior danych");
			}
			//Odpowiedź serwera
			if (op == 2)
			{
				//Klient został przyłączony
				if (odp == 0)
				{
					std::cout << "Dodano cie do gry" << std::endl;
					gui.pole[1].setString(" Dodano cie do gry");

					dolaczony = true;
					//Wysyłamy potwierdzenie
					no_kek.clear();
					pakuj(data, 0, 0, sesja, 0);
					no_kek << data;
					connection.send(no_kek, ip_addr, 50000);
				}
				//Klient nie został przyłączony
				if (odp == 1)
				{
					std::cout << "Serwer otrzymal wiadomosc lecz nie dodal cie do gry" << std::endl;
					gui.pole[1].setString("Serwer otrzymal wiadomosc lecz nie dodal cie do gry");
					//Wysyłamy potwierdzenie
					no_kek.clear();
					pakuj(data, 0, 0, sesja, 0);
					no_kek << data;
					connection.send(no_kek, ip_addr, 50000);
				}
				//Klient nie odgadnął liczby
				if (odp == 2)
				{
					std::cout << "nie odgadnieto liczby" << std::endl;
					gui.pole[1].setString("nie odgadnieto liczby");
					//Wysyłamy potwierdzenie
					no_kek.clear();
					pakuj(data, 0, 0, sesja, 0);
					no_kek << data;
					connection.send(no_kek, ip_addr, 50000);
				}
			}
			//Otrzymano komunikat o końcu gry
			if (op == 4)
			{
				//Klient wygrał
				if (odp == 0)
				{
					//Wysyłamy potwierdzenie
					no_kek.clear();
					pakuj(data, 0, 0, sesja, 0);	
					no_kek << data;
					connection.send(no_kek, ip_addr, 50000);

					std::cout << "Wygrales!" << std::endl;
					gui.pole[0].setString("Wygrales !");
					gui.pole[1].setString("Koniec gry ! Wylosowana liczba to: "+std::to_string(czas));

					//Nastąpił koniec gry
					koniec_gry = true;
				}
				//Klient przegrał - przeciwnik odgadnął liczbę
				else if (odp == 1)
				{
					//Wysyłamy potwierdzenie
					no_kek.clear();
					pakuj(data, 0, 0, sesja, 0);	
					no_kek << data;
					connection.send(no_kek, ip_addr, 50000);

					std::cout << "Przegrales - przeciwnik odgadnal liczbe" << std::endl;
					gui.pole[0].setString("Przegrales - przeciwnik odgadnal liczbe");
					gui.pole[1].setString("Koniec gry ! Wylosowana liczba to: " + std::to_string(czas));

					koniec_gry = true;
				}
				//Klient przegrał - upłynął czas
				else if (odp == 2)
				{
					//Wysyłamy potwierdzenie
					no_kek.clear();
					pakuj(data, 0, 0, sesja, 0);
					no_kek << data;
					connection.send(no_kek, ip_addr, 50000);

					std::cout << "Przegrales - uplynal limit czasu" << std::endl;
					gui.pole[0].setString("Przegrales - uplynal limit czasu");
					gui.pole[1].setString("Koniec gry ! Wylosowana liczba to: " + std::to_string(czas));
					gui.pole[4].setString("Czas: 0");

					koniec_gry = true;
				}
				
			}
			//Otrzymano regularny komunikat o pozostałym czasie
			if (op == 5)
			{
				//Wysyłamy potwierdzenie
				no_kek.clear();
				pakuj(data, 0, 0, sesja, 0);
				no_kek << data;
				connection.send(no_kek, ip_addr, 50000);

				std::cout << "Pozostalo " << czas << " sekund" << std::endl;
				gui.pole[4].setString("Czas: "+std::to_string(czas));


			}
			//Otrzymano powiadomienie o początku gry
			if (op == 6)
			{
				//Wysyłamy potwierdzenie
				no_kek.clear();
				pakuj(data, 0, 0, sesja, 0);
				no_kek << data;
				connection.send(no_kek, ip_addr, 50000);
				std::cout << "Gra rozpoczeta!"<<czas << std::endl;
				gui.pole[0].setString("Gra rozpoczeta! Wpisuj liczby!");
				gui.pole[1].setString("");
				gui.pole[4].setString("Czas: " + std::to_string(czas));
				start = true;
			}
		}
		//Rysujemy nową "klatkę" okna
		gui.drw(window);
		//Wyświetlamy "klatkę" okna
		window.display();
		//Czyścimy "klatkę" okna, przy następnej iteracji pętli wyświetli się zaktualizowana "klatka"
		window.clear();
		
	}
}