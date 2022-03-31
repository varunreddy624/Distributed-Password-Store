# **Distributed Password Store**
### **Introduction:** 
Centralized Password stores can never be trusted as they say they encrypt passwords but we can never trust them. Password stores like google often say that our password has been breached in online breaches or we can see that often passwords are leaked. Kepping this in account, we present a decentralized password store where there are no centralized identities whatsover. 
### **Pre requisites**
* C++ compiler with crypto library and mysql connectors install
* MYSQL database

### **Commands Required To RUN**
* Before running anything, we must start mysql database and then run the **tracker** and only then create the clients
* compile the tracker with command 
    *  `g++ tracker.cpp -o tracker -pthread`
* Run the tracker with command
    *   `./tracker tracker_info.txt`
    *   Here **tracker_info.txt** contains list of all tracker's IP and Port numbers but at present there is only one tracker (centralized) and it is passed as command line argument
* Compile the client with command
    *   `g++ -Wall -I/usr/include/cppconn client.cpp -o client -pthread -lcrypto -L/usr/lib -lmysqlcppconn`
    *   The additional statements will link the necesarry libraries required to run the client code.
* Run the client with command
    *  `/client <ipv4 address:port> tracker_info.txt <table_name> <aes_key>` 
    *  Here,
        *  **IPV4 address** should be loop back address for testing purposes  
        *  **port** should be in range [1024,49151] inclusive
        *  **table_name** is the table that particular client has access to
        *  **aes_key** is the key the client uses to encrypt the passwords and send them to other peers in the group
    * Using the same object file, we created multiple clients, changing IP addresses. 
* Supported Commands at client side
    *  Create User Account: `create_user <user_id> <passwd>`
    *  Login: `login <user_id>`
    *  Create Group: `create_group <group_id>`
    *  Join Group: `join_group <group_id>`
    *  Leave Group: `leave_group <group_id>`
    *  List pending join: `requests list_requests <group_id>`
    *  Accept Group Joining Request: `accept_request <group_id> <user_id>`
    *  List All Group In Network: `list_groups`
    *  List Particular Group Members: `list_members`
    *  Logout: `logout`
    *  Upload Password: `upload_password <user_name> <password> <domain_name>`
    *  Get password: `get_password <domain_name>`