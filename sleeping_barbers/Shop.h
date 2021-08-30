// Zach Shim
// Program 4
// CSS 430
#ifndef SHOP_H_
#define SHOP_H_
#include <pthread.h>
#include <iostream>
#include <sstream>
#include <string>
#include <queue>
#include <unordered_map>
#include <sys/time.h>
#include <unistd.h>
using namespace std;

#define kDefaultNumChairs 3
#define kDefaultBarbers 1
class Shop 
{
public:
   Shop(int num_barbers, int num_chairs) : max_barber_chairs_((num_barbers > 0 ) ? num_barbers : kDefaultBarbers), max_waiting_chairs_((num_chairs >= 0 ) ? num_chairs : kDefaultNumChairs), cust_drops_(0), customers_being_served_(0)
   { 
      init(); 
   };
   Shop() : max_waiting_chairs_(kDefaultNumChairs), max_barber_chairs_(kDefaultBarbers), cust_drops_(0), customers_being_served_(0)
   { 
      init(); 
   };
   ~Shop()
   {
      destroy();
   };

   int visitShop(int);                             // return barber id only when a customer got a service
   void leaveShop(int, int);
   void helloCustomer(int);
   void byeCustomer(int);
   int get_cust_drops() const;

 private:
   const int max_barber_chairs_;                   // the max number of barber chairs that can be filled
   const int max_waiting_chairs_;                  // the max number of threads that can wait

   bool firstTimeThrough = true;                   // when a barber first arrives, check if there are customers already waiting to avoid a deadlock
   int cust_drops_;                                // the number of customers not serviced because waiting queue was full
   int customers_being_served_;                    // total number of customers currently being served   

   enum state {WAITING, IN_SERVICE, DONE, PAID};   // state machine for struct Barber

   pthread_mutex_t mutex_;                         // Mutexes and condition variables to coordinate threads 
   pthread_cond_t cond_customer_wait;              // waiting condition for all customers
   pthread_cond_t cond_barber_has_arrived;         // for waiting processes that were woken up by a barber, they need to wait for the barber to sleep
   
   struct Barber {                                 // Barber struct holds the condition variables for each respective barber thread 
      int id;
      pthread_cond_t cond_barber_paid_;
      pthread_cond_t cond_barber_sleeping_;
      pthread_cond_t cond_customer_served;
      state bstate;                                // fsm for barber
   };

   Barber *barber;                                 // array of struct Barbers; size = max_barber_chairs_
   unordered_map<int, int> customers_barber_;      // each customer corresponds to a barber; when a customer wakes up, this is how they know their barber
   int *barbers_customer_;                         // each barber corresponds to a customer: customer_in_chair_[barber_id] = customer currently being served by barber_id

   queue<int> waiting_chairs_;                 // includes the ids of all waiting threads
   queue<int> sleeping_barbers;                // includes the ids of all sleeping barber threads
   queue<int> waiting_for_customer_;           // used for barbers waiting for customers to wake up in a separate queue than sleeping barbers to avoid hangs and deadlocks
   queue<int> being_served;                    // used for waiting list size to deter new customers 

   void init();                                // default constructor(s) helper
   void destroy();                             // destructor helper
   void init_Customer(int id);                 // initializes a struct customer
   string int2string(int i);                  
   void print(int person, string message, bool isCustomer);
};
#endif
