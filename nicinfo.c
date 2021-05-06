#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <time.h>
#include <pthread.h>


#ifdef _WIN32

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>


#else
	
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <netdb.h> 
#include <ifaddrs.h>

#endif

#include "m_pd.h"





typedef struct _nicinfo {
  t_object  x_obj;
  t_canvas  *x_canvas;
  pthread_t tid;
   
  } t_nicinfo;





t_class *nicinfo_class;

static void ipify(t_nicinfo *x);
static void getnic (t_nicinfo *x);



static void getnic (t_nicinfo *x) {
	
	
logpost(x,2,"****************************************");
logpost(x,2,"Network adapters and their ip addresses:");
logpost(x,2," ");

 

#ifdef _WIN32

    PIP_ADAPTER_INFO pAdapterInfo;
    PIP_ADAPTER_INFO pAdapter = NULL;
    DWORD dwRetVal = 0;
    

    ULONG ulOutBufLen = sizeof (IP_ADAPTER_INFO);
    pAdapterInfo = (IP_ADAPTER_INFO *) malloc(sizeof (IP_ADAPTER_INFO));
    if (pAdapterInfo == NULL) {
        printf("Error allocating memory needed to call GetAdaptersinfo\n");
        return;
    }
// Make an initial call to GetAdaptersInfo to get
// the necessary size into the ulOutBufLen variable
    if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW) {
        free(pAdapterInfo);
        pAdapterInfo = (IP_ADAPTER_INFO *) malloc(ulOutBufLen);
        if (pAdapterInfo == NULL) {
            printf("Error allocating memory needed to call GetAdaptersinfo\n");
            return;
        }
    }

    if ((dwRetVal = GetAdaptersInfo(pAdapterInfo, &ulOutBufLen)) == NO_ERROR) {
        pAdapter = pAdapterInfo;
        while (pAdapter) {
           
            logpost(x,2,"Adapter: \t%s", pAdapter->Description);            

            logpost(x,2,"IP Address: \t%s", pAdapter->IpAddressList.IpAddress.String);
            logpost(x,2,"IP Mask: \t%s", pAdapter->IpAddressList.IpMask.String);

            logpost(x,2,"Gateway: \t%s", pAdapter->GatewayList.IpAddress.String);
            logpost(x,2," ");
			pAdapter = pAdapter->Next;
		}
	}
    if (pAdapterInfo)
        free(pAdapterInfo);

    return;



    
#else




	
struct ifaddrs *addresses;
if (getifaddrs(&addresses) == -1)
  {
    logpost(x,2,"getifaddrs call failed\n");
    return;
  }

struct ifaddrs *address = addresses;

char concat[256] = {'\0'};
char buff[256] = {'\0'};

while(address) 
{
  int family = address->ifa_addr->sa_family;
  if (family == AF_INET || family == AF_INET6)
  {
    
    sprintf(buff,"%s\t",address->ifa_name);
    strcat(concat,buff);
    sprintf(buff,"%s\t",family == AF_INET ? "IPv4" : "IPv6");
    strcat(concat,buff);
    char ap[100];
    const int family_size = family == AF_INET ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6);
    getnameinfo(address->ifa_addr,family_size, ap, sizeof(ap), 0, 0, NI_NUMERICHOST);
    strcat(concat,ap);
    logpost(x,2,"%s", concat);
    memset(buff, 0 ,sizeof(buff));
    memset(concat, 0 ,sizeof(buff));	
  }
  address = address->ifa_next;
  }
  freeifaddrs(addresses);
return;



	
#endif

}


static void ipify(t_nicinfo *x) {
	
int portno = 80;
char *host = "ipify.org";

struct hostent *server;
struct sockaddr_in serv_addr;
int sockfd, bytes, sent, received, total;
char *message;

char response[2048];	
	
	#ifdef _WIN32
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2,2),&wsa) != 0)
	{
		printf("Failed. Error Code : %d",WSAGetLastError());		
	}
	#endif
	
	
	
	message=malloc(30);

	sprintf(message,"GET /\r\n HTTP/1.0\r\n");
	
	

	/* create the socket */
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
				printf("ERROR opening socket\n");
				return;}

	/* lookup the ip address */
	server = gethostbyname(host);
	if (server == NULL){ 
				printf("ERROR, no such host (do we have internet?)\n");
				return;}
	
	/* fill in the structure */
	memset(&serv_addr,0,sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portno);
	memcpy(&serv_addr.sin_addr.s_addr,server->h_addr,server->h_length);

	/* connect the socket */
	if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0) {
				printf("ERROR connecting\n");
				return;}
	
	total = strlen(message);
	
	sent = 0;
	do {
        
		bytes = send(sockfd,message+sent,total-sent, 0);
        if (bytes < 0){
				printf("ERROR writing message to socket\n");
				return;}
		if (bytes == 0)
			break;
		sent+=bytes;
	} while (sent < total);

    /* receive the response */
    memset(response,0,sizeof(response));
    total = sizeof(response)-1;
    received = 0;
    do {
        bytes = recv(sockfd,response+received,total-received, 0);
        if (bytes < 0){
				printf("ERROR reading response from socket\n");
				return;}
		if (bytes == 0)
			break;
		received+=bytes;
	} while (received < total);

    if (received == total) {
        printf("ERROR storing complete response from socket\n");
		return;}

	
	/* close the socket */
	#ifdef _WIN32
    closesocket(sockfd);
	#else
	close(sockfd);
	#endif

    /* process response */
    
    logpost(x,2," ");
    logpost(x,2,"Outside IPv4 reported by 'ipify.org':");
    logpost(x,2," ");
	logpost(x,2,"IP: %s",response);
	

    free(message);


	return;
}

static void nicinfo_free(t_nicinfo *x) {
	
}



void nicinfo_main(t_nicinfo *x) {

  getnic (x);
  pthread_create(&x->tid, NULL, ipify, x);
 	

}



static void *nicinfo_new(void)
{
  t_nicinfo *x = (t_nicinfo *)pd_new(nicinfo_class);

  
 
  return (void *)x;
}




void nicinfo_setup(void) {

  nicinfo_class = class_new(gensym("nicinfo"),      
			       (t_newmethod)nicinfo_new,
			       (t_method)nicinfo_free,                          
			       sizeof(t_nicinfo),       
			       CLASS_DEFAULT,				   
			       0);                        

  class_addbang(nicinfo_class, nicinfo_main);  
  //class_addmethod(nicinfo_class, (t_method)nicinfo_main, gensym("test"), A_GIMME, 0);

}