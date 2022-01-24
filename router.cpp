#include <cstdio>
#include <cstring>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <iostream>
#include <cstdlib>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sstream>
#include <fstream>
#include <time.h>
#include <math.h>
#include <cmath>
#include <netdb.h>
#include <sys/time.h>
#include <map>
#include <arpa/inet.h>
#include <pthread.h>
#include <vector>
#include <string>
#include <thread> 
#define infAsStr "99999"
#define infAsInt 99999
using namespace std;

struct sockaddr_in router_address;

struct routingTableEntry
{
	string dest;
	string nexthop;
	string cost;
};
typedef routingTableEntry rtentry;
vector<rtentry> routingTable;

int isAlreadyInTable(string ip){
	int size=routingTable.size();
	for (int i = 0 ; i < size ; i++){  
		if(routingTable[i].dest==ip){
			return i;
		}
	}
	return -1;
}

void readTopology(const char* fileName){
	ifstream infAsStrile(fileName);
	string line,ip1,ip2,cost;
	rtentry temp;
	string thisRouterAddress=inet_ntoa(router_address.sin_addr);

	while (getline(infAsStrile, line))
	{
		istringstream iss(line);
		int pos;
		if (!(iss >> ip1 >> ip2 >> cost)) { break; } // error
		
		if(ip1==thisRouterAddress){
			pos=isAlreadyInTable(ip2);
			if(pos==-1){
				temp.dest=ip2;
				temp.nexthop=ip2;
				temp.cost=cost;
				routingTable.push_back(temp);
			}
			else{
				routingTable[pos].nexthop=ip2;
				routingTable[pos].cost=cost;
			}
			
		}
		else if(ip2==thisRouterAddress){
			pos=isAlreadyInTable(ip1);
			if(pos==-1){
				temp.dest=ip1;
				temp.nexthop=ip1;
				temp.cost=cost;
				routingTable.push_back(temp);
			}
			else{
				routingTable[pos].nexthop=ip1;
				routingTable[pos].cost=cost;
			}
		}
		else{
			if(isAlreadyInTable(ip1)==-1){
				temp.dest=ip1;
				temp.nexthop="-";
				temp.cost=infAsStr;
				routingTable.push_back(temp);
			}
			if(isAlreadyInTable(ip2)==-1){
				temp.dest=ip2;
				temp.nexthop="-";
				temp.cost=infAsStr;
				routingTable.push_back(temp);
			}
		}
	}
}


void printRoutingTable(){
	int size=routingTable.size();
	cout<<"destination\tnext hop\tcost\n";
	cout<<"-----------\t--------\t----\n";
	for (int i = 0 ; i < size ; i++){  
		cout<<routingTable[i].dest<<"\t"<<routingTable[i].nexthop<<"\t"<<routingTable[i].cost<<endl;
	}
}

void sendTableToNeighbours(int sockfd){
	char buffer[1024];
	string rTableNetwork;
	stringstream ss;
	int size=routingTable.size();
	struct sockaddr_in neighbour_address;
	for (int i = 0 ; i < size ; i++){  
		ss<<routingTable[i].dest<<"$"<<routingTable[i].nexthop<<"$"<<routingTable[i].cost<<"$";
	}
	ss>>rTableNetwork;
   	strcpy(buffer, rTableNetwork.c_str());
	
	for (int i = 0 ; i < size ; i++){  

		neighbour_address.sin_addr.s_addr = inet_addr(routingTable[i].dest.c_str());
		sendto(sockfd, buffer, 1024, 0, (struct sockaddr*) &neighbour_address, sizeof(sockaddr_in));
	}
	memset(buffer, 0, sizeof buffer);
}

bool isValidTable(string neighbourAddress,string buffer){
	stringstream check1(buffer);
	string intermediate;
	string init=buffer.substr(0,3);
	int i=0;
	while(getline(check1, intermediate, '$')) 
	{	if(i==0){
			i++;
			if(neighbourAddress==intermediate) return false;
		}
		else if(i==1){
			i++;
		}
		else{
			i=0;
		}
	}
	return true;
}

void updateRoutingTable(string neighbourAddress,string buffer){
	stringstream check1(buffer);
	string intermediate;
	string init=buffer.substr(0,3);
	string thisRouterAddress=inet_ntoa(router_address.sin_addr);
	int i=0,nbrPosInRtable=-1;
	int costThroughNbr;
	int costFThisRtrTSndrNbr=0,costFThisRtrTOthrNbr=0,costFSndrNbrTOthrNbr=0;
	int rTableSize=routingTable.size();
	rtentry temp;
	
	if(init.compare("clk")!=0){
		if(!isValidTable(neighbourAddress,buffer)) return;
		for(int j=0;j<rTableSize;j++){
			if(routingTable[j].dest==neighbourAddress){
				nbrPosInRtable=j;
				costFThisRtrTSndrNbr = atoi(routingTable[j].cost.c_str());
				break;
			}
		}
		while(getline(check1, intermediate, '$')) 
		{	if(i==0){
				temp.dest = intermediate;
				i++;
			}
			else if(i==1){
				temp.nexthop = intermediate;
				i++;
			}
			else{
				i=0;
				temp.cost = intermediate;
				if(thisRouterAddress!=temp.dest){
					costFSndrNbrTOthrNbr = atoi(temp.cost.c_str());
					for(int j=0;j<rTableSize;j++){
						if((j!=nbrPosInRtable)&&(routingTable[j].dest==temp.dest)){
							costFThisRtrTSndrNbr = atoi(routingTable[nbrPosInRtable].cost.c_str());
							costFThisRtrTOthrNbr = atoi(routingTable[j].cost.c_str());
							costThroughNbr=costFThisRtrTSndrNbr+costFSndrNbrTOthrNbr;
							if(costThroughNbr<costFThisRtrTOthrNbr){
								routingTable[j].nexthop=routingTable[nbrPosInRtable].nexthop;
								routingTable[j].cost=to_string(costThroughNbr);
								printRoutingTable();
							}
						}
					}
				}
			}		
		}
	}
}


int findDestIpInTable(string ip){
	int rTableSize=routingTable.size();
	for(int j=0;j<rTableSize;j++){
		if(routingTable[j].dest==ip){
			return j;
		}
	}
}

void impfrwd(char* buffer,int sockfd){
	stringstream check1(buffer);
	int msgLength;
	string myIp=inet_ntoa(router_address.sin_addr);
	//char msg[msgLength];
	string intermediate,msg;
	int i=0;
	string dest;
	int posOfDest=0;
	string frwdmsg;
	stringstream ss;
	char frwdmsgBuffer[1024];
	struct sockaddr_in nhop_address;
	nhop_address.sin_family = AF_INET;
	nhop_address.sin_port = htons(4747);

	while(getline(check1, intermediate, '$')){
		if(i==0) i++;
		else if(i==1){
			dest=intermediate;
			i++;
		}
		else if(i==2){
			msgLength=atoi(intermediate.c_str());;
			i++;
		}
		else if(i==3){
			msg=intermediate;
			i++;
		}
	}
	
	if(myIp==dest){
		cout<<msg<<" packet reached destination (printed by "<<myIp<<")\n";
	}
	else{
		posOfDest=findDestIpInTable(dest);

		nhop_address.sin_addr.s_addr = inet_addr(routingTable[posOfDest].nexthop.c_str());
		int bytes_sent=sendto(sockfd, frwdmsgBuffer, 1024, 0, (struct sockaddr*) &nhop_address, sizeof(sockaddr_in));
		cout<<bytes_sent<<endl;
		cout<<"forwarded to "<<routingTable[posOfDest].nexthop<<" (printed by "<<myIp<<")\n";
	}
	
}



void recvFromNeighbour(int sockfd,string neighbourAddress){
	int bytes_received;
	char buffer[1024];
	socklen_t addrlen;
	struct sockaddr_in sender_address;
	sender_address.sin_family = AF_INET;
	sender_address.sin_port = htons(4747);
	sender_address.sin_addr.s_addr = inet_addr(neighbourAddress.c_str());
	bytes_received = recvfrom(sockfd, buffer, 1024, 0, (struct sockaddr*) &sender_address, &addrlen);
	cout<<buffer<<endl;
	if(strncmp(buffer,"frwd",4)==0){ 
		impfrwd(buffer,sockfd);
	}
	else{
		updateRoutingTable(neighbourAddress,string(buffer));
	}
	memset(buffer, 0, sizeof buffer);
}
void updateCost(char* buffer){
	
	char ip1[20], ip2[20] ;
	int cost;
	sprintf(ip1,"%d.%d.%d.%d",(unsigned char) buffer[4],(unsigned char) buffer[5],(unsigned char) buffer[6],(unsigned char) buffer[7]);
	sprintf(ip2,"%d.%d.%d.%d",(unsigned char) buffer[8],(unsigned char) buffer[9],(unsigned char) buffer[10],(unsigned char) buffer[11]);
	cost =  (int) (((unsigned char)buffer[13])<<8) + (int) ((unsigned char)buffer[12]) ;
}

void impSend(char* buffer,int sockfd){
	char ip1[20], ip2[20] ;
	int msgLength;
	string myIp=inet_ntoa(router_address.sin_addr);

	sprintf(ip1,"%d.%d.%d.%d",(unsigned char) buffer[4],(unsigned char) buffer[5],(unsigned char) buffer[6],(unsigned char) buffer[7]);
	sprintf(ip2,"%d.%d.%d.%d",(unsigned char) buffer[8],(unsigned char) buffer[9],(unsigned char) buffer[10],(unsigned char) buffer[11]);
	msgLength =  (int) (((unsigned char)buffer[13])<<8) + (int) ((unsigned char)buffer[12]) ;
	char msg[msgLength];

	int posOfDest=0;
	string frwdmsg;
	stringstream ss;
	char frwdmsgBuffer[1024];
	struct sockaddr_in nhop_address;
	nhop_address.sin_family = AF_INET;
	nhop_address.sin_port = htons(4747);
	
	for(int i=0;i<msgLength;i++){
		msg[i]=(unsigned char) buffer[14+i];
	}
	

	
	if(myIp==string(ip2)){
		cout<<msg<<" packet reached destination (printed by "<<myIp<<")\n";
	}
	else{
		posOfDest=findDestIpInTable(string(ip2));
		ss<<"frwd"<<"$"<<string(ip2)<<"$"<<msgLength<<"$"<<msg<<"$";
		ss>>frwdmsg;

		strcpy(frwdmsgBuffer, frwdmsg.c_str());
	
		nhop_address.sin_addr.s_addr = inet_addr(routingTable[posOfDest].nexthop.c_str());
		//sendto(sockfd, frwdmsgBuffer, 1024, 0, (struct sockaddr*) &nhop_address, sizeof(sockaddr_in));
		
		int bytes_sent=sendto(sockfd, frwdmsgBuffer, 1024, 0, (struct sockaddr*) &nhop_address, sizeof(sockaddr_in));
		cout<<bytes_sent<<endl;	
	
		cout<<"forwarded to "<<routingTable[posOfDest].nexthop<<" (printed by "<<myIp<<")\n";
	}
	
}


int main(int argc, char *argv[]){
	int sockfd; 
	int bind_flag;
	int bytes_received;
	char buffer[1024];
	int size;
	struct sockaddr_in driver_address;
	socklen_t addrlen;
	

	if(argc != 3){
		printf("%s <ip address> <topo.txt>\n", argv[0]);
		exit(1);
	}

	router_address.sin_family = AF_INET;
	router_address.sin_port = htons(4747);
	router_address.sin_addr.s_addr = inet_addr(argv[1]);

	driver_address.sin_family = AF_INET;
	driver_address.sin_port = htons(4747);
	driver_address.sin_addr.s_addr = inet_addr("192.168.10.100");

	readTopology(argv[2]);
	printRoutingTable();
	
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	bind_flag = bind(sockfd, (struct sockaddr*) &router_address, sizeof(sockaddr_in));
	
	//std::thread first (recvFromNeighbour,sockfd,routingTable[0].dest);
	//std::thread second (recvFromNeighbour,sockfd,routingTable[1].dest);
	//std::thread third (recvFromNeighbour,sockfd,routingTable[2].dest);

	while(true){
		size=routingTable.size();
		for(int i=0;i<size;i++)
			recvFromNeighbour(sockfd,routingTable[i].dest);
		bytes_received = recvfrom(sockfd, buffer, 1024, 0, (struct sockaddr*) &driver_address, &addrlen);
		//cout<<inet_ntoa(driver_address.sin_addr)<<" ";
		//cout<<ntohs(driver_address.sin_port)<<" "<<buffer<<endl;
		if(strncmp(buffer,"clk",3)==0) 
			sendTableToNeighbours(sockfd);
		if(strncmp(buffer,"cost",4)==0) 
			updateCost(buffer);
		if(strncmp(buffer,"send",4)==0) 
			impSend(buffer,sockfd);
		
		memset(buffer, 0, sizeof buffer);
		
	}

	close(sockfd);

	return 0;
}
