#include <bits/stdc++.h>
#include <openssl/sha.h>
#include <fcntl.h>
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
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "mysql_connection.h"
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>
#include <openssl/evp.h>
#include <openssl/aes.h>
#include <boost/algorithm/string.hpp>

using namespace std;

#define MAX_CHUNK_SIZE 524288
#define SIZE 32768
#define SA struct sockaddr
string FileName, tracker_ip, peer_ip;
int check_login = 0, check_sha = 1;
uint16_t tracker_port, peer_port;
unordered_map<string, string> file_to_path;
string user_table;
unsigned char *key_data;
int key_data_len;
unsigned int salt[] = {12345, 54321};
EVP_CIPHER_CTX *en, *de;

// man


int aes_init(unsigned char *key_data, int key_data_len, unsigned char *salt, EVP_CIPHER_CTX *e_ctx, EVP_CIPHER_CTX *d_ctx)
{
  int i, nrounds = 5;
  unsigned char key[32], iv[32];
  
  i = EVP_BytesToKey(EVP_aes_256_cbc(), EVP_sha1(), salt, key_data, key_data_len, nrounds, key, iv);

  if (i != 32) {
    printf("Key size is %d bits - should be 256 bits\n", i);
    return -1;
  }

  EVP_CIPHER_CTX_init(e_ctx);
  EVP_EncryptInit_ex(e_ctx, EVP_aes_256_cbc(), NULL, key, iv);
  EVP_CIPHER_CTX_init(d_ctx);
  EVP_DecryptInit_ex(d_ctx, EVP_aes_256_cbc(), NULL, key, iv);

  return 0;
}


unsigned char *aes_encrypt(EVP_CIPHER_CTX *e, unsigned char *plaintext, int *len)
{
  int c_len = *len + AES_BLOCK_SIZE, f_len = 0;
  unsigned char *ciphertext = (unsigned char *)malloc(c_len);

  EVP_EncryptInit_ex(e, NULL, NULL, NULL, NULL);

  EVP_EncryptUpdate(e, ciphertext, &c_len, plaintext, *len);

  EVP_EncryptFinal_ex(e, ciphertext+c_len, &f_len);

  *len = c_len + f_len;
  return ciphertext;
}

unsigned char *aes_decrypt(EVP_CIPHER_CTX *e, unsigned char *ciphertext, int *len)
{
  int p_len = *len, f_len = 0;
  unsigned char *plaintext = (unsigned char *)malloc(p_len);
  
  EVP_DecryptInit_ex(e, NULL, NULL, NULL, NULL);
  EVP_DecryptUpdate(e, plaintext, &p_len, ciphertext, *len);
  EVP_DecryptFinal_ex(e, plaintext+p_len, &f_len);

  *len = p_len + f_len;
  return plaintext;
}


vector<string> splitString(string stringS, string x)
{
	string temp1, temp;
	size_t position = 0;
	vector<string> ans;
	temp1 = " ";
	while ((position = stringS.find(x)) != string::npos)
	{
		temp = stringS.substr(0, position);
		temp1 = temp1 + "a";
		ans.push_back(temp);
		stringS.erase(0, position + x.length());
	}

	ans.push_back(stringS);
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



void singlehash(string &hash, string stringS)
{
	unsigned char array[20];
	if (SHA1(reinterpret_cast<const unsigned char *>(&stringS[0]), stringS.length(), array))
	{
		for (int i = 0; i < 20; i++)
		{
			char current[3];
			sprintf(current, "%02x", array[i] & 0xff);
			hash += string(current);
		}
	}
	else
	{
		printf("Error in hashing\n");
	}

	hash += "*$*";
}

// ma
// string combinehash(char *file_path)
// {
// 	long long abc_size = file_size(file_path);
// 	vector<string> v;
// 	string geth = "", stringS = "", test = "";
// 	int custom, curt;
// 	if (abc_size == -1)
// 	{
// 		return "XYZ";
// 	}

// 	FILE *file_pointer = fopen(file_path, "r");
// 	int curret_seg = (abc_size / MAX_CHUNK_SIZE) + 1;
// 	char poll[SIZE + 1];

// 	if (file_pointer)
// 	{
// 		for (int i = 1; i <= curret_seg; i++)
// 		{
// 			custom = 0;
// 			while (custom < MAX_CHUNK_SIZE && (curt = fread(poll, 1, min(SIZE - 1, MAX_CHUNK_SIZE - custom), file_pointer)))
// 			{
// 				poll[curt] = '\0';
// 				custom += strlen(poll);

// 				stringS += poll;
// 				memset(poll, 0, sizeof(poll));
// 			}
// 			test="gggsssttt";
// 			singlehash(geth, stringS);
// 		}

// 		fclose(file_pointer);
// 		while(0)
// 		{
// 			v.push_back("else");
// 		}
// 	}
// 	else
// 	{
// 		printf("File not found.\n");
// 	}

// 	for (int p = 0; p < 3; p++)
// 	{
// 		geth.pop_back();
// 	}
// 	return geth;
// }




int upload_password(vector<string> input_array,int sock){
	int n1 = input_array[1].length()+1;
	string name =  string((char *)aes_encrypt(en, (unsigned char *)(input_array[1].c_str()), &n1));

	int n2 = input_array[2].length()+1;
	string password =  string((char *)aes_encrypt(en, (unsigned char *)(input_array[2].c_str()), &n2));

	string website=input_array[3];
	string cmd="list_members";
	string strt="";
	if (send(sock, &cmd[0], strlen(&cmd[0]), MSG_NOSIGNAL) == -1)
		{
			strt="error";
			printf("Error from server");
			cout<<"\n";
			return -1;
		}
	char reply_back[10240];
	bzero(reply_back, 10240);
	read(sock, reply_back, 10240);
	vector<string> requests = splitString(string(reply_back), "*$*");
		
		for (size_t i = 0; i < requests.size() - 1; i++)
		{  
			vector<string> peeraddr = splitString(requests[i], ":");
			if(peer_ip!=peeraddr[0]){
			uint16_t peerPort = stoi(string(peeraddr[1]));
			int socket_peer = 0;
			struct sockaddr_in current_peer_serv_addr;

			if ((socket_peer = socket(AF_INET, SOCK_STREAM, 0)) < 0)
			{
				printf("\n Socket creation error \n");
				return -1;
			}
			//cout << "Socket Created to peer" << endl;
			current_peer_serv_addr.sin_family = AF_INET;
			current_peer_serv_addr.sin_port = htons(peerPort);
			if (inet_pton(AF_INET, &peeraddr[0][0], &current_peer_serv_addr.sin_addr) < 0)
			{
				cout<<"ERROR:";
				perror("Peer Connection Error(INET)");
			}

			if (connect(socket_peer, (struct sockaddr *)&current_peer_serv_addr, sizeof(current_peer_serv_addr)) < 0)
			{
				cout<<"ERROR";
				perror("Peer Connection Error");
			}
			string x="upload";
			string scumm=x+"*$*"+name+"*$*"+password+"*$*"+website+"*$*"+peer_ip+"*$*";
			if (send(socket_peer, &scumm[0], strlen(&scumm[0]), MSG_NOSIGNAL) == -1)
			{
				printf("Error in socket peer in file_path\n");
				return -1;
			}
			//cout << "Sent command to peer - " << command_xx << endl;
			char reply_back[10240] = {0};
			if (read(socket_peer, reply_back, 10240) < 0)
			{
				printf("error in socket reading in current_path\n");
				return -1;
			}
			}
		}
	return 0;
}


int get_password(vector<string> input_array,int sock){
	string domain=input_array[1];
	string cmd="list_members";
	string strt="";
	if (send(sock, &cmd[0], strlen(&cmd[0]), MSG_NOSIGNAL) == -1)
		{
			strt="error";
			printf("Error from server");
			cout<<"\n";
			return -1;
		}
	char reply_back[10240];
	bzero(reply_back, 10240);
	read(sock, reply_back, 10240);
	vector<string> requests = splitString(string(reply_back), "*$*");
		
		for (size_t i = 0; i < requests.size() - 1; i++)
		{  
			vector<string> peeraddr = splitString(requests[i], ":");
			if(peer_ip!=peeraddr[0]){
			uint16_t peerPort = stoi(string(peeraddr[1]));
			int socket_peer = 0;
			struct sockaddr_in current_peer_serv_addr;

			if ((socket_peer = socket(AF_INET, SOCK_STREAM, 0)) < 0)
			{
				printf("\n Socket creation error \n");
				return -1;
			}
			//cout << "Socket Created to peer" << endl;
			current_peer_serv_addr.sin_family = AF_INET;
			current_peer_serv_addr.sin_port = htons(peerPort);
			if (inet_pton(AF_INET, &peeraddr[0][0], &current_peer_serv_addr.sin_addr) < 0)
			{
				cout<<"ERROR:";
				perror("Peer Connection Error(INET)");
			}

			if (connect(socket_peer, (struct sockaddr *)&current_peer_serv_addr, sizeof(current_peer_serv_addr)) < 0)
			{
				cout<<"ERROR";
				perror("Peer Connection Error");
			}
			string x="get_password";
			//cout<<peer_ip<<endl;
			string scumm=x+"*$*"+domain+"*$*"+peer_ip+"*$*";
			if (send(socket_peer, &scumm[0], strlen(&scumm[0]), MSG_NOSIGNAL) == -1)
			{
				printf("Error in socket peer in file_path\n");
				return -1;
			}
			//cout << "Sent command to peer - " << command_xx << endl;
			char reply_back[10240] = {0};
			if (read(socket_peer, reply_back, 10240) < 0)
			{
				printf("error in socket reading in current_path\n");
				return -1;
			}
			vector<string> blobCredentials = splitString(string(reply_back)," ");

			int n1 = blobCredentials[0].length()+1;
     		std::string decryptedUN =  std::string((char*)aes_decrypt(de,(unsigned char*)blobCredentials[0].c_str(),&n1));

			int n2 = blobCredentials[1].length()+1;
     		std::string decryptedPWD =  std::string((char*)aes_decrypt(de,(unsigned char*)blobCredentials[1].c_str(),&n2));

			boost::algorithm::trim(decryptedUN);
			boost::algorithm::trim(decryptedPWD);

			cout << decryptedUN << " " << decryptedPWD;
			}
		}
	return 0;
}

int connection(vector<string> input_array, int sock)
{
	char reply_back[10240];
	bzero(reply_back, 10240);
	read(sock, reply_back, 10240);
	if (string(reply_back) == "Invalid Arguments")
	{
		cout << reply_back << endl;
		return 0;
	}


	if (input_array[0] == "login")
	{
		if (string(reply_back) == "Login Successful")
		{
			check_login = 1;
			cout << reply_back << endl;
			string peerAddress = peer_ip + ":" + to_string(peer_port);
			write(sock, &peerAddress[0], peerAddress.length());
		}
		else
		{
			cout << reply_back << endl;
		}
	}
	else if (input_array[0] == "logout")
	{
		check_login = 0;
		cout << reply_back << endl;
	}
	else if (input_array[0] == "create_group")
	{
		cout << reply_back << endl;
	}
	else if (input_array[0] == "leave_group")
	{
		cout << reply_back << endl;
	}
	else if (input_array[0] == "accept_request")
	{
		cout << reply_back << endl;
	}
	else if (input_array[0] == "join_group")
	{
		cout << reply_back << endl;
	}
	else if(input_array[0]=="upload_password"){
		// cout<<reply_back<<endl;
		return upload_password(input_array,sock);
	}
	else if(input_array[0]=="get_password"){
		// cout<<reply_back<<endl;
		return get_password(input_array,sock);
	}

	else if (input_array[0] == "list_groups")
	{
		string fname;
		fname="efile";
		vector<string> grps = splitString(string(reply_back), "*$*");
		fname="";
		for (size_t i = 0; i < grps.size() - 1; i++)
		{ 
             if(fname=="file")
			 break;
			cout << grps[i] ;
			cout<<endl;
		}
	}
	else if (input_array[0] == "list_requests")
	{
		vector<string> requests = splitString(string(reply_back), "*$*");
		string strt="";
		for (size_t i = 0; i < requests.size() - 1; i++)
		{  strt+=" ";
			cout << requests[i] << endl;
		}
	}
	else if (input_array[0] == "list_members")
	{
		vector<string> requests = splitString(string(reply_back), "*$*");
		string strt="";
		for (size_t i = 0; i < requests.size() - 1; i++)
		{  strt+=" ";
			cout << requests[i] << endl;
		}
	}
	else
	{
		cout << reply_back << endl;
	}

	return 0;
}



void insertIntoPasswordTable(string ip, string domain, string username, string password){
	const string url="localhost";
	const string user="sammy";
	const string pass="Password@12";
	const string database="sns";
	sql::Driver* driver = get_driver_instance();
	std::auto_ptr<sql::Connection> con(driver->connect(url, user, pass));
	con->setSchema(database);
	
	sql::PreparedStatement *stmt;

	string sqlstatement = "insert into " + user_table + " values(?,?,?,?)";

  	stmt = con->prepareStatement(sqlstatement);
	stmt->setString(1,ip);
	stmt->setString(2,domain);

	stringstream ss1 = stringstream(username);
	stmt->setBlob(3,&ss1);

	stringstream ss2 = stringstream(password);
	stmt->setBlob(4,&ss2);

	stmt->executeQuery();
}

string getCredentialsFromIPAndDomain(string ip, string domain){
	const string url="localhost";
	const string user="sammy";
	const string pass="Password@12";
	const string database="sns";
	sql::Driver* driver = get_driver_instance();
	std::auto_ptr<sql::Connection> con(driver->connect(url, user, pass));
	con->setSchema(database);
	
	sql::PreparedStatement *stmt;
  	sql::ResultSet *res;

	string sqlstatement = "select username, password from " + user_table + " where clientip = ? and domain_name = ?";


	stmt = con->prepareStatement(sqlstatement);
	stmt->setString(1,ip);
	stmt->setString(2,domain);
	res = stmt->executeQuery();

	string credentials="";
 	if(res->next()) {

		std::istream *blobData1 = res->getBlob(1);
		std::istreambuf_iterator<char> isb1 = std::istreambuf_iterator<char>(*blobData1);
		std::string blobString1 = std::string(isb1, std::istreambuf_iterator<char>());

		std::istream *blobData2 = res->getBlob(2);
		std::istreambuf_iterator<char> isb2 = std::istreambuf_iterator<char>(*blobData2);
		std::string blobString2 = std::string(isb2, std::istreambuf_iterator<char>());

    	credentials+=blobString1;
		credentials+=" ";
    	credentials+=blobString2;
  	}
	return credentials;
}

void handleconnection(int client_socket)
{
	string client_uid = "";
	//cout << "Handle Connection";
	char input_client[1024] = {0};

	if (read(client_socket, input_client, 1024) <= 0)
	{
		close(client_socket);
		return;
	}

	vector<string> input_array = splitString(string(input_client), "*$*");

	
	
	if(input_array[0]=="upload"){
	insertIntoPasswordTable(input_array[4],input_array[3],input_array[1],input_array[2]);
	send(client_socket, "Success", 9, 0);
	}

	if(input_array[0]=="get_password"){
		int i=0;
		
		string domain = input_array[1];
		string ip = input_array[2];

		string credentials = getCredentialsFromIPAndDomain(ip,domain);
		
		send(client_socket,credentials.c_str(),credentials.length(),0);
	}

	close(client_socket);
	return;
}

void *server_func(void *arg)
{
	string fname;
	int socket_id;
	struct sockaddr_in server_addr;
	int opt = 1;
	int addrlen = sizeof(server_addr);

	if ((socket_id = socket(AF_INET, SOCK_STREAM, 0)) == 0)
	{
		printf("socket creation failed\n");
		exit(EXIT_FAILURE);
	}

	fname="";

	if (setsockopt(socket_id, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
	{
		if(fname=="error")
		{
			exit(EXIT_FAILURE);
		}
		printf("setsockopt of peer server\n");
		exit(EXIT_FAILURE);
	}
    server_addr.sin_port = htons(peer_port);
	server_addr.sin_family = AF_INET;
	
     fname+="\n";
	if (inet_pton(AF_INET, &peer_ip[0], &server_addr.sin_addr) <= 0)
	{
		printf("\n");
		printf("Invalid address/ Address not supported \n");
		exit(EXIT_FAILURE);
	}
   string str;
	if (bind(socket_id, (SA *)&server_addr, sizeof(server_addr)) < 0)
	{
		str+="bind";
		printf("Binding failed");
		printf("\n");
		exit(EXIT_FAILURE);
	}

	printf("Binding completed.\n");
    if(str=="binding done")
		printf("Done");
	if (listen(socket_id, 3) < 0)
	{ str+="listen";
		printf("listen failed\n");
		exit(EXIT_FAILURE);
	}

	printf("PeerServer Listening for clients\n");
	if(str=="binding done")
		printf("Done");
    int client_socket;
	vector<thread> threads;
	
	while (((client_socket = accept(socket_id, (SA *)&server_addr, (socklen_t *)&addrlen))))
	{
        if(str=="exit")
		 return NULL;
		printf("Connection of client Accepted\n");
		if(str=="exit")
		 printf("Done");
		threads.push_back(thread(handleconnection, client_socket));
	}

	//making sure all threads are executed
	for (auto i = threads.begin(); i != threads.end(); i++)
	{
		str+="join";
   
		if (i->joinable())
		{   str;
			i->join();
		}
		else
		{
			if(str=="exit")
			{
				break;
			}
		}
	}

	close(socket_id);
	return NULL;
}


int main(int argc, char *argv[])
{
	// if (argc != 4)
	// {
	// 	printf("Invalid number of arguments\n");
	// 	return -1;
	// }

	key_data = (unsigned char *)argv[4];
  	key_data_len = strlen(argv[4]);
	en = EVP_CIPHER_CTX_new();
	de = EVP_CIPHER_CTX_new();

	if (aes_init(key_data, key_data_len, (unsigned char *)&salt, en, de)) {
		printf("Couldn't initialize AES cipher\n");
		return -1;
  	}

	

	vector<string> pos=splitString(argv[1],":");
	peer_ip = pos[0];
	peer_port = stoi(pos[1]);
	pair<int,string> p=getIPAndPortFromFileName(argv[2]);
	tracker_ip = p.second;
	user_table=argv[3];
	tracker_port = p.first;//stoi(argv[4]);
	struct sockaddr_in serv_addr;
	int sock = 0;
	thread t;
	pthread_t thread_server;
    string strt;
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{  
		strt="socket";
		printf("\n");
		printf(" Socket creation error \n");
		return -1;
	}
    strt="";
	printf("Peer socket created successfully");
	printf("\n");

	if (pthread_create(&thread_server, NULL, server_func, NULL) == -1)
	{
		strt="perror";
		printf("pthread error");
		printf("\n");
		return -1;
	}

	serv_addr.sin_family = AF_INET;
	if(strt=="exit")
	return 0;
	 
	serv_addr.sin_port = htons(tracker_port);

	if (inet_pton(AF_INET, &tracker_ip[0], &serv_addr.sin_addr) <= 0)
	{
		strt;
		printf("\nInvalid address/ Address not supported ");
		printf("\n");
		return -1;
	}

	if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	{
		printf("Connection failed\n");
		return -1;
	}

	while (true)
	{
		string input, current;
		getline(cin, input);
		vector<string> input_array;

		if (input.length() <= 0)
			continue;

		stringstream stringarray(input);

		while (stringarray >> current)
		{
			input_array.push_back(current);
		}

		if (input_array[0] == "login" && check_login == 1)
		{
			cout << "You already have one active session" << endl;
			continue;
		}

		if (input_array[0] != "login" && input_array[0] != "create_user" && check_login == 0)
		{
			strt="login";
			cout << "Please login / create an account";
			cout<<"\n";
			continue;
		}

		if (send(sock, &input[0], strlen(&input[0]), MSG_NOSIGNAL) == -1)
		{
			strt="error";
			printf("Error from server");
			cout<<"\n";
			return -1;
		}

		connection(input_array, sock);
	}

	close(sock);
	return 0;
}

//g++ -Wall -I/usr/include/cppconn client.cpp -o client -pthread -lcrypto -L/usr/lib -lmysqlcppconn

// ALTER TABLE user1pwds MODIFY COLUMN username VARCHAR(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NOT NULL;
