# makefile Michal Kysilko xkysil00


NAME=barber
CC=gcc
CFLAGS=-std=gnu99 -Wall -Wextra -Werror -pedantic -lrt

$(NAME): $(NAME).c
	$(CC) $(CFLAGS) $(NAME).c -o $(NAME)
