#!/usr/bin/env bats
#!/usr/bin/bash
#
# verifies that inputs produce expected outputs
#
# ./cgol -i10 -s0 -n > test/expected/test_out/i10
#
EXPECTED="$BATS_TEST_DIRNAME/expected/test_out"
RECEIVED="$BATS_TEST_DIRNAME/received/test_out"
DIR="$1"
EXE="$BATS_TEST_DIRNAME/../build/cgol -ns0 -d $BATS_TEST_DIRNAME/../../../data -f default"

function setup(){
	mkdir -p "$RECEIVED/$FILE"
}

function expected(){
	FILE="i$1"
	ARGS="-$FILE"
	$EXE $ARGS > "$RECEIVED/$FILE"
	echo "received $RECEIVED/$FILE"
	echo "expected $EXPECTED/$FILE"
	diff "$EXPECTED/$FILE" "$RECEIVED/$FILE"
	cat "$EXPECTED/$FILE"
	[[ "" = "$(diff $EXPECTED/$FILE $RECEIVED/$FILE)" ]]
}

@test "using default file, -i10" {
	expected "10"
}

@test "using default file, -i1024" {
	expected "1024"
}
