#!/bin/bash
# Copyright (c) 2015 Fabian Schuiki
# Runs a set of tests.

set -e

# assume "lowc" as the default low compiler
if [ -z "$LOWC" ]; then
	LOWC=lowc
fi

# assume "lli" as the default LLVM interpreter
if [ -z "$LLI" ]; then
	LLI=lli
fi

echo "0" > .test_passed
echo "0" > .test_failed

hr() { printf "%*s\n" "${COLUMNS:-$(tput cols)}" "" | tr " " -; }
log_skip() { printf "\r[skip]  %s\n" "$1"; }
log_fail() { printf "\r[`tput setaf 1`fail`tput sgr0`]  %s\n" "$1"; expr `cat .test_failed` + 1 > .test_failed; }
log_pass() { printf "\r[`tput setaf 2`pass`tput sgr0`]  %s\n" "$1"; expr `cat .test_passed` + 1 > .test_passed; }




DIR=$(dirname $0)

# iterate over all tests in the test directory
find $DIR -name "*.low" -print0 | while read -d $'\0' TEST; do
	TEST_OUT=${TEST%.low}.ll
	TEST_NAME=${TEST#$DIR/}
	TEST_DIR=$(dirname "$TEST")
	printf "[....]  %s" "$TEST_NAME"

	# determine what to expect from this test
	COMP_PASS=$(grep -icw -m 1 "\+compile" "$TEST" || true)
	COMP_FAIL=$(grep -icw -m 1 "\-compile" "$TEST" || true)
	EXEC_PASS=$(grep -icw -m 1 "\+execute" "$TEST" || true)
	EXEC_FAIL=$(grep -icw -m 1 "\-execute" "$TEST" || true)
	if [ $EXEC_PASS == 1 ] || [ $EXEC_FAIL == 1 ]; then
		if [ $COMP_FAIL == 1 ]; then
			log_fail "$TEST_NAME"
			printf "        test wants to execute but also to fail compilation\n"
			continue
		fi
		COMP_PASS=1
	fi

	# make sure any form of compilation was configured
	if [ $COMP_PASS == 0 ] && [ $COMP_FAIL == 0 ]; then
		log_skip "$TEST_NAME"
		continue
	fi

	# determine what additional files need to be compiled
	ALSO_RAW=$(grep -iohw "@[a-z0-9_/.]*" "$TEST" | cut -c2-)
	ALSO=
	for f in $ALSO_RAW; do
		ALSO="$ALSO $TEST_DIR/$f"
	done

	# compile the program
	if "$LOWC" "$TEST" $ALSO -o "$TEST_OUT" 1>.out 2>&1; then
		if [ $COMP_FAIL = 1 ]; then
			log_fail "$TEST_NAME"
			printf "        compilation succeeded, but should have failed\n"
			continue
		fi
	else
		if [ $COMP_PASS = 1 ]; then
			log_fail "$TEST_NAME"
			printf "        compilation failed\n"
			hr
			cat .out
			hr
			continue;
		fi
	fi

	# execute the program if configured that way
	if [ $EXEC_PASS == 1 ] || [ $EXEC_FAIL == 1 ]; then
		if "$LLI" "$TEST_OUT" 1>.out 2>&1; then
			if [ $EXEC_FAIL = 1 ]; then
				log_fail "$TEST_NAME"
				printf "      execution succeeded, but should have failed\n"
				continue
			fi
		else
			if [ $EXEC_PASS = 1 ]; then
				log_fail "$TEST_NAME"
				printf "      execution failed\n"
				hr
				cat .out
				hr
				continue;
			fi
		fi
	fi

	if [ -e "$TEST_OUT" ]; then
		rm "$TEST_OUT"
	fi

	log_pass "$TEST_NAME"
done

NUM_PASSED=`cat .test_passed`
NUM_FAILED=`cat .test_failed`

rm .test_passed
rm .test_failed
if [ -e .out ]; then
	rm .out
fi

if [ $NUM_FAILED = 0 ]; then
	printf "\n  all %d tests passed\n\n" $NUM_PASSED
	exit 0
else
	NUM_TOTAL=$(expr $NUM_FAILED + $NUM_PASSED)
	printf "\n  %d/%d tests failed\n\n" $NUM_FAILED $NUM_TOTAL
	exit 1
fi
