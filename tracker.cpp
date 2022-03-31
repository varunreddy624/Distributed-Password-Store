#include <bits/stdc++.h>
#include <stdarg.h>
#include <sys/time.h>
#include <sys/types.h>
#include <signal.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include <sys/types.h>
#include <pthread.h>
#include <openssl/sha.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#define SA struct sockaddr
using namespace std;

string tracker_ip, uploadFile;
uint16_t tracker_port;
unordered_map<string, set < string>> grp_members;
unordered_map<string, set < string>> grp_requests;
unordered_map<string, string> client_group;
unordered_map<string, string> login;
unordered_map<string, bool> checklogin;

unordered_map<string, string> convert_port;
unordered_map<string, string> piece_wise;
unordered_map<string, string> cur_file_size;
unordered_map<string, string> admin_groups;
vector<string> group_list;



bool ispath(const string &stringS)
{
	struct stat itr;
	if (stat(stringS.c_str(), &itr) == 0)
	{
		return true;
	}
	else
	{
		return false;
	}
}

vector<string> splitString(string stringS, string x)
{
	string temp;
	size_t position = 0;
	vector<string> ans;
	string strt="";
	while ((position = stringS.find(x)) != string::npos)
	{
		while(0)
		{
			strt+="splitstring";
		}
		temp = stringS.substr(0, position);
		ans.push_back(temp);
		strt;
		stringS.erase(0, position + x.length());

		if(strt=="exit")
		{
			vector<string> res;
			return res;
		}
	}

	ans.push_back(stringS);
	strt;
	return ans;
}

pair<int,string> getIPAndPortFromFileName(char* fn){
	string filename = string(fn);
    string line="";
    ifstream input_file(filename);

    if (input_file.is_open()) {
		getline(input_file, line);
		vector<string> trackerIP = splitString(line,":");
		string i="port";
		string ip=trackerIP[0];
		int port= stoi(trackerIP[1]);
		string make_pai="pair";
		return make_pair(port,ip);
        
    }

	else{
		
		printf("Cannot open the file");
		return make_pair(0,"0.0.0.0");
	}
}

void *close_server(void *arg)
{
	while (true)
	{
		string quit;
		getline(cin, quit);
		if (quit == "quit")
		{
			exit(0);
		}
	}
}

unordered_map<string, unordered_map<string, set< string>>> uploadList;

int check_login_validate(string user_id, string password)
{
	if (login.find(user_id) == login.end())
		return -1;

	if (login[user_id] != password)
		return -1;

	if (checklogin.find(user_id) == checklogin.end())
	{
		checklogin.insert({ user_id, true });
		return 0;
	}
	else
	{
		if (checklogin[user_id] == false)
		{
			checklogin[user_id] = true;
			return 0;
		}
		else
		{
			return 1;
		}
	}

	return 0;
}




//connection of clients using threads and handle commands
void connection(int client_socket)
{
	string client_id, client_group_id, current, input;
	cout << "Thread created" << to_string(client_socket) << "\n";

	while (true)
	{
		char buffer[2048] = { 0 };
		vector<string> input_array;
		if (read(client_socket, buffer, 2048) <= 0)
		{
			checklogin[client_id] = false;
			close(client_socket);
			break;
		}

		input = string(buffer);
		cout << "Request from client" << input << "\n";
		stringstream stringarray(input);

		while (stringarray >> current)
		{
			input_array.push_back(current);
		}

		if (input_array[0] == "create_user")
		{
			if (input_array.size() == 3)
			{
				string user_id=input_array[1];
				string password=input_array[2];
				int res;
				if (login.find(user_id) == login.end())
				{
					login.insert({ user_id, password });
					res = 0;
				}
				else
				{
					res = -1;
				}

				if (res == -1)
				{
					write(client_socket, "User already exists", 20);
				}
				else
				{
					write(client_socket, "Account Sucessfully Created", 28);
				}
			}
			else
			{
				write(client_socket, "Invalid Arguments", 18);
			}
		}
		else if (input_array[0] == "login")
		{
			if (input_array.size() == 3)
			{
				char login_buffer[1024];
				string cur_client_address;
				client_id = input_array[1];
				int res = check_login_validate(input_array[1], input_array[2]);
				if (res == 1)
				{
					write(client_socket, "User has already logged in", 28);
				}
				else if (res == 0)
				{
					write(client_socket, "Login Successful", 17);
					read(client_socket, login_buffer, 1024);
					cur_client_address = string(login_buffer);
					convert_port[client_id] = cur_client_address;
				}
				else
				{
					write(client_socket, "User_id/Password is incorrect", 30);
				}
			}
			else
			{
				write(client_socket, "Invalid Arguments", 18);
			}
		}
		else if (input_array[0] == "logout")
		{
			checklogin[client_id] = false;
			write(client_socket, "Logout Successful", 20);
		}
		else if (input_array[0] == "accept_request")
		{
			if (input_array.size() != 3)
			{
				write(client_socket, "Invalid Arguments", 18);
			}

			else{string group_id = input_array[1];
			string user_id = input_array[2];
			if (admin_groups.find(group_id) == admin_groups.end())
			{
				write(client_socket, "No group found", 13);
			}
			else if (admin_groups.find(group_id)->second != client_id)
			{
				write(client_socket, "You are not admin", 17);
			}
			else if(grp_requests[group_id].find(user_id)==grp_requests[group_id].end())
			{
				write(client_socket,"Group request is not sent yet",50);
			}
			else
			{
				write(client_socket, "Request accepted.", 18);
				grp_members[group_id].insert(user_id);
				grp_requests[group_id].erase(user_id);
				client_group[user_id]=group_id;
			}
			}
		}
		else if (input_array[0] == "create_group")
		{
			if (input_array.size() != 2)
			{
				write(client_socket, "Invalid Arguments", 18);
			}

			else{string group_id = input_array[1];
			int res=0;
			for (string i: group_list)
			{
				if (i == group_id)
				{
					write(client_socket, "Group Already Exists", 21);
					res=1;
				}
			}

			if(res==0){group_list.push_back(group_id);
			admin_groups.insert({ group_id, client_id });
			grp_members[group_id].insert(client_id);
			client_group[client_id]=group_id;
			client_group_id = input_array[1];
			write(client_socket, "Group Successfully Created", 35);
			}
			}

		}
		else if (input_array[0] == "join_group")
		{
			if (input_array.size() != 2)
			{
				write(client_socket, "Invalid Arguments", 18);
			}

			else{string group_id = input_array[1];
			if (admin_groups.find(group_id) == admin_groups.end())
			{
				write(client_socket, "Wrong Group ID", 14);
			}
			else if (grp_members[group_id].find(client_id) != grp_members[group_id].end())
			{
				write(client_socket, "You are already present in group", 32);
			}
			else
			{
				if(client_group.find(client_id)==client_group.end())
				{grp_requests[group_id].insert(client_id);
				write(client_socket, "Group request successfully sent", 31);}
				else{
					write(client_socket, "Already present in another group", 40);
				}
			}
			}
		}
		else if(input_array[0] == "list_members"){
			//cout << client_id << endl;
			//cout<<client_group[client_id] << endl;
			string cur_client_group=client_group[client_id];
			string res="";
			for(auto itr: grp_members[cur_client_group]){
				res += convert_port[itr]+"*$*";
			}
			write(client_socket, &res[0], res.length());

		}
		else if (input_array[0] == "list_requests")
		{
			if (input_array.size() != 2)
			{
				write(client_socket, "Invalid Arguments", 18);
			}

			else{string group_id = input_array[1];
			string requests = "";

			if (admin_groups.find(group_id) == admin_groups.end())
			{
				write(client_socket, "No group found*$*", 18);
			}

			if (admin_groups[group_id] != client_id)
			{
				write(client_socket, "You are not admin*$*", 20);
			}
			else if (grp_requests[group_id].size() == 0)
			{
				write(client_socket, "No requests*$*", 15);
			}
			else
			{
				for (auto i = grp_requests[group_id].begin(); i != grp_requests[group_id].end(); i++)
				{
					requests += string(*i) + "*$*";
				}

				write(client_socket, &requests[0], requests.length());
			}
			}
		}
		else if (input_array[0] == "leave_group")
		{
			if (input_array.size() != 2)
			{
				write(client_socket, "Invalid Arguments", 18);
			}
			else{
			string group_id=input_array[1];
			if (admin_groups.find(group_id) == admin_groups.end())
			{
				write(client_socket, "No group found", 14);
			}
			
			else if (grp_members[group_id].find(client_id) != grp_members[group_id].end())
			{
				int res=0;
				if (admin_groups[group_id]!= client_id)
				{
				grp_members[group_id].erase(client_id);
				client_group.erase(client_id);
				write(client_socket, "Group left succesfully", 23);
				int vart=0;
				}
				else{
				write(client_socket, "You are admin, you cannot leave", 35);
				}
			}
			else
			{
				write(client_socket, "You are not part of group", 25);
			}
			}
		}
		else if (input_array[0] == "list_groups")
		{  
			if (input_array.size() != 1)
			{
			
				write(client_socket, "Invalid Arguments", 18);

			}

			else if (group_list.size() == 0)
			{
				write(client_socket, "No groups avail*$*", 18);
			}

			else{string groups = "";
			for (int i = 0; i < group_list.size(); i++)
			{  
				groups += group_list[i] + "*$*";
			}


			write(client_socket, &groups[0], groups.length() + 1);
			}

		}
		
		else
		{
			write(client_socket, "Invalid Command", 16);
		}
	}

	close(client_socket);
}

int main(int argc, char *argv[])
{
	int socket_id;
	struct sockaddr_in server_addr;
	//tracker_ip = argv[1];
	pthread_t close_thread;
	pair<int,string> p=getIPAndPortFromFileName(argv[1]);
	tracker_ip = p.second;
	tracker_port = p.first;//stoi(argv[4]);
	//tracker_port = stoi(argv[2]);
	int opt = 1;
	int addrlen = sizeof(server_addr);

	if ((socket_id = socket(AF_INET, SOCK_STREAM, 0)) == 0)
	{
		printf("socket creation failed\n");
		return -1;
	}

	cout << "Tracker socket created.\n";

	if (setsockopt(socket_id, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
	{
		printf("setsockopt of tracker\n");
		return -1;
	}

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(tracker_port);
	if (inet_pton(AF_INET, &tracker_ip[0], &server_addr.sin_addr) <= 0)
	{
		printf("\nInvalid address/ Address not supported \n");
		return -1;
	}
	if (bind(socket_id, (SA*) &server_addr, sizeof(server_addr)) < 0)
	{
		printf("Binding failed\n");
		return -1;
	}


	printf("\n");	printf("Binding completed.\n");


	if (listen(socket_id, 5) < 0)
	{
		printf("listen failed\n");
		return -1;
	}

	printf("Tracker Listening for clients\n");

	vector<thread> threads;

	//checks for command quit
	if (pthread_create(&close_thread, NULL, close_server, NULL) == -1)
	{
		printf("pthread creation error\n");
		return -1;
	}

	while (true)
	{
		int client_socket;

		if ((client_socket = accept(socket_id, (SA*) &server_addr, (socklen_t*) &addrlen)) < 0)
		{
			printf("Acceptance error during connection of client\n");
		}
       printf("\n");
	   string str;
		printf("Connection of client Accepted");
		printf("\n");

		//adding all clients to tracker in while loop
		threads.push_back(thread(connection, client_socket));
	}

	if(false)
	{
		cout<<"error"<<endl;
	}

	//making sure all threads are executed
	for (auto i = threads.begin(); i != threads.end(); i++)
	{
		string strt="";
		strt+=" ";
		if (i->joinable())
		{
			strt;
			i->join();
		}
		else
		{
			strt+="";
		}
	}

	printf("Exit\n");
	return 0;
}
