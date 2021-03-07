#!/usr/bin/env bats
#!/usr/bin/bash
#
# verifies that inputs produce expected outputs
#set -x

EXPECTED=test/expected/test_out
RECEIVED=test/received/test_out
EXE="./cgol -ns0 "

function setup(){
	mkdir -p "$RECEIVED/$FILE"
}
function teardown(){
	rm -rf "$RECEIVED/$FILE"
}

@test "using default file, -i10" {
	FILE=i10
	ARGS="-$FILE"
	$EXE $ARGS > "$RECEIVED/$FILE"
	echo "received $RECEIVED/$FILE"
	echo "expected $EXPECTED/$FILE"
	diff "$EXPECTED/$FILE" "$RECEIVED/$FILE"
	cat "$EXPECTED/$FILE"
	[[ "" = "$(diff $EXPECTED/$FILE $RECEIVED/$FILE)" ]]
}
