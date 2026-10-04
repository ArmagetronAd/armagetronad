#!/usr/bin/env bash

#set -x

TOPIC=Misc
case "$1" in
	*/tools/*)
		TOPIC=Tools
		;;
	*/network/*)
		TOPIC=Network
		;;
	*/engine/*)
		TOPIC=Engine
		;;
	*/game/*)
		TOPIC=Game
		;;
	*/ui/*)
		TOPIC=UI
		;;
	*/render/*)
		TOPIC=Render
		;;
	*/test/*)
		TOPIC=Tests
		;;
	*/thirdparty/*)
		TOPIC=Thirdparty
		;;
	*/doc/*)
		TOPIC=Doc
		;;
esac

# Add @ingroup before class and structs
filter_add_topic()
{
	DECORATE=true

	while IFS= read -r line; do
		case "$line" in
			*\;*)
				# forward declarations do not get decorated
				;;
			class*|struct*|template*|///*|//!*)
				# class, struct and top level doc coments get decorated
				if [ "$DECORATE" = "true" ]; then
					echo /// @ingroup $TOPIC
					DECORATE=false
				fi
				;;
			*)
				DECORATE=true;
				;;
		esac
		echo "$line"
	done
}

# no processing if groups are already used
if grep '@ingroup' "$1" > /dev/null; then
	cat "$1"
	exit 0
fi

cat "$1" | filter_add_topic
