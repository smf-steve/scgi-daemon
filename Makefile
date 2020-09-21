all: scgi-launch scgi2env-exec

scgi-launch:
	touch 

scgi2env-exec: scgi2env-exec.c
	$(CC) -o scgi2env-exec scgi2env-exec.c

