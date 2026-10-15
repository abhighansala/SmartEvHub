#ifndef CHARGING_H
#define CHARGING_H

#include "customer.h"
#include <iostream>
#include <string>

using namespace std;

class Charging
{
private:
    int station_id;
    int vehicle_id;
    int battery_id;

    Customer customer;

    string vehicle_name;

public:

    Charging(int sid, int vid, int bid, int cid)
        : station_id(sid),
          vehicle_id(vid),
          battery_id(bid),
          customer(cid)
    {
    }

    void assignNewCharging()
    {
        cout << "Charging Data" << endl;

        cout << "Enter Station ID: ";
        cin >> station_id;

        cout << "Enter Vehicle ID: ";
        cin >> vehicle_id;

        cout << "Enter Battery ID: ";
        cin >> battery_id;

        cout << "Enter Vehicle Name: ";
        cin >> vehicle_name;

        customer.getCustomerDetails();
    }

    void display()
    {
        cout << "\n--- Charging Queue Data ---" << endl;

        cout << "Station ID: " << station_id << endl;
        cout << "Vehicle ID: " << vehicle_id << endl;
        cout << "Battery ID: " << battery_id << endl;
        cout << "Customer ID: " << customer.customer_id << endl;
        cout << "Vehicle Name: " << vehicle_name << endl;
    }
};

#endif