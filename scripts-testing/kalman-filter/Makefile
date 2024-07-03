objects = kalman_test.o kalman.o

CXXFLAGS = -Werror -I./BasicLinearAlgebra/test -I./BasicLinearAlgebra

kalman : $(objects)
	g++ -o kalman $(objects)

.PHONY : clean run

run: kalman
	./kalman

clean :
	rm -f kalman $(objects)

$(objects) : kalman.h