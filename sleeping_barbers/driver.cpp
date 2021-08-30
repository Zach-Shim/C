// Zach Shim
// Program 4
// CSS 430
#include <iostream>
#include <sys/time.h>
#include <unistd.h>
#include "Shop.h"
using namespace std;

void *barber(void *);
void *customer(void *);

// ThreadParam class
// This class is used as a way to pass more than one argument to a thread. 
class ThreadParam
{
public:
    ThreadParam(Shop* shop, int id, int service_time) : shop(shop), id(id), service_time(service_time) {};
    Shop* shop;         
    int id;             
    int service_time;    
};

// validate input from the command line
// there can be 0 waiting chairs
void validateInput(int argc, char *argv[]) {
   if (argc != 5)            // Read arguments from command line
   {
       cerr << "Usage: [num_barbers] [num_chairs] [num_customers] [service_time]" << endl;
       exit(0);
   }
   if(atoi(argv[1]) <= 0)    // need atleast 1 barber
   {
      cerr << "[num_barbers] must be > 0" << endl;
      exit(0);
   }
   if(atoi(argv[3]) <= 0)    // need atleast 1 customer
   {
      cerr << "[num_customers] must be > 0" << endl;
      exit(0);
   }
   if(atoi(argv[4]) < 1000)  // customers must be serviced for atleast 1 second
   {
      cerr << "[service_time] must be >= 1000" << endl;
      exit(0);
   }
}

int main(int argc, char *argv[]) 
{
   validateInput(argc, argv);
   int nBarbers  = atoi(argv[1]);
   int num_waiting_chairs = atoi(argv[2]);
   int num_customers = atoi(argv[3]);
   int service_time = atoi(argv[4]);

   // one shop is shared by many barber and customer threads
   Shop shop(nBarbers, num_waiting_chairs);
  
   // instantiate and start nBarber threads
   pthread_t barber_threads[nBarbers];
   for(int i = 0; i < nBarbers; i++) {
      ThreadParam *barber_param = new ThreadParam(&shop, i, service_time);
      pthread_create(&barber_threads[i], NULL, barber, (void*)barber_param);
   }

   // instantiate and start num_customers threads
   pthread_t customer_threads[num_customers];
   for (int i = 0; i < num_customers; i++)
   {
      usleep(rand() % 1000);
      ThreadParam *customer_param = new ThreadParam(&shop, i+1, 0);
      pthread_create(&customer_threads[i], NULL, customer, (void*)customer_param);
   }

   // Wait for customers to finish
   for (int i = 0; i < num_customers; i++)
   {
      pthread_join(customer_threads[i], NULL);
   }

   // Cancel barber execution after all customers finish
   for (int i = 0; i < nBarbers; i++)
   {
      pthread_cancel(barber_threads[i]);
   }
   cout << "# customers who didn't receive a service = " << shop.get_cust_drops() << endl;
   return 0;
}

// used by barber threads; each thread shares a shop and has a unique id; serves all customers until it is cancelled
void *barber(void *arg) 
{
   ThreadParam &param = *(ThreadParam *) arg; 
   Shop &shop = *(param.shop);               // a pointer to the Shop object
   int id = param.id;                        // a thread identifier
   int service_time = param.service_time;    // service time (usec) for barber; 0 for customer
   delete &param;

   while(true) 
   {
      shop.helloCustomer(id);
      usleep(service_time);
      shop.byeCustomer(id);
   }
   return nullptr;
}

// used by customer threads; each thread shares a shop and has a unique id; threads enter the shop and either get a haircut or leave the shop
void *customer(void *arg) 
{
   ThreadParam &param = *(ThreadParam *) arg; 
   Shop &shop = *(param.shop);                  // a pointer to the Shop object
   int id = param.id;                           // a thread identifier
   delete &param;                               

   // if assigned to barber i then wait for service to finish 
   // -1 means did not get barber
   int barber = -1;
   if ((barber = shop.visitShop(id)) != -1)
   {
       shop.leaveShop(id, barber);              // wait until my service is finished
   }
   return nullptr;
}
