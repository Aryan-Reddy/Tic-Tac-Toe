#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <bits/stdc++.h>
#define PORT 8080
using namespace std;
using namespace std::chrono;
int sendMsg(int sock,string msg)
{
	if(send(sock,&msg[0],msg.size(),0) != msg.size())
	{
		cout << "SEND ERROR" << endl;
		return 0;
	}
	return 1;
}
string toupper(string t)
{
	for(char &c:t)
	{
		c = ::toupper((int)c);
	}
	return t;
}
int main()
{
	int sock = 0,valread;
	struct sockaddr_in serv_addr;
	char buffer[4096];
	buffer[0] = 0;
	if((sock = socket(AF_INET,SOCK_STREAM,0)) < 0)
	{
		cout << "SOCKET ERROR";
		return -1;
	}
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(PORT);
	if(inet_pton(AF_INET,"127.0.0.1",&serv_addr.sin_addr) <= 0)
	{
		cout << "Address Invalid\n";
		return -1;
	}
	if(connect(sock,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0)
	{
		cout << "Connection Failed\n";
		return -1;
	}
	cout << "Connected to the game server\n";
	valread = read(sock,buffer,4096);
	buffer[valread] = 0;
	string tmpmsg = string(buffer);
	cout << tmpmsg << endl;
	stringstream X(tmpmsg);
	buffer[0] = 0;
	int op;
	X >> op;
	if(op != 1)
	{
		cout << "Error Wrong msg received";
		return -1;
	}
	int pId;
	X >> pId;
	cout << "Your player Id is: "  << pId << endl;
	cout << "Waiting for a partner to join" << endl;
	valread = read(sock,buffer,4096);
	buffer[valread] = 0;
	tmpmsg.clear();
	tmpmsg = string(buffer);
	X = stringstream(tmpmsg);
	buffer[0] = 0;
	X >> op;
	if(op != 2)
	{
		cout << "Error Wrong msg received";
		return -1;
	}
	int partnerId;
	X >> partnerId;
	string symb;
	X >> symb;
	int gId;
	X >> gId;
	string b1,b2,b3;
	X >> b1;X >> b2;X >> b3;
	string b = b1 + "\n" + b2 + "\n" + b3 + "\n";
	cout << "Your Partner's ID is " <<  partnerId << ". Your symbol is " << symb << endl;
	cout << "Your Game Id is :" << gId << endl;
	cout << "Starting the game..." << endl;
	cout << b << endl;
	while(1)
	{
		buffer[0] = 0;
		valread = read(sock,buffer,4096);
		if(valread == 0)
		{
			cout << "Server Disconnected" << endl;
			cout << "Closing Game" << endl;
			return 0;
		}
		tmpmsg.clear();
		buffer[valread] = 0;
		tmpmsg = string(buffer);
		X = stringstream(tmpmsg);
		X >> op;
		if(op == 1)
		{
			cout << "Invalid Msg received";
			return -1;
		}
		else if(op == 2)
		{
			cout << "Rematch Accepted!!" << endl;
			int partnerId;
			X >> partnerId;
			string symb;
			X >> symb;
			int gameId;
			X >> gameId;
			string b1,b2,b3;
			X >> b1;X >> b2;X >> b3;
			string b = b1 + "\n" + b2 + "\n" + b3 + "\n";
			cout << "Your Partner's ID is " <<  partnerId << ". Your symbol is " << symb << endl;
			cout << "Your Game Id is :" << gameId << endl;
			cout << "Starting the game..." << endl;
			cout << b << endl;
		}
		else if(op == 3)
		{
			cout << "Enter (ROW, COL) for placing your mark:" << endl;
			//auto t = high_resolution_clock::now();
			int r,c;
			cin >> r >> c;
/*			auto t2 = high_resolution_clock::now();
			//cout << "T T2 " << t << " " << t2 << endl;
			auto duration = duration_cast<microseconds>(t2 - t);
			if(duration.count() > 15000)
			{
				cout << "Timer Out!!" << endl;
				cout << "Exiting the game" << endl;
				return 0;
			}*/
			cout << "Moves are " << r << " " << c << endl;
			string msg = "1 " + to_string(r) + " " + to_string(c);
			sendMsg(sock,msg);
		}
		else if(op == 4)
		{
			cout << "INVALID MOVE PERFORMED!!" << endl;
			cout << "TRYING AGAIN" << endl;
			string b1,b2,b3;
			getline(X,b1,'\n');getline(X,b2,'\n');getline(X,b3,'\n');
			string b = b1.substr(1,b1.size()-1) + "\n" + b2 + "\n" + b3 + "\n";
			cout << b << endl;
		}
		else if(op == 5)
		{
			string b1,b2,b3;
			getline(X,b1,'\n');getline(X,b2,'\n');getline(X,b3,'\n');
			string b = b1.substr(1,b1.size()-1) + "\n" + b2 + "\n" + b3 + "\n";
			cout << b << endl;			
		}
		else if(op == 6)
		{
			cout << "Game Ended" << endl;
			int w;
			X >> w;
			string b1,b2,b3;
			getline(X,b1,'\n');getline(X,b2,'\n');getline(X,b3,'\n');
			string b = b1 + "\n" + b2 + "\n" + b3 + "\n";
			cout << b << endl;
			if(w == 1)
			{
				cout << "You Won !!" << endl;
			}
			else if(w == 2)
			{
				cout << "You Lost :(" << endl;
			}
			else if(w == 0)
			{
				cout << "Game Draw :|" << endl;
			}
			else
			{
				cout << "Timer Out!!" << endl;
			}
			cout << "Do you want to play another game with same partner (YES/NO)" << endl;
			string ans;
			cin >> ans;
			cout << "Waiting for other player response" << endl;
			ans = toupper(ans);
			if(ans == "YES")
			{
				string msg = "2 1";
				sendMsg(sock,msg);
			}
			else
			{
				string msg = "2 0";
				sendMsg(sock,msg);
			}
		}
		else if(op == 7)
		{
			cout << "Your Partner Disconnected/Timed out" << endl;
			cout << "Sorry, the game is closing" << endl;
			return 0;
		}
		else if(op == 8)
		{
			cout << "Game is closing because of disagreement" << endl;
			return 0;
		}
		else
		{
			cout << "Error Op not found: "<< op << endl;
			return -1;
		}
	}
	return 0;
}