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
		    *@ingroup*)
				# already decorated, stop
				DECORATE=false
				;;
			*\;*)
				# forward declarations do not get decorated
				;;
			class*|struct*)
				# class and struct get decorated
				if [ "$DECORATE" = "true" ]; then
					echo /// @ingroup $TOPIC
				fi
				;;
		esac
		echo "$line"
	done
}

cat "$1" | filter_add_topic
