// Zach Shim
// Program 4
// CSS 430
#include "Shop.h"

void Shop::init() 
{
   pthread_mutex_init(&mutex_, NULL);

   // each barber knows its customer 
   barbers_customer_ = new int[max_barber_chairs_] {0};
   barber = new Barber[max_barber_chairs_];

   // initialize all barber attributes
   for(int i = 0; i < max_barber_chairs_; i++) {
      barber[i].id = i;
      pthread_cond_init(&(barber[i].cond_barber_paid_), NULL);
      pthread_cond_init(&(barber[i].cond_barber_sleeping_), NULL);
      pthread_cond_init(&(barber[i].cond_customer_served), NULL);
      barber[i].bstate = WAITING;
   }

   // for a customer that just woke up, wait for barber to arrive
   pthread_cond_init(&cond_customer_wait, NULL);
   pthread_cond_init(&cond_barber_has_arrived, NULL);
}

void Shop::destroy() 
{
   delete []barbers_customer_;
   delete []barber;
}

string Shop::int2string(int i) 
{
   stringstream out;
   out << i;
   return out.str( );
}

// print output for either barber or customer
// when isCustomer is true, print customer; when isCustomer is false, print barber
void Shop::print(int id, string message, bool isCustomer)
{
   cout << ((isCustomer) ? "customer[" : "barber  [" ) << id << "]: " << message << endl;
}

// returns all customers that left the barber shop when all waiting chairs were full
int Shop::get_cust_drops() const
{
    return cust_drops_;
}

// called by customer thread to visit the shop
// when a customer first arrives, it grabs the mutex and either starts getting a haircut or waits for an available barber
int Shop::visitShop(int id) 
{
   pthread_mutex_lock(&mutex_);
   
   int current_barber;

   // if all barbers are busy
   if (being_served.size() == max_barber_chairs_ || sleeping_barbers.empty()) 
   {
      // If all waiting chairs are full, then leave the barber shop
      if (waiting_chairs_.size() == max_waiting_chairs_) 
      {
         print(id, "leaves the shop because of no available waiting chairs.", true);
         ++cust_drops_;
         pthread_mutex_unlock(&mutex_);
         return -1;
      }

      // add customer to waiting queue, pop when woken up by barber
      waiting_chairs_.push(id);
      print(id, "takes a waiting chair. # waiting seats available = " + int2string(max_waiting_chairs_ - waiting_chairs_.size()), true);
      pthread_cond_wait(&cond_customer_wait, &mutex_);
      waiting_chairs_.pop();

      // customer is awake, but if barber has not fallen asleep, wait until they do so barber and customer can sync
      while(barber[customers_barber_[id]].bstate != WAITING) 
      {
         pthread_cond_wait(&cond_barber_has_arrived, &mutex_);
      } 
      waiting_for_customer_.pop();                    // pop barber

      current_barber = customers_barber_[id];
      barbers_customer_[current_barber] = id; 
   } 
   // new customer; there is a barber available
   else 
   {
      current_barber = sleeping_barbers.front();
      sleeping_barbers.pop();                         // pop barber
      customers_barber_[id] = current_barber;
      barbers_customer_[current_barber] = id;   
   }
   
   being_served.push(id);
   print(id, "moves to the service chair[" + int2string(current_barber) + "], # waiting seats available = " + int2string(max_waiting_chairs_ - waiting_chairs_.size()), true);
   
   // wake up the barber just in case if he is sleeping
   pthread_cond_signal(&(barber[current_barber].cond_barber_sleeping_));

   pthread_mutex_unlock(&mutex_); 
   return current_barber;
}

// called by customer thread to leave the shop
// customer waits for haircut to be done; when it is done, pay and signal barber
void Shop::leaveShop(int customer_id, int barber_id) 
{
   pthread_mutex_lock(&mutex_);

   // Wait for service to be completed
   print(customer_id, "waits for barber[" + int2string(barber_id) + "] to be done with hair-cut", true);
   while (barber[barber_id].bstate != DONE)
   {
      pthread_cond_wait(&barber[barber_id].cond_customer_served, &mutex_);
   }

   // Pay the barber and signal barber appropriately
   barber[barber_id].bstate = PAID;
   pthread_cond_signal(&(barber[barber_id].cond_barber_paid_));
   print(customer_id, "says good-bye to barber[" + int2string(barber_id) + "]", true);

   pthread_mutex_unlock(&mutex_);
}

// called by a barber thread
void Shop::helloCustomer(int barber_id) 
{
   pthread_mutex_lock(&mutex_);
   
   // when a barber first arrives, check if there are customers already waiting to avoid a deadlock
   if(firstTimeThrough) 
   {
      firstTimeThrough = false;
      if(!waiting_chairs_.empty()) {
         customers_barber_[waiting_chairs_.front()] = barber_id;
         barber[barber_id].bstate = IN_SERVICE;
         pthread_cond_signal(&cond_customer_wait);     
      }
   }

   // barber has just woken up a customer
   if(barber[barber_id].bstate == IN_SERVICE) 
   {
      // check is customer has woken up yet from sleeping, if not, signal them and sleep
      while (barbers_customer_[barber_id] == 0) 
      {
         barber[barber_id].bstate = WAITING;
         pthread_cond_signal(&cond_barber_has_arrived);
         waiting_for_customer_.push(barber_id);
         pthread_cond_wait(&(barber[barber_id].cond_barber_sleeping_), &mutex_);
      }
   }
   // No customer waiting, now barber can sleep
   else 
   {
      while (waiting_chairs_.empty() && barbers_customer_[barber_id] == 0) 
      {
         print(barber_id, "sleeps because of no customers.", false);
         sleeping_barbers.push(barber_id);
         pthread_cond_wait(&(barber[barber_id].cond_barber_sleeping_), &mutex_);
      }
   }
   
   barber[barber_id].bstate = IN_SERVICE;
   print(barber_id, "starts a hair-cut service for customer[" + int2string(barbers_customer_[barber_id]) + "]", false);
   pthread_mutex_unlock(&mutex_);
}

// called by a barber thread
void Shop::byeCustomer(int barber_id) 
{
   pthread_mutex_lock(&mutex_);

   int customer_id = barbers_customer_[barber_id];       // current customer being worked on by barber
   print(barber_id, "says he's done with a hair-cut service for customer[" + int2string(customer_id) + "]", false);
   
   // barber is done serving customer, update attributes
   barbers_customer_[barber_id] = 0;            
   customers_barber_[customer_id] = 0;
   barber[barber_id].bstate = DONE;             

   pthread_cond_signal(&barber[barber_id].cond_customer_served);     // signal customer that the haircut is done

   while (barber[barber_id].bstate != PAID)                          // wait to be paid by customer
   {
      pthread_cond_wait(&(barber[barber_id].cond_barber_paid_), &mutex_);
   }

   barber[barber_id].bstate = WAITING;
   being_served.pop();      
   print(barber_id, "calls in another customer", false);

   // call in new customer if there is at least one waiting
   if(!waiting_chairs_.empty()) {
      customers_barber_[waiting_chairs_.front()] = barber_id;
      barber[barber_id].bstate = IN_SERVICE;
   } 

   pthread_cond_signal(&cond_customer_wait);     // back to visitShop

   pthread_mutex_unlock(&mutex_);                // unlock mutex
}
