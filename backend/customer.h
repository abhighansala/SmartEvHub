#ifndef CUSTOMER_H
#define CUSTOMER_H

#include <iostream>
#include<string>
#include<cmath>
using namespace std;

class Customer
{
private:
    string vehicle_name, customer_name;
public:
     int customer_id;
    Customer(){
    }
    Customer(int c_id){
        customer_id = c_id;
    }
    int getCustomerDetails()
    {
        customer_id = rand() % 1000 + 1;
        cout<<"Customer ID: "<<customer_id;
        cout << "Enter Customer name";
        cin >> customer_name;
        cout << "Enter Vehicle Name";
        return customer_id;
    }
};

#endif