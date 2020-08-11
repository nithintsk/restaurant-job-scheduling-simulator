##############
# How to Use #
##############
1. Run the make command
   $ make

2. Run the executable with the input file as a argument
   $ ./Restaurant input.txt

########################
# Description of files #
########################
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


