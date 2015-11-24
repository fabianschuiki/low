LOWC ?= lowc
LINK ?= llvm-link

all: main.bc

clean:
	rm *.ll *.bc

math.bc: fibonacci.low square.low
	${LOWC} fibonacci.low square.low
	${LINK} -o=math.bc fibonacci.ll square.ll


main.bc: main.low math.bc
	${LOWC} main.low
	${LINK} -o=main.bc main.ll math.bc
