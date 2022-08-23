#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <bits/stdc++.h>
#define PORT 8080
using namespace std;
function<int(void)> Clock;
struct Board{
	string board;
	int cnt;
	Board()
	{
		board = "__|__|__\n__|__|__\n__|__|__\n";
		cnt = 0;
	}
	void clearBoard()
	{
		cnt = 0;
		board = "__|__|__\n__|__|__\n__|__|__\n";
	}
	void showBoard()
	{
		cout << board;
	}
	int checkWinner()
	{
		for(int o = 0;o < 2;o++)
		{
			for(int i = 0;i < 3;i++)
			{
				string res;
				for(int j = 0;j < 3;j++)
				{
					int ind;
					if(o == 0) ind = 9 * i + 3 * j;
					else ind = 9 * j + 3 * i;
					res += board[ind];
				}
				if(res == "XXX") return 1;
				else if(res == "OOO") return 0;
			}
		}
		string diag1 = {board[0],board[12],board[24]};
		string diag2 = {board[6],board[12],board[18]};
		if(diag1 == "XXX" || diag2 == "XXX") return 1;
		else if(diag1 == "OOO" || diag2 == "OOO") return 0;
		else if(cnt == 9) return 3;
		return 2;
	}
	int updateBoard(int r,int c,int mv)
	{
		if(r <= 0 || r >= 4 || c <= 0 || c >= 4)
		{
			return -1;
		}
		r--;c--;
		int i = 9 * r + 3 * c;
		if(board.substr(i,2) == "__")
		{
			if(mv == 0)
			{
				board[i] = 'O';
				board[i+1] = ' ';
				cnt++;
				int r = checkWinner();
				if(r != 2) return r;
			}
			else if (mv == 1)
			{
				board[i] = 'X';
				board[i+1] = ' ';
				cnt++;
				int r = checkWinner();
				if(r != 2) return r;
			}
			else return -1;
		}
		else
		{
			return -1;
		}
		return 2;
	}
};
struct Game{
	int player1;
	int player2;
	Board *b;
	bool done;
	int curTurn;
	int player1Again;
	int player2Again;
	int gameId;
	int startTime;
	int timerNextMove;
	Game(int p1,int p2,Board *bb,int gId)
	{
		player1 = p1;
		player2 = p2;
		b = bb;
		done = false;
		curTurn = -1;
		player1Again = -1;
		player2Again = -1;
		gameId = gId;
		startTime = Clock();
	}
	void rematch(int gId)
	{
		b -> clearBoard();
		done = false;
		curTurn = -1;
		player1Again = -1;
		player2Again = -1;
		gameId = gId;
		startTime = Clock();
	}
};
struct playerDetails
{
	int playerId;
	Game *game;
	int socketSlot;
	playerDetails(int pId,int sslot)
	{
		playerId = pId;
		socketSlot = sslot;
	}
};
int sendMsg(int s,string msg)
{
	if(send(s,&msg[0],msg.size(),0) != msg.size())
	{
		cout << "SEND ERROR" << endl;
		return 0;
	}
	return 1;
}
double getTimeInterval(timespec st,timespec ed)
{
	double val = ed.tv_sec-st.tv_sec;
	val *= 1000;
	val += (ed.tv_nsec - st.tv_nsec)/1e6;
	return val;
}
queue<int> availablePlayers;
map<int,playerDetails *> playersMap;
vector<int> slotIdMap;
int main()
{
	struct timespec startTime;
	clock_gettime(CLOCK_MONOTONIC,&startTime);
	Clock = [&]()
	{
		struct timespec curTime;
		clock_gettime(CLOCK_MONOTONIC,&curTime);
		double mlsec = getTimeInterval(startTime,curTime);
		return ceil(mlsec);
	};
	ofstream file;
	file.open("log.txt");
	int server_fd,valread,new_socket;
	int opt = 1;
	int max_clients = 20,activity,sd;
	int max_sd;
	slotIdMap.assign(max_clients,-1);
	int client_sockets[max_clients];
	struct sockaddr_in address;
	int addrlen = sizeof(address);
	char buffer[4096] = {0};
	fd_set readfds;
	int playerId = 1;
	int gameId = 1;
	set<pair<int,int>> moveQueue;
	for (int i = 0;i < max_clients;i++) client_sockets[i] = 0;
	if((server_fd = socket(AF_INET,SOCK_STREAM,0)) == 0)
	{
		cout << "Failed" << endl;
		return -1;
	}
	if(setsockopt(server_fd,SOL_SOCKET,SO_REUSEADDR,(char *)&opt,sizeof(opt)) < 0)
	{
		cout << "ERROR IN setsockopt";
		return -1;
	}
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(PORT);
	if(bind(server_fd,(struct sockaddr *) &address,sizeof(address)) < 0)
	{
		cout << "Bind Failed";
		return -1;
	}
	if(listen(server_fd,3) < 0)
	{
		cout << "Listen";
		return -1;
	}
	cout << "Game server started. Waiting for players\n";
	file << "Server started at " << Clock() << "ms" << endl;
	while(1)
	{

		FD_ZERO(&readfds);
		FD_SET(server_fd,&readfds);
		max_sd = server_fd;
		for(int i = 0;i < max_clients;i++)
		{
			sd = client_sockets[i];
			if(sd > 0)
			{
				FD_SET(sd,&readfds);
			}
			if(sd > max_sd)
			{
				max_sd = sd;
			}
		}
		activity = select(max_sd + 1,&readfds,NULL,NULL,NULL);
		if((activity < 0 ) && errno != EINTR)
		{
			cout << "SELECT ERROR";
		}
		if(FD_ISSET(server_fd,&readfds))
		{
			if((new_socket = accept(server_fd,(struct sockaddr *)&address,(socklen_t*)&addrlen)) < 0)
			{
				cout << "Accept Error";
				return -1;
			}
			int curPlayer = playerId;
			playerId++;
			string id = to_string(curPlayer);
			int slot = -1;
			for(int i = 0;i < max_clients;i++)
			{
				if(client_sockets[i] == 0)
				{
					client_sockets[i] = new_socket;
					slot = i;
					break;
				}
			}
			if(slot == -1)
			{
				cout << "Players Full!! Server Overloaded";
				return -1;
			}
			playerDetails *player = new playerDetails(curPlayer,slot);
			playersMap[curPlayer] = player;
			slotIdMap[slot] = curPlayer;
			cout << "Player Joined, Assigned Id: " << id << "\n";
			file << "Player Joined, Assigned Id: " << id << " at time:  " << Clock() << "ms" << endl;
			string msg = "1 " + id;
			if(!sendMsg(new_socket,msg))
			{
				cout << "Id send Error";
			}
			sleep(1);
			msg.clear();
			if(availablePlayers.empty())
			{
				availablePlayers.push(curPlayer);
			}
			else
			{
				int partnerPlayer = availablePlayers.front();
				availablePlayers.pop();
				playerDetails * p1 = playersMap[curPlayer], *p2 = playersMap[partnerPlayer];
				Board * b = new Board();
				Game *newGame = new Game(curPlayer,partnerPlayer,b,gameId);
				p1 -> game = newGame;
				p2 -> game = newGame;
				file << "Game Id: " << gameId << " p1ID: " <<curPlayer << " p2ID: " << partnerPlayer << " Time: " << newGame -> startTime << "ms" << endl;
				int s1 = client_sockets[p1->socketSlot],s2 = client_sockets[p2->socketSlot];
				string msg1 = "2 " + to_string(partnerPlayer) + " X " + to_string(gameId) + " " + newGame->b->board;
				string msg2 = "2 " + to_string(curPlayer) + " O " + to_string(gameId) + " " + newGame-> b -> board;
				gameId++;
				newGame -> curTurn = 1;
				sendMsg(s1,msg1);
				sendMsg(s2,msg2);
				sleep(0.5);
				string msg3 = "3";
				sendMsg(s1,msg3);
				newGame -> timerNextMove = Clock() + 15000;
			}
		}
		for(int i = 0;i < max_clients;i++)
		{

			sd = client_sockets[i];
			if(FD_ISSET(sd,&readfds))
			{
				if((valread = read(sd,buffer,4096)) == 0)
				{
					getpeername(sd,(struct sockaddr*) & address,(socklen_t*)&addrlen);
					printf("Host disconnected , ip %s , port %d \n" , 
					inet_ntoa(address.sin_addr) , ntohs(address.sin_port));
					close(sd);
					client_sockets[i] = 0;
					int pId = slotIdMap[i];
					playerDetails * pDet = playersMap[pId];
					int partnerId = -1;
					if(pDet -> game ->player1 == pId)
					{
						partnerId = pDet -> game -> player2;
					}
					else
					{
						partnerId = pDet -> game -> player1;
					}
					playerDetails * partDet = playersMap[partnerId];
					int partSlot = partDet -> socketSlot;
					int sd2 = client_sockets[partSlot];
					string msg = "7";
					sendMsg(sd2,msg);
					getpeername(sd2,(struct sockaddr*) & address,(socklen_t*)&addrlen);
					printf("Manually Disconnecting Host , ip %s , port %d \n" , 
					inet_ntoa(address.sin_addr) , ntohs(address.sin_port));
					close(sd2);
					client_sockets[partSlot] = 0;
					slotIdMap[i] =  -1;
					slotIdMap[partSlot] = -1;
				}
				else
				{
					int t = Clock();
					buffer[valread] = 0;
					string msg = string(buffer);
					stringstream X(msg);
					int op;
					X >> op;
					if(op == 1)
					{
						int r,c;
						X >> r >> c;
						int pId = slotIdMap[i];
						playerDetails * pDet = playersMap[pId];
						int res = -1;
						int partnerId = -1;
						if(pDet -> game ->player1 == pId)
						{
							partnerId = pDet -> game -> player2;
							res = pDet -> game -> b -> updateBoard(r,c,1);
						}
						else
						{
							partnerId = pDet -> game -> player1;
							res = pDet -> game -> b -> updateBoard(r,c,0);
						}
						int p1 = pDet -> game -> player1;
						int p2 = pDet -> game -> player2;
						Game *game = pDet -> game;
						int tt = Clock();
						string msg1,msg2;
						int s1 = client_sockets[playersMap[p1]-> socketSlot];
						int s2 = client_sockets[playersMap[p2]-> socketSlot];
						if(game -> timerNextMove < tt)
						{
							string msg = "6 3";
							sendMsg(s1,msg);
							sendMsg(s2,msg);
							file << "Timed out Move by playerId: " << pId << " GameId: " << game -> gameId <<  endl;
							continue;
						}
						file << "Move by playerId: " << pId << " GameId: " << game -> gameId << " R C: " << r << " " << c << endl;
						if(res == 0)
						{

							msg1 = "6 2";
							msg2 = "6 1";
							file << "GameId: " << game-> gameId << " Won by PlayerId: " << p1 << endl;
						}
						else if(res == 1)
						{
							msg1 = "6 1";
							msg2 = "6 2";
							file << "GameId: " << game-> gameId << " Won by PlayerId: " << p2 << endl;
						}
						else if(res == 3)
						{
							msg1 = "6 0";
							msg2 = "6 0";
							file << "GameId: " << game-> gameId << " Draw" << endl;
						}
						else if(res == -1)
						{
							cout << "INVALID MOVE REACHED" << endl;
							string msg = "4 ";
							msg += pDet -> game -> b -> board;
							sendMsg(s1,msg);
							sendMsg(s2,msg);
							sleep(0.5);
							string msg2 = "3";
							if(p1 == pId)
							{
								sendMsg(s1,msg2);
							}
							else
							{
								sendMsg(s2,msg2);
							}
							continue;
						}
						else
						{
							string msg = "5 ";
							msg += pDet -> game -> b -> board;
							sendMsg(s1,msg);
							sendMsg(s2,msg);
							sleep(0.5);
							string msg2 = "3";
							if(p1 == partnerId)
							{
								sendMsg(s1,msg2);
							}
							else
							{
								sendMsg(s2,msg2);
							}
							pDet -> game -> timerNextMove = Clock() + 15000;
							continue;
						}
						msg1 += game -> b -> board;
						msg2 += game -> b -> board;
						sendMsg(s1,msg1);
						sendMsg(s2,msg2);
						int t = Clock();
						file << "Game Id: " << game -> gameId << " Ended at: " << t << "ms,Total Time: " << t - (game -> startTime) << endl;
					}
					else if(op == 2)
					{
						int wantsToPlay;
						X >> wantsToPlay;
						int pId = slotIdMap[i];
						playerDetails * pDet = playersMap[pId];
						Game * game = pDet -> game;
						if(game -> player1  == pId)
						{
							game -> player1Again = wantsToPlay;
						}
						else
						{
							game -> player2Again = wantsToPlay;
						}
						if(game -> player1Again == -1 || game -> player2Again == -1)
						{
							continue;
						}
						int both = (game -> player1Again) & (game -> player2Again);
						int s1 = client_sockets[playersMap[game -> player1]->socketSlot];
						int s2 = client_sockets[playersMap[game -> player2]->socketSlot];
						if(both)
						{
							game -> rematch(gameId);
							string msg1 = "2 " + to_string(game -> player1) + " X "  + to_string(gameId) + " " + game->b->board;
							string msg2 = "2 " + to_string(game -> player2) + " O " + to_string(gameId) + " " + game-> b -> board;
							game -> curTurn = 1;
							gameId++;
							sendMsg(s1,msg1);
							sendMsg(s2,msg2);
							sleep(0.5);
							string msg3 = "3";
							sendMsg(s1,msg3);
							game -> timerNextMove = Clock() + 15000;
						}
						else
						{
							string msg = "8";
							sendMsg(s1,msg);
							sendMsg(s2,msg);
						}
					}
					else
					{
						cout << "Wrong msg Recieved" << endl;
						cout << msg << endl;
					}
				}
			}
			
		}
	}
	
	return 0;
}