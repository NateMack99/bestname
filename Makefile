all: filter_name.c
	gcc-15 -O2 -fopenmp filter_name.c -o filter_name

clean:
	rm filter_name