README

Annie Steenson
COM S 327
Assignment 1.03

Overview:
This program can generate, save, and load a dungeon. After either generating or loading a dungeon, it will print distance maps for monsters that can tunnel and for monsters that cannot tunnel.

Implementation:
Path.c was created to implement dijkstra's algorithm. This algorithm loops through the dungeon and assigns relative distances. Each distance is denoted by 0-9, 'a-z', and 'A-Z' from least to greatest.
Rlg327.c has the main function which calls each path-finding algorithm and prints out the corresponding distance maps.