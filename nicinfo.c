#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
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

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/bio.h>

#define HOST "api.ipify.org"
#define PORT "443"

#include "m_pd.h"


typedef enum _thrd_request
{
  IDLE = 0,
  IP = 1,
  QUIT = 2, 
} t_thrd_request;


typedef struct _nicinfo {
  t_object  x_obj;
  t_outlet *x_outlet1;
  t_atom x_outsideip[3];
  t_thrd_request x_requestcode;
  pthread_mutex_t x_mutex;
  pthread_cond_t x_requestcondition;
  pthread_t x_tid;
  t_clock *x_clock;
   
  } t_nicinfo;


t_class *nicinfo_class;


static void nicnfo_getnic (t_nicinfo *x) {
	
    t_atom nics[3];
    
#ifdef _WIN32

    DWORD asize = 20000;
    PIP_ADAPTER_ADDRESSES adapters;
    do {
        adapters = (PIP_ADAPTER_ADDRESSES)malloc(asize);
        if (!adapters) {
            pd_error(x,"Couldn't allocate %ld bytes for adapters.\n", asize);
            return;
        }
        int r = GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX, 0,
                adapters, &asize);
        if (r == ERROR_BUFFER_OVERFLOW) {
            pd_error(x,"GetAdaptersAddresses wants %ld bytes.\n", asize);
            free(adapters);
        } else if (r == ERROR_SUCCESS) {
            break;
        } else {
            pd_error(x,"Error from GetAdaptersAddresses: %d\n", r);
            free(adapters);
            return;
        }
    } while (!adapters);
    
    PIP_ADAPTER_ADDRESSES adapter = adapters;
    while (adapter) {
        //printf("\nAdapter name: %S\n", adapter->FriendlyName);        
        char *foo = (char *)malloc( sizeof(adapter->FriendlyName)+1 );
        size_t i;
        wcstombs_s(&i, foo, (size_t) sizeof(adapter->FriendlyName)+1,
               adapter->FriendlyName, (size_t)sizeof(adapter->FriendlyName) );
        SETSYMBOL(nics+0, gensym(foo));
        PIP_ADAPTER_UNICAST_ADDRESS address = adapter->FirstUnicastAddress;
        while (address) {
            //printf("\t%s",
            //        address->Address.lpSockaddr->sa_family == AF_INET ?
            //        "IPv4" : "IPv6");
            SETSYMBOL(nics+1, address->Address.lpSockaddr->sa_family == AF_INET ?
                    gensym("IPv4") : gensym("IPv6"));
            char ap[100];

            getnameinfo(address->Address.lpSockaddr,
                    address->Address.iSockaddrLength,
                    ap, sizeof(ap), 0, 0, NI_NUMERICHOST);
            //printf("\t%s\n", ap);
            SETSYMBOL(nics+2, gensym(ap));
            outlet_list(x->x_outlet1, 0, 3, nics);
            address = address->Next;
        }
        adapter = adapter->Next;
    }
    free(adapters);    
#else
    
    struct ifaddrs *addresses;

    if (getifaddrs(&addresses) == -1) {
        pd_error(x,"getifaddrs call failed\n");
        return;
    }    
    struct ifaddrs *address = addresses;
    while(address) {
        int family = address->ifa_addr->sa_family;
        if (family == AF_INET || family == AF_INET6) {
            //printf("%s\t", address->ifa_name);
            SETSYMBOL(nics+0, gensym(address->ifa_name));
            //printf("%s\t", family == AF_INET ? "IPv4" : "IPv6");
            SETSYMBOL(nics+1, family == AF_INET ? gensym("IPv4") : gensym("IPv6"));
            char ap[100];
            const int family_size = family == AF_INET ?
                sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6);
            getnameinfo(address->ifa_addr,
                    family_size, ap, sizeof(ap), 0, 0, NI_NUMERICHOST);
            //printf("\t%s\n", ap);
            SETSYMBOL(nics+2, gensym(ap));
            outlet_list(x->x_outlet1, 0, 3, nics);
        }
        address = address->ifa_next;
    }
    freeifaddrs(addresses);    
#endif
}

static void nicinfo_clock_tick(t_nicinfo *x)
{    
    outlet_list(x->x_outlet1, 0, 3, x->x_outsideip);    
}


static void nicnfo_ipify(t_nicinfo *x) 
{
    //
    //  Initialize the variables
    //
    BIO* bio;
    SSL* ssl;
    SSL_CTX* ctx;

    //
    //   Registers the SSL/TLS ciphers and digests.
    //
    //   Basically start the security layer.
    //
    SSL_library_init();

    //
    //  Creates a new SSL_CTX object as a framework to establish TLS/SSL
    //  or DTLS enabled connections
    //
    ctx = SSL_CTX_new(SSLv23_client_method());

    //
    //  -> Error check
    //
    if (ctx == NULL)
    {
        //printf("Ctx is null\n");
    }

    //
    //   Creates a new BIO chain consisting of an SSL BIO
    //
    bio = BIO_new_ssl_connect(ctx);

    //
    //  Use the variable from the beginning of the file to create a 
    //  string that contains the URL to the site that you want to connect
    //  to while also specifying the port.
    //
    BIO_set_conn_hostname(bio, HOST ":" PORT);

    //
    //   Attempts to connect the supplied BIO
    //
    if(BIO_do_connect(bio) <= 0)
    {
        printf("Failed connection\n");
        exit(1);
    }
    else
    {
        //printf("Connected\n");
    }

    //
    //  The bare minimum to make a HTTP request.
    //
    char* write_buf = "GET / HTTP/1.1\r\n"
                      "Host: " HOST "\r\n"
                      "Connection: close\r\n"
                      "\r\n";

    //
    //   Attempts to write len bytes from buf to BIO
    //
    if(BIO_write(bio, write_buf, strlen(write_buf)) <= 0)
    {
        //
        //  Handle failed writes here
        //
        if(!BIO_should_retry(bio))
        {
            // Not worth implementing, but worth knowing.
        }

        //
        //  -> Let us know about the failed writes
        //
        printf("Failed write\n");
    }

    //
    //  Variables used to read the response from the server
    //
    int size;
    char buf[1024];

    //
    //  Read the response message
    //
    for(;;)
    {
        //
        //  Get chunks of the response 1023 at the time.
        //
        size = BIO_read(bio, buf, 1023);

        //
        //  If no more data, then exit the loop
        //
        if(size <= 0)
        {
            break;
        }

        //
        //  Terminate the string with a 0, to let know C when the string 
        //  ends.
        //
        buf[size] = '\0';

        //
        //  ->  Print out the response
        //
        //printf("%s", buf);
        
        //
        //  Do some stuff here to get only the IPv4 we asked to https://www.ipify.org/. 
        //
        
        char buf2[20] = {'\0'};
        int i, lastlf;
        for(i=0;i<200;i++)
        {
            if(buf[i] == 0x0a)
            lastlf = i;
        }
        strncpy(buf2, buf+(lastlf+1), 20);
        //printf("%s", buf2);
        SETSYMBOL(x->x_outsideip+0, gensym("Outside"));
        SETSYMBOL(x->x_outsideip+1, gensym("IPv4"));
        SETSYMBOL(x->x_outsideip+2, gensym(buf2));
        
    }

    //
    //  Clean after ourselves
    //
    BIO_free_all(bio);
    SSL_CTX_free(ctx);
    
    //
    // From Pd thread
    //

    pthread_mutex_lock(&x->x_mutex);
    if (x->x_requestcode != QUIT)
    {
        sys_lock();
        clock_delay(x->x_clock, 0);
        sys_unlock();
    }
    pthread_mutex_unlock(&x->x_mutex);
    
    return;   


}

static void nicinfo_thread(t_nicinfo *x)
{
    while (1) 
    {
        pthread_mutex_lock(&x->x_mutex);
        while (x->x_requestcode == IDLE) 
        {
            pthread_cond_wait(&x->x_requestcondition, &x->x_mutex);
        }
        if (x->x_requestcode == IP)
        {
            pthread_mutex_unlock(&x->x_mutex);
            nicnfo_ipify(x);
            pthread_mutex_lock(&x->x_mutex);
            if (x->x_requestcode == IP)
                x->x_requestcode = IDLE;
            pthread_mutex_unlock(&x->x_mutex);
        }
        else if (x->x_requestcode == QUIT)
        {
            pthread_mutex_unlock(&x->x_mutex);
            break;
        }
    }
    
}

void nicinfo_main(t_nicinfo *x) {

    nicnfo_getnic(x);    
    pthread_mutex_lock(&x->x_mutex);
    x->x_requestcode = IP;
    pthread_mutex_unlock(&x->x_mutex);
    pthread_cond_signal(&x->x_requestcondition); 	

}

static void nicinfo_free(t_nicinfo *x) {
    
    pthread_mutex_lock(&x->x_mutex);
    x->x_requestcode = QUIT;
    pthread_mutex_unlock(&x->x_mutex);  
    pthread_cond_signal(&x->x_requestcondition);
    pthread_join(x->x_tid, NULL);
    pthread_cond_destroy(&x->x_requestcondition);
    pthread_mutex_destroy(&x->x_mutex);
    clock_free(x->x_clock);	
}

static void *nicinfo_new(void)
{
    t_nicinfo *x = (t_nicinfo *)pd_new(nicinfo_class);
    x->x_clock = clock_new(x, (t_method)nicinfo_clock_tick);  
    x->x_outlet1 = outlet_new(&x->x_obj, 0);    
    pthread_mutex_init(&x->x_mutex, 0);
    pthread_cond_init(&x->x_requestcondition, 0);
    pthread_create(&x->x_tid, NULL, nicinfo_thread, x);
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

}