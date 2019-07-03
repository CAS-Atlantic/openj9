#!/bin/bash
# CASA Watchdog
# Author: Aaron Graham <aaron.graham@unb.ca>

RC=0
arg_num=0
PWD=`pwd`
COMMAND_TO_EXECUTE="$@"
UNBUFFER_OR_STDBUF=""

# echo -e "${0##*/}@${HOSTNAME}: DEBUG: Init:\n"

# for arg in "$@"
# do
	# arg_num=$((LINE + 1))
	# echo -e "${0##*/}@${HOSTNAME}: DEBUG: arg[$arg_num]: $arg\n"
# done

# echo -e "${0##*/}@${HOSTNAME}: DEBUG: PWD: $PWD/\n"
# echo -e "${0##*/}@${HOSTNAME}: DEBUG: UNBUFFER_OR_STDBUF: $UNBUFFER_OR_STDBUF\n"

if type unbuffer >/dev/null 2>&1; then
	UNBUFFER_OR_STDBUF="unbuffer "
	# echo -e "${0##*/}@${HOSTNAME}: DEBUG: UNBUFFER_OR_STDBUF: $UNBUFFER_OR_STDBUF\n"
elif type stdbuf >/dev/null 2>&1; then
	UNBUFFER_OR_STDBUF="stdbuf -o 0 -e 0 "
	# echo -e "${0##*/}@${HOSTNAME}: DEBUG: UNBUFFER_OR_STDBUF: $UNBUFFER_OR_STDBUF\n"
fi

echo -e "${0##*/}@${HOSTNAME}: COMMAND_TO_EXECUTE: '$PWD/$COMMAND_TO_EXECUTE'\n"

# NOTE: Needs to output 1 more line after the '*error:*' line (to fail successfully):
OUTPUT=`cd $PWD/ && $UNBUFFER_OR_STDBUF$COMMAND_TO_EXECUTE |& tee /dev/stderr | sed -e '/: error:/q'`

# echo -e "${0##*/}@${HOSTNAME}: DEBUG: OUTPUT:\n'\n$OUTPUT\n'\n"

GREP_FOR_ERRORS="" # TODO: Remove.
# GREP_FOR_ERRORS=`cat $OUTPUT | grep \"error:\"`

if [ "$GREP_FOR_ERRORS" != "" ];
then
	echo -e "${0##*/}@${HOSTNAME}: ERROR: '$COMMAND_TO_EXECUTE' Command Has Errors: Set RC($RC) to '-19'\n"
	RC=-19
	echo -e "${0##*/}@${HOSTNAME}: ERROR: '$COMMAND_TO_EXECUTE' Errors: \n'\n$GREP_FOR_ERRORS\n'\n"
elif [ $RC -ne 0 ];
then
	echo -e "${0##*/}@${HOSTNAME}: ERROR: : Set RC($RC) to '$RC'\n"
fi

# echo -e "${0##*/}@${HOSTNAME}: DEBUG: RC: $RC\n"
# echo -e "${0##*/}@${HOSTNAME}: DEBUG: End.\n"

exit $RC
