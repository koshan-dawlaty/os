#!/usr/bin/env python

import sys

def main():
    config_file = open(sys.argv[1])
    lines = config_file.readlines()

    for line in lines:
        line = line.strip()
        
        if (line[0] == '#'):
            continue

        sys.stdout.write("-DCONFIG_" + line + " ")

if __name__ == "__main__":
    main()
