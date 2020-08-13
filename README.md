# Description
A restaurant has patrons arriving at regular intervals. They are to be seated at available tables if they are free, or wait for a table to become available. The place an order consisting of a burger, zero or more fries and zero or more coke, once seated.

Cooks are available to cook the order and they in turn use a fixed number of machines available to cook each item (we assume that there is no contention on other possible resources, like servers to take orders and serve the orders). Each cook handles one order at a time. The cook needs to use the burger machine for 5 minutes to prepare each burger, fries machine for 3 minutes for one order of fries, and the soda machine for 1 minute to fill a glass with coke. The cook can use at most one of these three machines at any given time. There is only one machine of each type in the restaurant (so, for example, over a 5-minute duration, only one burger can be prepared).

Once the food (all items at the same time) are brought to a diner's table, they take exactly 30 minutes to finish eating them, and then leave the restaurant, making the table available for another diner (immediately).

Design the ideal way to ensure that service is the fastest.

# Input Format
Diners arrive at the restaurant over a 120 minute duration, which is taken as input by the simulation. If there are N diners arriving, the input has N+3 lines, specifying, in order: number of diners (N), number of tables, number of cooks, and N other lines each with the following four numbers: a number between 0 and 120 stating when the diner arrived (this number is increasing across lines), the number of burgers ordered (1 or higher), number of order of fries (0 or higher), and whether or not coke was ordered (0 or 1).

# Objective
Simulation should output when each diner was seated, which table they were seated in, which cook processed their order, when each of the machines was used for their orders, and when the food was brought to their table. Finally, you should state the time when the last diner leaves the restaurant. To reduce the length of an experiment, I have used one second to simulate one minute.

# How to Use #
Run the make command
  ``` 
  $ make
  ```

Run the executable with the input file as a argument
   ```
   $ ./Restaurant input.txt
   ```

# Description of files #
1. Restaurant.cpp
   - Contains main()
   - Opens input file and reads Number of Cooks, Diners and Tables
   - Initialize all synchroniztion variables
   - Create threads for all the cooks
   - Get START_TIME
   - Read diner information from input line by line and get Time of Entry
   - Wait until Time of Entry < Currrent Time
   - Create thread for the Diner
   - Continue reading until end of file is reached or 120 minutes are up

2. utility.cpp
   - Contains definition of Diner thread
   - Contains definition of Cook thread
   - Contains definition of Diner Class functions
   - Contains all print and utility functions

3. utility.h
   - Contains declaration of all global variables
   - Contains all constant variables


