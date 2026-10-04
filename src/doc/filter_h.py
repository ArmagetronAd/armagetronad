#!/usr/bin/env python3

import sys
import os


def get_topic(filepath):
    """Determine the topic based on the file path."""
    path_lower = filepath.lower()
    
    if '/tools/' in path_lower:
        return 'Tools'
    elif '/network/' in path_lower:
        return 'Network'
    elif '/engine/' in path_lower:
        return 'Engine'
    elif '/game/' in path_lower:
        return 'Game'
    elif '/ui/' in path_lower:
        return 'UI'
    elif '/render/' in path_lower:
        return 'Render'
    elif '/test/' in path_lower:
        return 'Tests'
    elif '/thirdparty/' in path_lower:
        return 'Thirdparty'
    elif '/doc/' in path_lower:
        return 'Doc'
    else:
        return 'Misc'


def is_special_line(line):
    """Check if line matches class*, struct*, template*, ///*, or //!* patterns.
    
    This matches the bash case statement which checks the raw line (not stripped).
    """
    return (line.startswith('class') or
            line.startswith('struct') or
            line.startswith('template') or
            line.startswith('///') or
            line.startswith('//!'))


def filter_add_topic(input_file, topic):
    """Add @ingroup comments before class/struct/template definitions."""
    decorate = True
    output_lines = []
    
    with open(input_file, 'r') as f:
        for line in f:
            line_stripped = line.rstrip('\n\r')
            
            # Check each case in order, matching bash case statement behavior
            # The bash pattern "*\;*" matches any line containing a semicolon
            if ';' in line_stripped:
                # Forward declarations/lines with semicolons - do nothing, DECORATE stays as is
                output_lines.append(line_stripped)
            elif is_special_line(line_stripped):
                # class, struct, template, or doxygen comment (at start of line)
                if decorate:
                    output_lines.append(f'/// @ingroup {topic}')
                    decorate = False
                output_lines.append(line_stripped)
            else:
                # Any other line - enable decoration for next matching line
                decorate = True
                output_lines.append(line_stripped)
    
    return '\n'.join(output_lines)


def main():
    if len(sys.argv) < 2:
        print("Usage: filter_h.py <file.h>", file=sys.stderr)
        sys.exit(1)
    
    input_file = sys.argv[1]
    
    # Check if file already has @ingroup
    with open(input_file, 'r') as f:
        content = f.read()
        if '@ingroup' in content:
            # Just output the file as-is
            print(content, end='')
            sys.exit(0)
    
    # Determine topic
    topic = get_topic(input_file)
    
    # Process and output
    result = filter_add_topic(input_file, topic)
    print(result)


if __name__ == '__main__':
    main()
