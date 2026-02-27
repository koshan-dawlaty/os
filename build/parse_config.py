#!/usr/bin/env python

import sys

def main():
    for line in sys.stdin:
        if (line[0] == '#'):
            continue

        sys.stdout.write("-DCONFIG_" + line)

if __name__ == "__main__":
    main()
